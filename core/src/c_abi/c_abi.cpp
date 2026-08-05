#include "oncrypto/c_abi/c_abi.h"
#include "oncrypto/oncrypto.hpp"

#include <vector>
#include <string>
#include <cstring>
#include <new>        // for std::bad_alloc

// =========================================================================
// Thread‑local last error message
// =========================================================================

static thread_local std::string g_last_error = "";

static void clear_last_error() {
    g_last_error.clear();
}

static void set_last_error(const std::string& err) {
    g_last_error = err;
}

// =========================================================================
// Helper: reset onc_buffer to a safe state
// =========================================================================

static void reset_buffer(onc_buffer* buf) {
    if (buf) {
        buf->data = nullptr;
        buf->size = 0;
    }
}

// =========================================================================
// Opaque Handles Struct Definitions
// =========================================================================

struct onc_builder_s {
    crypto::builder::Encryptor encryptor;
    crypto::builder::Decryptor decryptor;
    std::vector<uint8_t> key;
    bool is_encrypt = true;
};

struct onc_stream_s {
    std::string key;
    bool is_encrypt = true;
};

// =========================================================================
// Metadata & Memory Utilities
// =========================================================================

const char* onc_version(void) {
    // Thread‑safe static local initialization (C++11 guarantees it)
    static const std::string ver = []() -> std::string {
        try {
            return crypto::getVersion();
        } catch (...) {
            return "unknown";
        }
    }();
    return ver.c_str();
}

const char* onc_status_to_string(onc_status status) {
    switch (status) {
        case ONC_SUCCESS:                  return "Success";
        case ONC_ERROR_INVALID_ARGUMENT:   return "Invalid Argument";
        case ONC_ERROR_ENCRYPTION_FAILED:  return "Encryption Failed";
        case ONC_ERROR_DECRYPTION_FAILED:  return "Decryption Failed";
        case ONC_ERROR_AUTHENTICATION_FAILED: return "Authentication Failed";
        case ONC_ERROR_INVALID_FORMAT:     return "Invalid Format";
        case ONC_ERROR_IO:                 return "I/O Error";
        case ONC_ERROR_OUT_OF_MEMORY:      return "Out of Memory";
        case ONC_ERROR_INTERNAL:           return "Internal Error";
        default:                           return "Unknown Error";
    }
}

const char* onc_get_last_error(void) {
    return g_last_error.c_str();
}

void onc_buffer_free(onc_buffer* buf) {
    if (buf && buf->data) {
        delete[] buf->data;
        buf->data = nullptr;
        buf->size = 0;
    }
}

void onc_string_free(onc_string* str) {
    if (str && str->str) {
        delete[] str->str;
        str->str = nullptr;
        str->length = 0;
    }
}

// =========================================================================
// Buffer Encryption & Decryption
// =========================================================================

static std::vector<unsigned char> make_vector(const uint8_t* data, size_t len) {
    std::vector<unsigned char> vec;
    vec.resize(len);
    if (len > 0 && data != nullptr) {
        std::memcpy(vec.data(), data, len);
    }
    return vec;
}

static std::string make_string(const uint8_t* data, size_t len) {
    if (data == nullptr && len == 0) {
        return std::string();
    }
    return std::string(reinterpret_cast<const char*>(data), len);
}

onc_status onc_encrypt_buffer(
    const uint8_t* input,
    size_t input_len,
    const uint8_t* key,
    size_t key_len,
    onc_buffer* out_buf
) {
    clear_last_error();
    reset_buffer(out_buf);

    if (out_buf == nullptr) {
        set_last_error("Output buffer pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (input_len > 0 && input == nullptr) {
        set_last_error("Input pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (key_len > 0 && key == nullptr) {
        set_last_error("Key pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::vector<unsigned char> in_vec = make_vector(input, input_len);
        std::string pwd = make_string(key, key_len);

        std::vector<unsigned char> encrypted = crypto::encrypt(in_vec, pwd);

        out_buf->data = new uint8_t[encrypted.size()];
        out_buf->size = encrypted.size();
        std::memcpy(out_buf->data, encrypted.data(), encrypted.size());

        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory");
        reset_buffer(out_buf);
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        reset_buffer(out_buf);
        return ONC_ERROR_ENCRYPTION_FAILED;
    } catch (...) {
        set_last_error("Unknown internal encryption error.");
        reset_buffer(out_buf);
        return ONC_ERROR_INTERNAL;
    }
}

onc_status onc_decrypt_buffer(
    const uint8_t* input,
    size_t input_len,
    const uint8_t* key,
    size_t key_len,
    onc_buffer* out_buf
) {
    clear_last_error();
    reset_buffer(out_buf);

    if (out_buf == nullptr) {
        set_last_error("Output buffer pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (input_len > 0 && input == nullptr) {
        set_last_error("Input pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (key_len > 0 && key == nullptr) {
        set_last_error("Key pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::vector<unsigned char> in_vec = make_vector(input, input_len);
        std::string pwd = make_string(key, key_len);

        std::vector<unsigned char> decrypted = crypto::decrypt(in_vec, pwd);

        out_buf->data = new uint8_t[decrypted.size()];
        out_buf->size = decrypted.size();
        std::memcpy(out_buf->data, decrypted.data(), decrypted.size());

        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory");
        reset_buffer(out_buf);
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        reset_buffer(out_buf);
        return ONC_ERROR_DECRYPTION_FAILED;
    } catch (...) {
        set_last_error("Unknown internal decryption error.");
        reset_buffer(out_buf);
        return ONC_ERROR_INTERNAL;
    }
}

// =========================================================================
// File Encryption & Decryption
// =========================================================================

onc_status onc_encrypt_file(
    const char* src_path,
    const char* dst_path,
    const uint8_t* key,
    size_t key_len
) {
    clear_last_error();

    if (!src_path || !dst_path) {
        set_last_error("File path pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (key_len > 0 && key == nullptr) {
        set_last_error("Key pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::string pwd = make_string(key, key_len);
        bool ok = crypto::encryptFile(src_path, dst_path, pwd);
        if (!ok) {
            set_last_error("File encryption operation failed (I/O error).");
            return ONC_ERROR_IO;
        }
        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_ENCRYPTION_FAILED;
    } catch (...) {
        set_last_error("Unknown internal file encryption error.");
        return ONC_ERROR_INTERNAL;
    }
}

onc_status onc_decrypt_file(
    const char* src_path,
    const char* dst_path,
    const uint8_t* key,
    size_t key_len
) {
    clear_last_error();

    if (!src_path || !dst_path) {
        set_last_error("File path pointer is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (key_len > 0 && key == nullptr) {
        set_last_error("Key pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::string pwd = make_string(key, key_len);
        bool ok = crypto::decryptFile(src_path, dst_path, pwd);
        if (!ok) {
            set_last_error("File decryption operation failed (I/O error).");
            return ONC_ERROR_IO;
        }
        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_DECRYPTION_FAILED;
    } catch (...) {
        set_last_error("Unknown internal file decryption error.");
        return ONC_ERROR_INTERNAL;
    }
}

// =========================================================================
// Builder Pattern APIs
// =========================================================================

onc_builder_t onc_builder_create(void) {
    clear_last_error();
    onc_builder_t builder = nullptr;
    try {
        builder = new (std::nothrow) onc_builder_s();
        if (!builder) {
            set_last_error("Out of memory");
        }
    } catch (...) {
        set_last_error("Unknown exception during builder construction");
        // In case of exception, ensure we return null
        return nullptr;
    }
    return builder;
}

onc_status onc_builder_set_key(onc_builder_t builder, const uint8_t* key, size_t key_len) {
    clear_last_error();
    if (!builder) {
        set_last_error("Builder handle is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (key_len > 0 && key == nullptr) {
        set_last_error("Key pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::string pwd = make_string(key, key_len);
        builder->encryptor.password(pwd);
        builder->decryptor.password(pwd);
        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory");
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_INTERNAL;
    } catch (...) {
        set_last_error("Unknown error setting key");
        return ONC_ERROR_INTERNAL;
    }
}

onc_status onc_builder_set_algorithm(onc_builder_t builder, const char* algo_name) {
    clear_last_error();
    if (!builder || !algo_name) {
        set_last_error("Builder handle or algorithm name is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    std::string name(algo_name);
    crypto::builder::Algorithm algo;
    if (name == "Auto") {
        algo = crypto::builder::Algorithm::Auto;
    } else if (name == "AES256_GCM") {
        algo = crypto::builder::Algorithm::AES256_GCM;
    } else if (name == "ChaCha20") {
        algo = crypto::builder::Algorithm::ChaCha20;
    } else if (name == "XChaCha20") {
        algo = crypto::builder::Algorithm::XChaCha20;
    } else {
        set_last_error("Unsupported algorithm: " + name);
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        builder->encryptor.algorithm(algo);
        builder->decryptor.algorithm(algo);
        return ONC_SUCCESS;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_INTERNAL;
    } catch (...) {
        set_last_error("Unknown error setting algorithm");
        return ONC_ERROR_INTERNAL;
    }
}

onc_status onc_builder_set_iterations(onc_builder_t builder, uint32_t iterations) {
    clear_last_error();
    if (!builder) {
        set_last_error("Builder handle is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    try {
        builder->encryptor.iterations(static_cast<int>(iterations));
        return ONC_SUCCESS;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_INTERNAL;
    } catch (...) {
        set_last_error("Unknown error setting iterations");
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
    reset_buffer(out_buf);

    if (!builder || !out_buf) {
        set_last_error("Builder or output buffer handle is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (input_len > 0 && input == nullptr) {
        set_last_error("Input pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::vector<unsigned char> in_vec = make_vector(input, input_len);
        std::vector<unsigned char> res = builder->encryptor.encrypt(in_vec);

        out_buf->data = new uint8_t[res.size()];
        out_buf->size = res.size();
        std::memcpy(out_buf->data, res.data(), res.size());

        return ONC_SUCCESS;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory");
        reset_buffer(out_buf);
        return ONC_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        reset_buffer(out_buf);
        return ONC_ERROR_ENCRYPTION_FAILED;
    } catch (...) {
        set_last_error("Unknown internal builder encryption error.");
        reset_buffer(out_buf);
        return ONC_ERROR_INTERNAL;
    }
}

void onc_builder_destroy(onc_builder_t builder) {
    delete builder;   // safe with null
}

// =========================================================================
// Streaming APIs
// =========================================================================

onc_stream_t onc_stream_create_encryptor(const uint8_t* key, size_t key_len) {
    clear_last_error();
    if (key_len > 0 && key == nullptr) {
        set_last_error("Key pointer is null but length > 0.");
        return nullptr;
    }

    onc_stream_t st = nullptr;
    try {
        st = new (std::nothrow) onc_stream_s();
        if (!st) {
            set_last_error("Out of memory");
            return nullptr;
        }
        st->key = make_string(key, key_len);
        st->is_encrypt = true;
        return st;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory");
        delete st;
        return nullptr;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        delete st;
        return nullptr;
    } catch (...) {
        set_last_error("Unknown error creating encryptor");
        delete st;
        return nullptr;
    }
}

onc_stream_t onc_stream_create_decryptor(const uint8_t* key, size_t key_len) {
    clear_last_error();
    if (key_len > 0 && key == nullptr) {
        set_last_error("Key pointer is null but length > 0.");
        return nullptr;
    }

    onc_stream_t st = nullptr;
    try {
        st = new (std::nothrow) onc_stream_s();
        if (!st) {
            set_last_error("Out of memory");
            return nullptr;
        }
        st->key = make_string(key, key_len);
        st->is_encrypt = false;
        return st;
    } catch (const std::bad_alloc&) {
        set_last_error("Out of memory");
        delete st;
        return nullptr;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        delete st;
        return nullptr;
    } catch (...) {
        set_last_error("Unknown error creating decryptor");
        delete st;
        return nullptr;
    }
}

onc_status onc_stream_update(
    onc_stream_t stream,
    const uint8_t* chunk,
    size_t chunk_len,
    onc_buffer* out_buf
) {
    clear_last_error();
    reset_buffer(out_buf);

    if (!stream || !out_buf) {
        set_last_error("Stream or output buffer handle is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }
    if (chunk_len > 0 && chunk == nullptr) {
        set_last_error("Chunk pointer is null but length > 0.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    // Streaming is not implemented in the underlying C++ library.
    // We return a clear error instead of faking success.
    set_last_error("Streaming API is not implemented in this version.");
    return ONC_ERROR_INTERNAL;
}

onc_status onc_stream_final(
    onc_stream_t stream,
    onc_buffer* out_buf
) {
    clear_last_error();
    reset_buffer(out_buf);

    if (!stream || !out_buf) {
        set_last_error("Stream or output buffer handle is null.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    set_last_error("Streaming API is not implemented in this version.");
    return ONC_ERROR_INTERNAL;
}

void onc_stream_destroy(onc_stream_t stream) {
    delete stream;   // safe with null
}