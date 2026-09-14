#include "oncrypto/c_abi/c_abi.h"
#include "oncrypto/backend/Backend.hpp"
#include "oncrypto/oncrypto.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// ============================================================================
// Internal constants
// ============================================================================

namespace {

constexpr uint8_t STREAM_ALGORITHM_CHACHA20 = 2;

constexpr size_t STREAM_SALT_SIZE = 16;
constexpr size_t STREAM_NONCE_SIZE = 12;
constexpr size_t STREAM_TAG_SIZE = 16;

constexpr size_t STREAM_KEY_SIZE = 32;
constexpr int STREAM_KDF_ITERATIONS = 100000;

constexpr size_t STREAM_FRAME_HEADER_SIZE =
    sizeof(uint32_t) + STREAM_TAG_SIZE;

// Protect the C ABI from pathological allocations caused by malformed input.
// This is intentionally conservative; callers may still split large data into
// multiple update() calls.
constexpr uint32_t STREAM_MAX_CHUNK_SIZE =
    std::numeric_limits<uint32_t>::max() - STREAM_TAG_SIZE;

// ============================================================================
// Thread-local error state
// ============================================================================
//
// Each calling thread receives its own error message. This prevents one
// thread's error from overwriting another thread's diagnostic information.
//

thread_local std::string g_last_error;

// ============================================================================
// Error helpers
// ============================================================================

void clear_last_error() {
    g_last_error.clear();
}

void set_last_error(const std::string& message) {
    g_last_error = message;
}

// ============================================================================
// Buffer helpers
// ============================================================================

void reset_buffer(onc_buffer* buffer) {
    if (buffer == nullptr) {
        return;
    }

    buffer->data = nullptr;
    buffer->size = 0;
}

onc_status write_buffer(
    const std::vector<unsigned char>& source,
    onc_buffer* destination
) {
    if (destination == nullptr) {
        set_last_error("Output buffer pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    reset_buffer(destination);

    if (source.empty()) {
        return ONC_SUCCESS;
    }

    try {
        destination->data = new uint8_t[source.size()];
        destination->size = source.size();

        std::memcpy(
            destination->data,
            source.data(),
            source.size()
        );

        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        reset_buffer(destination);
        set_last_error("Out of memory.");
        return ONC_ERROR_OUT_OF_MEMORY;
    }
}

// ============================================================================
// Input conversion helpers
// ============================================================================

std::vector<unsigned char> make_vector(
    const uint8_t* data,
    size_t length
) {
    std::vector<unsigned char> result;

    if (length == 0) {
        return result;
    }

    result.resize(length);

    std::memcpy(
        result.data(),
        data,
        length
    );

    return result;
}

std::string make_string(
    const uint8_t* data,
    size_t length
) {
    if (length == 0) {
        return {};
    }

    return std::string(
        reinterpret_cast<const char*>(data),
        length
    );
}

// ============================================================================
// Integer serialization helpers
// ============================================================================
//
// The streaming format stores integer values explicitly as little-endian.
// This avoids relying on the host machine's native byte order.
//

void write_u32_le(
    uint8_t* destination,
    uint32_t value
) {
    destination[0] =
        static_cast<uint8_t>(value & 0xFFu);

    destination[1] =
        static_cast<uint8_t>((value >> 8) & 0xFFu);

    destination[2] =
        static_cast<uint8_t>((value >> 16) & 0xFFu);

    destination[3] =
        static_cast<uint8_t>((value >> 24) & 0xFFu);
}

uint32_t read_u32_le(
    const uint8_t* source
) {
    return
        static_cast<uint32_t>(source[0]) |
        (static_cast<uint32_t>(source[1]) << 8) |
        (static_cast<uint32_t>(source[2]) << 16) |
        (static_cast<uint32_t>(source[3]) << 24);
}

// ============================================================================
// Nonce generation
// ============================================================================
//
// ChaCha20-Poly1305 requires nonce uniqueness for a given key.
// Every encrypted stream chunk receives a monotonically increasing counter.
//
// The counter occupies the first eight bytes of the 12-byte nonce.
// The remaining four bytes are zero.
//

void make_stream_nonce(
    std::vector<unsigned char>& nonce,
    uint64_t counter
) {
    nonce.resize(STREAM_NONCE_SIZE);

    std::fill(
        nonce.begin(),
        nonce.end(),
        static_cast<unsigned char>(0)
    );

    for (size_t i = 0; i < sizeof(counter); ++i) {
        nonce[i] =
            static_cast<unsigned char>(
                (counter >> (i * 8)) & 0xFFu
            );
    }
}

// ============================================================================
// Exception mapping
// ============================================================================
//
// The backend currently exposes exceptions rather than a dedicated C error
// object. Keep the C ABI stable by translating backend exceptions into the
// public onc_status enum.
//

onc_status map_encryption_exception(
    const std::exception& exception
) {
    set_last_error(exception.what());
    return ONC_ERROR_ENCRYPTION_FAILED;
}

onc_status map_decryption_exception(
    const std::exception& exception
) {
    const std::string message = exception.what();

    // The backend may report authentication failures through an exception
    // message. Preserve the distinction when it is available.
    if (
        message.find("authentication") != std::string::npos ||
        message.find("Authentication") != std::string::npos ||
        message.find("AEAD") != std::string::npos ||
        message.find("aead_decrypt") != std::string::npos
    ) {
        set_last_error(message);
        return ONC_ERROR_AUTHENTICATION_FAILED;
    }

    set_last_error(message);
    return ONC_ERROR_DECRYPTION_FAILED;
}

} // namespace

// ============================================================================
// Opaque handle definitions
// ============================================================================

struct onc_builder_s {
    crypto::builder::Encryptor encryptor;
    crypto::builder::Decryptor decryptor;
};

struct onc_stream_s {
    // ------------------------------------------------------------------------
    // Common stream state
    // ------------------------------------------------------------------------

    std::string key;

    bool is_encrypt = true;
    bool initialized = false;
    bool finalized = false;

    uint64_t counter = 0;

    // ------------------------------------------------------------------------
    // Encryption state
    // ------------------------------------------------------------------------

    std::vector<unsigned char> salt;
    std::vector<unsigned char> derived_key;

    // ------------------------------------------------------------------------
    // Decryption state
    // ------------------------------------------------------------------------
    //
    // Input passed to update() may end in the middle of a frame. The parser
    // therefore retains incomplete input here until the next update().
    //

    std::vector<unsigned char> pending;
    size_t pending_offset = 0;

    // Header parsing state.
    bool header_complete = false;
};

// ============================================================================
// Metadata and memory management
// ============================================================================

const char* onc_version(void) {
    static const std::string version = [] {
        try {
            return crypto::getVersion();
        } catch (...) {
            return std::string("unknown");
        }
    }();

    return version.c_str();
}

const char* onc_status_to_string(onc_status status) {
    switch (status) {
        case ONC_SUCCESS:
            return "Success";

        case ONC_ERROR_INVALID_ARGUMENT:
            return "Invalid Argument";

        case ONC_ERROR_ENCRYPTION_FAILED:
            return "Encryption Failed";

        case ONC_ERROR_DECRYPTION_FAILED:
            return "Decryption Failed";

        case ONC_ERROR_AUTHENTICATION_FAILED:
            return "Authentication Failed";

        case ONC_ERROR_INVALID_FORMAT:
            return "Invalid Format";

        case ONC_ERROR_IO:
            return "I/O Error";

        case ONC_ERROR_OUT_OF_MEMORY:
            return "Out of Memory";

        case ONC_ERROR_INTERNAL:
            return "Internal Error";

        default:
            return "Unknown Error";
    }
}

const char* onc_get_last_error(void) {
    return g_last_error.c_str();
}

void onc_buffer_free(onc_buffer* buffer) {
    if (buffer == nullptr || buffer->data == nullptr) {
        return;
    }

    delete[] buffer->data;

    buffer->data = nullptr;
    buffer->size = 0;
}

void onc_string_free(onc_string* string) {
    if (string == nullptr || string->str == nullptr) {
        return;
    }

    delete[] string->str;

    string->str = nullptr;
    string->length = 0;
}

// ============================================================================
// One-shot buffer encryption
// ============================================================================

onc_status onc_encrypt_buffer(
    const uint8_t* input,
    size_t input_len,
    const uint8_t* key,
    size_t key_len,
    onc_buffer* out_buf
) {
    clear_last_error();

    if (out_buf == nullptr) {
        set_last_error("Output buffer pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    reset_buffer(out_buf);

    if (input_len > 0 && input == nullptr) {
        set_last_error(
            "Input pointer is null while input length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (key_len > 0 && key == nullptr) {
        set_last_error(
            "Key pointer is null while key length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        const auto input_vector =
            make_vector(input, input_len);

        const auto password =
            make_string(key, key_len);

        const auto encrypted =
            crypto::encrypt(input_vector, password);

        return write_buffer(
            encrypted,
            out_buf
        );
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        reset_buffer(out_buf);
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        reset_buffer(out_buf);
        return map_encryption_exception(exception);
    } catch (...) {
        reset_buffer(out_buf);
        set_last_error(
            "Unknown internal encryption error."
        );
        return ONC_ERROR_INTERNAL;
    }
}

// ============================================================================
// One-shot buffer decryption
// ============================================================================

onc_status onc_decrypt_buffer(
    const uint8_t* input,
    size_t input_len,
    const uint8_t* key,
    size_t key_len,
    onc_buffer* out_buf
) {
    clear_last_error();

    if (out_buf == nullptr) {
        set_last_error("Output buffer pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    reset_buffer(out_buf);

    if (input_len > 0 && input == nullptr) {
        set_last_error(
            "Input pointer is null while input length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (key_len > 0 && key == nullptr) {
        set_last_error(
            "Key pointer is null while key length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        const auto input_vector =
            make_vector(input, input_len);

        const auto password =
            make_string(key, key_len);

        const auto decrypted =
            crypto::decrypt(input_vector, password);

        return write_buffer(
            decrypted,
            out_buf
        );
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        reset_buffer(out_buf);
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        reset_buffer(out_buf);
        return map_decryption_exception(exception);
    } catch (...) {
        reset_buffer(out_buf);
        set_last_error(
            "Unknown internal decryption error."
        );
        return ONC_ERROR_INTERNAL;
    }
}

// ============================================================================
// File encryption
// ============================================================================

onc_status onc_encrypt_file(
    const char* src_path,
    const char* dst_path,
    const uint8_t* key,
    size_t key_len
) {
    clear_last_error();

    if (src_path == nullptr || dst_path == nullptr) {
        set_last_error("Source or destination path pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (key_len > 0 && key == nullptr) {
        set_last_error(
            "Key pointer is null while key length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        const auto password =
            make_string(key, key_len);

        const bool success =
            crypto::encryptFile(
                src_path,
                dst_path,
                password
            );

        if (!success) {
            set_last_error(
                "File encryption operation failed."
            );
            return ONC_ERROR_IO;
        }

        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        return map_encryption_exception(exception);
    } catch (...) {
        set_last_error(
            "Unknown internal file encryption error."
        );
        return ONC_ERROR_INTERNAL;
    }
}

// ============================================================================
// File decryption
// ============================================================================

onc_status onc_decrypt_file(
    const char* src_path,
    const char* dst_path,
    const uint8_t* key,
    size_t key_len
) {
    clear_last_error();

    if (src_path == nullptr || dst_path == nullptr) {
        set_last_error("Source or destination path pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (key_len > 0 && key == nullptr) {
        set_last_error(
            "Key pointer is null while key length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        const auto password =
            make_string(key, key_len);

        const bool success =
            crypto::decryptFile(
                src_path,
                dst_path,
                password
            );

        if (!success) {
            set_last_error(
                "File decryption operation failed."
            );
            return ONC_ERROR_IO;
        }

        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        return map_decryption_exception(exception);
    } catch (...) {
        set_last_error(
            "Unknown internal file decryption error."
        );
        return ONC_ERROR_INTERNAL;
    }
}

// ============================================================================
// Builder API
// ============================================================================

onc_builder_t onc_builder_create(void) {
    clear_last_error();

    try {
        auto* builder =
            new (std::nothrow) onc_builder_s();

        if (builder == nullptr) {
            set_last_error("Out of memory.");
        }

        return builder;
    } catch (...) {
        set_last_error(
            "Unknown exception during builder construction."
        );
        return nullptr;
    }
}

void onc_builder_destroy(
    onc_builder_t builder
) {
    delete builder;
}

onc_status onc_builder_set_key(
    onc_builder_t builder,
    const uint8_t* key,
    size_t key_len
) {
    clear_last_error();

    if (builder == nullptr) {
        set_last_error("Builder handle is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (key_len > 0 && key == nullptr) {
        set_last_error(
            "Key pointer is null while key length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        const auto password =
            make_string(key, key_len);

        builder->encryptor.password(password);
        builder->decryptor.password(password);

        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        set_last_error(exception.what());
        return ONC_ERROR_INTERNAL;
    } catch (...) {
        set_last_error(
            "Unknown error while setting builder key."
        );
        return ONC_ERROR_INTERNAL;
    }
}

onc_status onc_builder_set_algorithm(
    onc_builder_t builder,
    const char* algorithm_name
) {
    clear_last_error();

    if (builder == nullptr || algorithm_name == nullptr) {
        set_last_error(
            "Builder handle or algorithm name is null."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    const std::string name(algorithm_name);

    crypto::builder::Algorithm algorithm;

    if (name == "Auto") {
        algorithm = crypto::builder::Algorithm::Auto;
    } else if (name == "AES256_GCM") {
        algorithm = crypto::builder::Algorithm::AES256_GCM;
    } else if (name == "ChaCha20") {
        algorithm = crypto::builder::Algorithm::ChaCha20;
    } else if (name == "XChaCha20") {
        algorithm = crypto::builder::Algorithm::XChaCha20;
    } else {
        set_last_error(
            "Unsupported algorithm: " + name
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        builder->encryptor.algorithm(algorithm);
        builder->decryptor.algorithm(algorithm);

        return ONC_SUCCESS;
    } catch (const std::exception& exception) {
        set_last_error(exception.what());
        return ONC_ERROR_INTERNAL;
    } catch (...) {
        set_last_error(
            "Unknown error while setting builder algorithm."
        );
        return ONC_ERROR_INTERNAL;
    }
}

onc_status onc_builder_set_iterations(
    onc_builder_t builder,
    uint32_t iterations
) {
    clear_last_error();

    if (builder == nullptr) {
        set_last_error("Builder handle is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        builder->encryptor.iterations(
            static_cast<int>(iterations)
        );

        return ONC_SUCCESS;
    } catch (const std::exception& exception) {
        set_last_error(exception.what());
        return ONC_ERROR_INTERNAL;
    } catch (...) {
        set_last_error(
            "Unknown error while setting KDF iterations."
        );
        return ONC_ERROR_INTERNAL;
    }
}

onc_status onc_builder_encrypt(
    onc_builder_t builder,
    const uint8_t* input,
    size_t input_len,
    onc_buffer* out_buf
) {
    clear_last_error();

    if (builder == nullptr || out_buf == nullptr) {
        set_last_error(
            "Builder handle or output buffer pointer is null."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    reset_buffer(out_buf);

    if (input_len > 0 && input == nullptr) {
        set_last_error(
            "Input pointer is null while input length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        const auto input_vector =
            make_vector(input, input_len);

        const auto result =
            builder->encryptor.encrypt(input_vector);

        return write_buffer(
            result,
            out_buf
        );
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        return map_encryption_exception(exception);
    } catch (...) {
        set_last_error(
            "Unknown internal builder encryption error."
        );
        return ONC_ERROR_INTERNAL;
    }
}

// ============================================================================
// Streaming API
// ============================================================================
//
// Streaming format:
//
//   [16-byte salt]
//   [1-byte algorithm ID]
//
//   repeated frames:
//
//   [4-byte little-endian ciphertext size]
//   [16-byte authentication tag]
//   [ciphertext]
//
// Each update() call on an encryptor produces exactly one encrypted frame.
// The first update() additionally emits the stream header.
//
// Decryptors accept arbitrary byte boundaries. An update() call may contain:
//   - a partial header
//   - a complete header
//   - a partial frame
//   - multiple complete frames
//   - any combination of the above
//
// This makes the C ABI suitable for Rust, Python, Java, Kotlin and other
// bindings where input may arrive from arbitrary buffers or I/O callbacks.
//

onc_stream_t onc_stream_create_encryptor(
    const uint8_t* key,
    size_t key_len
) {
    clear_last_error();

    if (key_len > 0 && key == nullptr) {
        set_last_error(
            "Key pointer is null while key length is greater than zero."
        );
        return nullptr;
    }

    try {
        auto* stream =
            new (std::nothrow) onc_stream_s();

        if (stream == nullptr) {
            set_last_error("Out of memory.");
            return nullptr;
        }

        stream->key =
            make_string(key, key_len);

        stream->is_encrypt = true;
        stream->initialized = false;
        stream->finalized = false;
        stream->counter = 0;

        return stream;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return nullptr;
    } catch (const std::exception& exception) {
        set_last_error(exception.what());
        return nullptr;
    } catch (...) {
        set_last_error(
            "Unknown error while creating encryptor stream."
        );
        return nullptr;
    }
}

onc_stream_t onc_stream_create_decryptor(
    const uint8_t* key,
    size_t key_len
) {
    clear_last_error();

    if (key_len > 0 && key == nullptr) {
        set_last_error(
            "Key pointer is null while key length is greater than zero."
        );
        return nullptr;
    }

    try {
        auto* stream =
            new (std::nothrow) onc_stream_s();

        if (stream == nullptr) {
            set_last_error("Out of memory.");
            return nullptr;
        }

        stream->key =
            make_string(key, key_len);

        stream->is_encrypt = false;
        stream->initialized = false;
        stream->finalized = false;
        stream->counter = 0;

        return stream;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return nullptr;
    } catch (const std::exception& exception) {
        set_last_error(exception.what());
        return nullptr;
    } catch (...) {
        set_last_error(
            "Unknown error while creating decryptor stream."
        );
        return nullptr;
    }
}

// ============================================================================
// Streaming encryption
// ============================================================================

static onc_status stream_encrypt_update(
    onc_stream_t stream,
    const uint8_t* chunk,
    size_t chunk_len,
    onc_buffer* out_buf
) {
    if (chunk_len > STREAM_MAX_CHUNK_SIZE) {
        set_last_error(
            "Streaming chunk is too large."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        // ------------------------------------------------------------
        // Initialize stream cryptographic state on first update.
        // ------------------------------------------------------------

        if (!stream->initialized) {
            stream->salt =
                onc::core::backend::randomBytes(
                    STREAM_SALT_SIZE
                );

            stream->derived_key =
                onc::core::backend::deriveKey(
                    stream->key,
                    stream->salt,
                    STREAM_KEY_SIZE,
                    STREAM_KDF_ITERATIONS
                );

            stream->initialized = true;
        }

        // ------------------------------------------------------------
        // Build plaintext vector.
        // ------------------------------------------------------------

        const auto plaintext =
            make_vector(chunk, chunk_len);

        // ------------------------------------------------------------
        // Generate unique nonce.
        // ------------------------------------------------------------

        std::vector<unsigned char> nonce(
            STREAM_NONCE_SIZE
        );

        make_stream_nonce(
            nonce,
            stream->counter
        );

        // ------------------------------------------------------------
        // Encrypt one independent authenticated frame.
        // ------------------------------------------------------------

        const auto result =
            onc::core::backend::encrypt(
                plaintext,
                stream->derived_key,
                nonce,
                "ChaCha20-Poly1305"
            );

        if (
            result.ciphertext.size() >
            std::numeric_limits<uint32_t>::max()
        ) {
            set_last_error(
                "Encrypted chunk exceeds streaming frame size limit."
            );
            return ONC_ERROR_ENCRYPTION_FAILED;
        }

        const uint32_t ciphertext_size =
            static_cast<uint32_t>(
                result.ciphertext.size()
            );

        // ------------------------------------------------------------
        // Calculate output size.
        //
        // First frame additionally contains the stream header.
        // ------------------------------------------------------------

        const size_t header_size =
            stream->counter == 0
                ? STREAM_SALT_SIZE + 1
                : 0;

        const size_t frame_size =
            STREAM_FRAME_HEADER_SIZE +
            result.ciphertext.size();

        std::vector<unsigned char> output;

        output.reserve(
            header_size + frame_size
        );

        // ------------------------------------------------------------
        // Write stream header once.
        // ------------------------------------------------------------

        if (stream->counter == 0) {
            output.insert(
                output.end(),
                stream->salt.begin(),
                stream->salt.end()
            );

            output.push_back(
                STREAM_ALGORITHM_CHACHA20
            );
        }

        // ------------------------------------------------------------
        // Write frame header.
        // ------------------------------------------------------------

        const size_t frame_header_offset =
            output.size();

        output.resize(
            output.size() +
            sizeof(uint32_t)
        );

        write_u32_le(
            output.data() + frame_header_offset,
            ciphertext_size
        );

        // ------------------------------------------------------------
        // Write authentication tag.
        // ------------------------------------------------------------

        output.insert(
            output.end(),
            result.tag.begin(),
            result.tag.end()
        );

        // ------------------------------------------------------------
        // Write ciphertext.
        // ------------------------------------------------------------

        output.insert(
            output.end(),
            result.ciphertext.begin(),
            result.ciphertext.end()
        );

        ++stream->counter;

        return write_buffer(
            output,
            out_buf
        );
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        return map_encryption_exception(exception);
    } catch (...) {
        set_last_error(
            "Unknown internal streaming encryption error."
        );
        return ONC_ERROR_INTERNAL;
    }
}

// ============================================================================
// Streaming decryption
// ============================================================================

static onc_status stream_decrypt_update(
    onc_stream_t stream,
    const uint8_t* chunk,
    size_t chunk_len,
    onc_buffer* out_buf
) {
    try {
        // ------------------------------------------------------------
        // Append new bytes to the parser buffer.
        // ------------------------------------------------------------

        if (chunk_len > 0) {
            const size_t old_size =
                stream->pending.size();

            stream->pending.resize(
                old_size + chunk_len
            );

            std::memcpy(
                stream->pending.data() + old_size,
                chunk,
                chunk_len
            );
        }

        std::vector<unsigned char> plaintext_output;

        // ------------------------------------------------------------
        // Parse stream header.
        // ------------------------------------------------------------

        if (!stream->header_complete) {
            const size_t header_size =
                STREAM_SALT_SIZE + 1;

            const size_t available =
                stream->pending.size() -
                stream->pending_offset;

            if (available < header_size) {
                return write_buffer(
                    plaintext_output,
                    out_buf
                );
            }

            const unsigned char* header =
                stream->pending.data() +
                stream->pending_offset;

            stream->salt.assign(
                header,
                header + STREAM_SALT_SIZE
            );

            const uint8_t algorithm_id =
                header[STREAM_SALT_SIZE];

            if (
                algorithm_id !=
                STREAM_ALGORITHM_CHACHA20
            ) {
                set_last_error(
                    "Unsupported streaming algorithm."
                );
                return ONC_ERROR_INVALID_FORMAT;
            }

            stream->derived_key =
                onc::core::backend::deriveKey(
                    stream->key,
                    stream->salt,
                    STREAM_KEY_SIZE,
                    STREAM_KDF_ITERATIONS
                );

            stream->pending_offset +=
                header_size;

            stream->header_complete = true;
            stream->initialized = true;
        }

        // ------------------------------------------------------------
        // Parse complete frames.
        // ------------------------------------------------------------

        while (true) {
            const size_t available =
                stream->pending.size() -
                stream->pending_offset;

            // Need at least:
            //
            //   uint32 ciphertext size
            //   authentication tag
            //
            if (
                available <
                STREAM_FRAME_HEADER_SIZE
            ) {
                break;
            }

            const uint8_t* frame =
                stream->pending.data() +
                stream->pending_offset;

            const uint32_t ciphertext_size =
                read_u32_le(frame);

            if (ciphertext_size == 0) {
                set_last_error(
                    "Invalid zero-length encrypted frame."
                );
                return ONC_ERROR_INVALID_FORMAT;
            }

            if (
                ciphertext_size >
                STREAM_MAX_CHUNK_SIZE
            ) {
                set_last_error(
                    "Encrypted frame exceeds the maximum supported size."
                );
                return ONC_ERROR_INVALID_FORMAT;
            }

            const size_t complete_frame_size =
                STREAM_FRAME_HEADER_SIZE +
                static_cast<size_t>(ciphertext_size);

            // The complete frame has not arrived yet.
            if (available < complete_frame_size) {
                break;
            }

            // --------------------------------------------------------
            // Extract authentication tag.
            // --------------------------------------------------------

            std::vector<unsigned char> tag(
                STREAM_TAG_SIZE
            );

            std::memcpy(
                tag.data(),
                frame + sizeof(uint32_t),
                STREAM_TAG_SIZE
            );

            // --------------------------------------------------------
            // Extract ciphertext.
            // --------------------------------------------------------

            const uint8_t* ciphertext_ptr =
                frame +
                STREAM_FRAME_HEADER_SIZE;

            std::vector<unsigned char> ciphertext(
                ciphertext_ptr,
                ciphertext_ptr + ciphertext_size
            );

            // --------------------------------------------------------
            // Generate the nonce corresponding to this frame.
            // --------------------------------------------------------

            std::vector<unsigned char> nonce(
                STREAM_NONCE_SIZE
            );

            make_stream_nonce(
                nonce,
                stream->counter
            );

            // --------------------------------------------------------
            // Authenticate and decrypt.
            //
            // IMPORTANT:
            // Plaintext is appended to the output only after the backend
            // successfully authenticates the frame.
            // --------------------------------------------------------

            const auto plaintext =
                onc::core::backend::decrypt(
                    ciphertext,
                    stream->derived_key,
                    nonce,
                    tag,
                    "ChaCha20-Poly1305"
                );

            plaintext_output.insert(
                plaintext_output.end(),
                plaintext.begin(),
                plaintext.end()
            );

            ++stream->counter;

            stream->pending_offset +=
                complete_frame_size;
        }

        // ------------------------------------------------------------
        // Compact parser buffer.
        //
        // Avoid keeping already-consumed frames in memory forever.
        // ------------------------------------------------------------

        if (stream->pending_offset > 0) {
            const size_t remaining =
                stream->pending.size() -
                stream->pending_offset;

            if (remaining == 0) {
                stream->pending.clear();
                stream->pending_offset = 0;
            } else if (
                stream->pending_offset >
                stream->pending.size() / 2
            ) {
                std::vector<unsigned char> remaining_data(
                    stream->pending.begin() +
                        static_cast<std::ptrdiff_t>(
                            stream->pending_offset
                        ),
                    stream->pending.end()
                );

                stream->pending.swap(
                    remaining_data
                );

                stream->pending_offset = 0;
            }
        }

        return write_buffer(
            plaintext_output,
            out_buf
        );
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        return map_decryption_exception(exception);
    } catch (...) {
        set_last_error(
            "Unknown internal streaming decryption error."
        );
        return ONC_ERROR_INTERNAL;
    }
}

// ============================================================================
// Generic stream update
// ============================================================================

onc_status onc_stream_update(
    onc_stream_t stream,
    const uint8_t* chunk,
    size_t chunk_len,
    onc_buffer* out_buf
) {
    clear_last_error();

    if (out_buf == nullptr) {
        set_last_error(
            "Output buffer pointer is null."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    reset_buffer(out_buf);

    if (stream == nullptr) {
        set_last_error(
            "Stream handle is null."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (stream->finalized) {
        set_last_error(
            "Stream has already been finalized."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (chunk_len > 0 && chunk == nullptr) {
        set_last_error(
            "Chunk pointer is null while chunk length is greater than zero."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (stream->is_encrypt) {
        return stream_encrypt_update(
            stream,
            chunk,
            chunk_len,
            out_buf
        );
    }

    return stream_decrypt_update(
        stream,
        chunk,
        chunk_len,
        out_buf
    );
}

// ============================================================================
// Stream finalization
// ============================================================================
//
// Encryption:
//   - If no update() has occurred, final() emits a valid empty stream header.
//   - Otherwise it emits an empty buffer.
//
// Decryption:
//   - The stream must have received and parsed the complete header.
//   - No incomplete frame may remain.
//
// No additional authentication frame is generated because each streaming
// chunk is independently authenticated by ChaCha20-Poly1305.
//

onc_status onc_stream_final(
    onc_stream_t stream,
    onc_buffer* out_buf
) {
    clear_last_error();

    if (out_buf == nullptr) {
        set_last_error(
            "Output buffer pointer is null."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    reset_buffer(out_buf);

    if (stream == nullptr) {
        set_last_error(
            "Stream handle is null."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    if (stream->finalized) {
        set_last_error(
            "Stream has already been finalized."
        );
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        if (stream->is_encrypt) {
            // --------------------------------------------------------
            // Empty input stream.
            //
            // EncryptStream writes its header even when the input file
            // contains zero bytes. Preserve the same behavior here.
            // --------------------------------------------------------

            if (!stream->initialized) {
                stream->salt =
                    onc::core::backend::randomBytes(
                        STREAM_SALT_SIZE
                    );

                stream->derived_key =
                    onc::core::backend::deriveKey(
                        stream->key,
                        stream->salt,
                        STREAM_KEY_SIZE,
                        STREAM_KDF_ITERATIONS
                    );

                stream->initialized = true;

                std::vector<unsigned char> header;

                header.reserve(
                    STREAM_SALT_SIZE + 1
                );

                header.insert(
                    header.end(),
                    stream->salt.begin(),
                    stream->salt.end()
                );

                header.push_back(
                    STREAM_ALGORITHM_CHACHA20
                );

                stream->finalized = true;

                return write_buffer(
                    header,
                    out_buf
                );
            }

            stream->finalized = true;

            return ONC_SUCCESS;
        }

        // ------------------------------------------------------------
        // Decryption finalization.
        // ------------------------------------------------------------

        if (!stream->header_complete) {
            set_last_error(
                "Incomplete streaming header."
            );
            return ONC_ERROR_INVALID_FORMAT;
        }

        const size_t remaining =
            stream->pending.size() -
            stream->pending_offset;

        if (remaining != 0) {
            set_last_error(
                "Incomplete encrypted streaming frame."
            );
            return ONC_ERROR_INVALID_FORMAT;
        }

        stream->finalized = true;

        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory.");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& exception) {
        set_last_error(exception.what());
        return ONC_ERROR_INTERNAL;
    } catch (...) {
        set_last_error(
            "Unknown error while finalizing stream."
        );
        return ONC_ERROR_INTERNAL;
    }
}

// ============================================================================
// Stream destruction
// ============================================================================

void onc_stream_destroy(
    onc_stream_t stream
) {
    delete stream;
}