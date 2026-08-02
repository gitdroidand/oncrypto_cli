#include "oncrypto/backend/Backend.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/core_names.h>
#include <stdexcept>
#include <cstring>

namespace onc::core::backend {

// ============================================================
// Random
// ============================================================

std::vector<unsigned char> randomBytes(size_t size) {
    std::vector<unsigned char> bytes(size);
    if (RAND_bytes(bytes.data(), size) != 1) {
        throw std::runtime_error("Backend: Failed to generate random bytes");
    }
    return bytes;
}

// ============================================================
// Key Derivation (PBKDF2)
// ============================================================

std::vector<unsigned char> deriveKey(
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

static std::vector<unsigned char> xchacha20ToChacha20Nonce(
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

static const EVP_CIPHER* getCipher(const std::string& algorithm) {
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

EncryptResult encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
) {
    const EVP_CIPHER* cipher = getCipher(algorithm);
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
        actualNonce = xchacha20ToChacha20Nonce(nonce, key);
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

std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
) {
    const EVP_CIPHER* cipher = getCipher(algorithm);
    size_t keySize = EVP_CIPHER_key_length(cipher);
    
    if (key.size() != keySize) {
        throw std::runtime_error("Backend: Invalid key size");
    }
    
    std::vector<unsigned char> actualNonce = nonce;
    if (algorithm == "XChaCha20-Poly1305") {
        if (nonce.size() != 24) {
            throw std::runtime_error("Backend: XChaCha20 requires 24-byte nonce");
        }
        actualNonce = xchacha20ToChacha20Nonce(nonce, key);
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
