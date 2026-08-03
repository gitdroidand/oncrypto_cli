#include "oncrypto/backend/Backend.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/core_names.h>
#include <stdexcept>
#include <cstring>

namespace onc::core::backend {

#include <vector>

// Forward declarations for internal impl_ functions so C ABI wrappers can call them.
std::vector<unsigned char> onc_core_backend_impl_randomBytes(size_t size);
std::vector<unsigned char> onc_core_backend_impl_deriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
);
static std::vector<unsigned char> onc_core_backend_impl_xchacha20ToChacha20Nonce(
    const std::vector<unsigned char>& xnonce,
    const std::vector<unsigned char>& key
);
static const EVP_CIPHER* onc_core_backend_impl_getCipher(const std::string& algorithm);
EncryptResult onc_core_backend_impl_encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
);
std::vector<unsigned char> onc_core_backend_impl_decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
);

// Export C-ABI symbols for engine. These call the internal impl_ functions above.
#include "oncrypto_engine.h"

extern "C" {

ONCRYPTO_ENGINE_API unsigned int oncrypto_engine_version_major(void) {
    return ONCRYPTO_ENGINE_VERSION_MAJOR;
}

ONCRYPTO_ENGINE_API unsigned int oncrypto_engine_version_minor(void) {
    return ONCRYPTO_ENGINE_VERSION_MINOR;
}

ONCRYPTO_ENGINE_API const char* oncrypto_engine_version_string(void) {
    return "1.0.0";
}

ONCRYPTO_ENGINE_API int oncrypto_engine_random_bytes(unsigned char* out, size_t out_len) {
    try {
        auto v = onc_core_backend_impl_randomBytes(out_len);
        std::memcpy(out, v.data(), out_len);
        return 0;
    } catch (...) {
        return 1;
    }
}

ONCRYPTO_ENGINE_API int oncrypto_engine_pbkdf2_hmac_sha256(
    const char* password,
    const unsigned char* salt,
    size_t salt_len,
    unsigned int iterations,
    unsigned char* out,
    size_t out_len
) {
    try {
        std::string pw(password ? password : "");
        std::vector<unsigned char> s(salt, salt + salt_len);
        auto key = onc_core_backend_impl_deriveKey(pw, s, out_len, iterations);
        std::memcpy(out, key.data(), out_len);
        return 0;
    } catch (...) {
        return 1;
    }
}

ONCRYPTO_ENGINE_API int oncrypto_engine_aead_encrypt(
    const char* algorithm,
    const unsigned char* key,
    size_t key_len,
    const unsigned char* nonce,
    size_t nonce_len,
    const unsigned char* plaintext,
    size_t plaintext_len,
    unsigned char* ciphertext,
    size_t* ciphertext_len,
    unsigned char* tag,
    size_t* tag_len
) {
    try {
        std::vector<unsigned char> kp(key, key + key_len);
        std::vector<unsigned char> nn(nonce, nonce + nonce_len);
        std::vector<unsigned char> pt(plaintext, plaintext + plaintext_len);
        auto res = onc_core_backend_impl_encrypt(pt, kp, nn, std::string(algorithm));
        if (*ciphertext_len < res.ciphertext.size()) return 2;
        if (*tag_len < res.tag.size()) return 3;
        std::memcpy(ciphertext, res.ciphertext.data(), res.ciphertext.size());
        std::memcpy(tag, res.tag.data(), res.tag.size());
        *ciphertext_len = res.ciphertext.size();
        *tag_len = res.tag.size();
        return 0;
    } catch (...) {
        return 1;
    }
}

ONCRYPTO_ENGINE_API int oncrypto_engine_aead_decrypt(
    const char* algorithm,
    const unsigned char* key,
    size_t key_len,
    const unsigned char* nonce,
    size_t nonce_len,
    const unsigned char* ciphertext,
    size_t ciphertext_len,
    const unsigned char* tag,
    size_t tag_len,
    unsigned char* plaintext,
    size_t* plaintext_len
) {
    try {
        std::vector<unsigned char> kp(key, key + key_len);
        std::vector<unsigned char> nn(nonce, nonce + nonce_len);
        std::vector<unsigned char> ct(ciphertext, ciphertext + ciphertext_len);
        std::vector<unsigned char> tg(tag, tag + tag_len);
        auto res = onc_core_backend_impl_decrypt(ct, kp, nn, tg, std::string(algorithm));
        if (*plaintext_len < res.size()) return 2;
        std::memcpy(plaintext, res.data(), res.size());
        *plaintext_len = res.size();
        return 0;
    } catch (...) {
        return 1;
    }
}

ONCRYPTO_ENGINE_API int oncrypto_engine_hmac_sha256(
    const unsigned char* key,
    size_t key_len,
    const unsigned char* data,
    size_t data_len,
    unsigned char* out,
    size_t* out_len
) {
    try {
        EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
        if (!mac) return 2;
        EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
        if (!ctx) { EVP_MAC_free(mac); return 2; }
        OSSL_PARAM params[] = { OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, (char*)"SHA256", 0), OSSL_PARAM_construct_end() };
        if (EVP_MAC_init(ctx, key, key_len, params) != 1) { EVP_MAC_CTX_free(ctx); EVP_MAC_free(mac); return 2; }
        if (EVP_MAC_update(ctx, data, data_len) != 1) { EVP_MAC_CTX_free(ctx); EVP_MAC_free(mac); return 2; }
        size_t olen = *out_len;
        if (EVP_MAC_final(ctx, out, &olen, *out_len) != 1) { EVP_MAC_CTX_free(ctx); EVP_MAC_free(mac); return 2; }
        *out_len = olen;
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return 0;
    } catch (...) {
        return 1;
    }
}

} // extern "C"
// ============================================================
// Random
// ============================================================

// Internal implementation used by engine C-ABI wrapper.
std::vector<unsigned char> onc_core_backend_impl_randomBytes(size_t size) {
    std::vector<unsigned char> bytes(size);
    if (RAND_bytes(bytes.data(), size) != 1) {
        throw std::runtime_error("Backend: Failed to generate random bytes");
    }
    return bytes;
}

// ============================================================
// Key Derivation (PBKDF2)
// ============================================================

std::vector<unsigned char> onc_core_backend_impl_deriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
) {
    std::vector<unsigned char> key(keySize);
    
    if (PKCS5_PBKDF2_HMAC(
        password.c_str(),
        password.size(),
        salt.data(),
        salt.size(),
        iterations,
        EVP_sha256(),
        keySize,
        key.data()
    ) != 1) {
        throw std::runtime_error("Backend: Key derivation failed");
    }
    
    return key;
}

// ============================================================
// XChaCha20: Convert 24-byte nonce to 12-byte nonce using EVP_MAC
// ============================================================

static std::vector<unsigned char> onc_core_backend_impl_xchacha20ToChacha20Nonce(
    const std::vector<unsigned char>& xnonce,
    const std::vector<unsigned char>& key
) {
    std::vector<unsigned char> derivedNonce(12);
    
    EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if (!mac) {
        throw std::runtime_error("Backend: Failed to fetch HMAC");
    }
    
    EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
    if (!ctx) {
        EVP_MAC_free(mac);
        throw std::runtime_error("Backend: Failed to create MAC context");
    }
    
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST,
                                         (char*)"SHA256", 0),
        OSSL_PARAM_construct_end()
    };
    
    if (EVP_MAC_init(ctx, key.data(), key.size(), params) != 1) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        throw std::runtime_error("Backend: Failed to init HMAC");
    }
    
    if (EVP_MAC_update(ctx, xnonce.data(), xnonce.size()) != 1) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        throw std::runtime_error("Backend: Failed to update HMAC");
    }
    
    size_t outLen = 32;
    std::vector<unsigned char> hmac(32);
    if (EVP_MAC_final(ctx, hmac.data(), &outLen, hmac.size()) != 1) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        throw std::runtime_error("Backend: Failed to finalize HMAC");
    }
    
    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    
    std::memcpy(derivedNonce.data(), hmac.data(), 12);
    return derivedNonce;
}

// ============================================================
// Get Cipher
// ============================================================

static const EVP_CIPHER* onc_core_backend_impl_getCipher(const std::string& algorithm) {
    if (algorithm == "AES-256-GCM") {
        return EVP_aes_256_gcm();
    }
    if (algorithm == "ChaCha20-Poly1305" || algorithm == "XChaCha20-Poly1305") {
        return EVP_chacha20_poly1305();
    }
    throw std::runtime_error("Backend: Unknown algorithm");
}

// ============================================================
// Encrypt
// ============================================================

EncryptResult onc_core_backend_impl_encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
) {
    const EVP_CIPHER* cipher = onc_core_backend_impl_getCipher(algorithm);
    size_t keySize = EVP_CIPHER_key_length(cipher);
    size_t tagSize = 16;
    
    if (key.size() != keySize) {
        throw std::runtime_error("Backend: Invalid key size");
    }
    
    std::vector<unsigned char> actualNonce = nonce;
    if (algorithm == "XChaCha20-Poly1305") {
        if (nonce.size() != 24) {
            throw std::runtime_error("Backend: XChaCha20 requires 24-byte nonce");
        }
        actualNonce = onc_core_backend_impl_xchacha20ToChacha20Nonce(nonce, key);
    } else {
        if (nonce.size() != 12 && nonce.size() != 24) {
            throw std::runtime_error("Backend: Invalid nonce size");
        }
        if (nonce.size() == 24) {
            actualNonce.resize(12);
            std::memcpy(actualNonce.data(), nonce.data(), 12);
        }
    }
    
    std::vector<unsigned char> ciphertext(plaintext.size());
    std::vector<unsigned char> tag(tagSize);
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Backend: Failed to create cipher context");
    }
    
    int outlen = 0, tmplen = 0;
    
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, actualNonce.size(), nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), actualNonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Backend: Encryption init failed");
    }
    
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &outlen, plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Backend: Encryption update failed");
    }
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + outlen, &tmplen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Backend: Encryption final failed");
    }
    
    outlen += tmplen;
    ciphertext.resize(outlen);
    
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Backend: Failed to get tag");
    }
    
    EVP_CIPHER_CTX_free(ctx);
    
    return {ciphertext, tag};
}

// ============================================================
// Decrypt
// ============================================================

std::vector<unsigned char> onc_core_backend_impl_decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
) {
    const EVP_CIPHER* cipher = onc_core_backend_impl_getCipher(algorithm);
    size_t keySize = EVP_CIPHER_key_length(cipher);
    
    if (key.size() != keySize) {
        throw std::runtime_error("Backend: Invalid key size");
    }
    
    std::vector<unsigned char> actualNonce = nonce;
    if (algorithm == "XChaCha20-Poly1305") {
        if (nonce.size() != 24) {
            throw std::runtime_error("Backend: XChaCha20 requires 24-byte nonce");
        }
        actualNonce = onc_core_backend_impl_xchacha20ToChacha20Nonce(nonce, key);
    } else {
        if (nonce.size() != 12 && nonce.size() != 24) {
            throw std::runtime_error("Backend: Invalid nonce size");
        }
        if (nonce.size() == 24) {
            actualNonce.resize(12);
            std::memcpy(actualNonce.data(), nonce.data(), 12);
        }
    }
    
    std::vector<unsigned char> plaintext(ciphertext.size());
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Backend: Failed to create cipher context");
    }
    
    int outlen = 0, tmplen = 0;
    
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, actualNonce.size(), nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), actualNonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Backend: Decryption init failed");
    }
    
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &outlen, ciphertext.data(), ciphertext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Backend: Decryption update failed");
    }
    
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag.size(), (void*)tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Backend: Failed to set tag");
    }
    
    int rv = EVP_DecryptFinal_ex(ctx, plaintext.data() + outlen, &tmplen);
    EVP_CIPHER_CTX_free(ctx);
    
    if (rv <= 0) {
        throw std::runtime_error("Backend: Decryption failed - incorrect key or corrupted data");
    }
    
    outlen += tmplen;
    plaintext.resize(outlen);
    
    return plaintext;
}

} // namespace onc::core::backend
