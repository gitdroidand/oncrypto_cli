#include "oncrypto/c_abi/c_abi.h"
#include "oncrypto/oncrypto.hpp"

#include <vector>
#include <string>
#include <cstring>
#include <thread>

// نگهداری آخرین پیام خطا به صورت Thread-Local
static thread_local std::string g_last_error = "";

static void set_last_error(const std::string& err) {
    g_last_error = err;
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
    static std::string ver = crypto::getVersion();
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

onc_status onc_encrypt_buffer(
    const uint8_t* input,
    size_t input_len,
    const uint8_t* key,
    size_t key_len,
    onc_buffer* out_buf
) {
    if (!input || !key || !out_buf) {
        set_last_error("Invalid null pointer argument.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::vector<unsigned char> in_vec(input, input + input_len);
        std::string pwd(reinterpret_cast<const char*>(key), key_len);

        std::vector<unsigned char> encrypted = crypto::encrypt(in_vec, pwd);

        out_buf->data = new uint8_t[encrypted.size()];
        out_buf->size = encrypted.size();
        std::memcpy(out_buf->data, encrypted.data(), encrypted.size());

        return ONC_SUCCESS;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_ENCRYPTION_FAILED;
    } catch (...) {
        set_last_error("Unknown internal encryption error.");
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
    if (!input || !key || !out_buf) {
        set_last_error("Invalid null pointer argument.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::vector<unsigned char> in_vec(input, input + input_len);
        std::string pwd(reinterpret_cast<const char*>(key), key_len);

        std::vector<unsigned char> decrypted = crypto::decrypt(in_vec, pwd);

        out_buf->data = new uint8_t[decrypted.size()];
        out_buf->size = decrypted.size();
        std::memcpy(out_buf->data, decrypted.data(), decrypted.size());

        return ONC_SUCCESS;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_DECRYPTION_FAILED;
    } catch (...) {
        set_last_error("Unknown internal decryption error.");
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
    if (!src_path || !dst_path || !key) {
        set_last_error("Invalid null pointer argument.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::string pwd(reinterpret_cast<const char*>(key), key_len);
        bool ok = crypto::encryptFile(src_path, dst_path, pwd);
        return ok ? ONC_SUCCESS : ONC_ERROR_IO;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_ENCRYPTION_FAILED;
    }
}

onc_status onc_decrypt_file(
    const char* src_path,
    const char* dst_path,
    const uint8_t* key,
    size_t key_len
) {
    if (!src_path || !dst_path || !key) {
        set_last_error("Invalid null pointer argument.");
        return ONC_ERROR_INVALID_ARGUMENT;
    }

    try {
        std::string pwd(reinterpret_cast<const char*>(key), key_len);
        bool ok = crypto::decryptFile(src_path, dst_path, pwd);
        return ok ? ONC_SUCCESS : ONC_ERROR_IO;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_DECRYPTION_FAILED;
    }
}

// =========================================================================
// Builder Pattern APIs
// =========================================================================

onc_builder_t onc_builder_create(void) {
    return new (std::nothrow) onc_builder_s();
}

onc_status onc_builder_set_key(onc_builder_t builder, const uint8_t* key, size_t key_len) {
    if (!builder || !key) return ONC_ERROR_INVALID_ARGUMENT;
    std::string pwd(reinterpret_cast<const char*>(key), key_len);
    builder->encryptor.password(pwd);
    builder->decryptor.password(pwd);
    return ONC_SUCCESS;
}

onc_status onc_builder_set_algorithm(onc_builder_t builder, const char* algo_name) {
    if (!builder || !algo_name) return ONC_ERROR_INVALID_ARGUMENT;
    
    crypto::builder::Algorithm algo = crypto::builder::Algorithm::Auto;
    std::string name(algo_name);
    if (name == "AES256_GCM") algo = crypto::builder::Algorithm::AES256_GCM;
    else if (name == "ChaCha20") algo = crypto::builder::Algorithm::ChaCha20;
    else if (name == "XChaCha20") algo = crypto::builder::Algorithm::XChaCha20;

    builder->encryptor.algorithm(algo);
    builder->decryptor.algorithm(algo);
    return ONC_SUCCESS;
}

onc_status onc_builder_set_iterations(onc_builder_t builder, uint32_t iterations) {
    if (!builder) return ONC_ERROR_INVALID_ARGUMENT;
    builder->encryptor.iterations(static_cast<int>(iterations));
    return ONC_SUCCESS;
}

onc_status onc_builder_encrypt(
    onc_builder_t builder,
    const uint8_t* input,
    size_t input_len,
    onc_buffer* out_buf
) {
    if (!builder || !input || !out_buf) return ONC_ERROR_INVALID_ARGUMENT;

    try {
        std::vector<unsigned char> in_vec(input, input + input_len);
        std::vector<unsigned char> res = builder->encryptor.encrypt(in_vec);

        out_buf->data = new uint8_t[res.size()];
        out_buf->size = res.size();
        std::memcpy(out_buf->data, res.data(), res.size());

        return ONC_SUCCESS;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return ONC_ERROR_ENCRYPTION_FAILED;
    }
}

void onc_builder_destroy(onc_builder_t builder) {
    delete builder;
}

// =========================================================================
// Streaming APIs
// =========================================================================

onc_stream_t onc_stream_create_encryptor(const uint8_t* key, size_t key_len) {
    if (!key) return nullptr;
    auto* st = new (std::nothrow) onc_stream_s();
    if (st) {
        st->key = std::string(reinterpret_cast<const char*>(key), key_len);
        st->is_encrypt = true;
    }
    return st;
}

onc_stream_t onc_stream_create_decryptor(const uint8_t* key, size_t key_len) {
    if (!key) return nullptr;
    auto* st = new (std::nothrow) onc_stream_s();
    if (st) {
        st->key = std::string(reinterpret_cast<const char*>(key), key_len);
        st->is_encrypt = false;
    }
    return st;
}

onc_status onc_stream_update(
    onc_stream_t stream,
    const uint8_t* chunk,
    size_t chunk_len,
    onc_buffer* out_buf
) {
    if (!stream || !chunk || !out_buf) return ONC_ERROR_INVALID_ARGUMENT;
    
    // Stub implementation for stream buffer updates
    out_buf->data = nullptr;
    out_buf->size = 0;
    return ONC_SUCCESS;
}

onc_status onc_stream_final(onc_stream_t stream, onc_buffer* out_buf) {
    if (!stream || !out_buf) return ONC_ERROR_INVALID_ARGUMENT;

    out_buf->data = nullptr;
    out_buf->size = 0;
    return ONC_SUCCESS;
}

void onc_stream_destroy(onc_stream_t stream) {
    delete stream;
}