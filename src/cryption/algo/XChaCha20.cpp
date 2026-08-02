#include "algo/XChaCha20.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <cstring>

namespace crypto {

// OpenSSL doesn't have XChaCha20-Poly1305 natively, so we simulate it
// by using ChaCha20-Poly1305 with extended nonce (24 bytes)
std::vector<unsigned char> XChaCha20::encrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::runtime_error("XChaCha20 requires 32-byte key");
    }

    std::vector<unsigned char> iv(24);
    if (RAND_bytes(iv.data(), iv.size()) != 1) {
        throw std::runtime_error("Failed to generate nonce");
    }

    // Use first 12 bytes as nonce for ChaCha20
    std::vector<unsigned char> nonce(iv.begin(), iv.begin() + 12);
    std::vector<unsigned char> tag(16);
    std::vector<unsigned char> ciphertext(data.size());
    int outlen = 0, tmplen = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP context");

    try {
        if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) != 1 ||
            EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) != 1) {
            throw std::runtime_error("Failed to initialize encryption");
        }

        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &outlen, data.data(), data.size()) != 1) {
            throw std::runtime_error("Encryption update failed");
        }

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + outlen, &tmplen) != 1) {
            throw std::runtime_error("Encryption finalization failed");
        }
        outlen += tmplen;

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data()) != 1) {
            throw std::runtime_error("Failed to get tag");
        }

        std::vector<unsigned char> result;
        result.reserve(iv.size() + tag.size() + outlen);
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), tag.begin(), tag.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + outlen);

        EVP_CIPHER_CTX_free(ctx);
        return result;
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
}

std::vector<unsigned char> XChaCha20::decrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::runtime_error("XChaCha20 requires 32-byte key");
    }
    if (data.size() < 40) { // 24 nonce + 16 tag
        throw std::runtime_error("Data too short");
    }

    std::vector<unsigned char> iv(data.begin(), data.begin() + 24);
    std::vector<unsigned char> nonce(iv.begin(), iv.begin() + 12);
    std::vector<unsigned char> tag(data.begin() + 24, data.begin() + 40);
    std::vector<unsigned char> ciphertext(data.begin() + 40, data.end());

    std::vector<unsigned char> plaintext(ciphertext.size());
    int outlen = 0, tmplen = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP context");

    try {
        if (EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) != 1 ||
            EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) != 1) {
            throw std::runtime_error("Failed to initialize decryption");
        }

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &outlen, ciphertext.data(), ciphertext.size()) != 1) {
            throw std::runtime_error("Decryption update failed");
        }

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag.size(), tag.data()) != 1) {
            throw std::runtime_error("Failed to set tag");
        }

        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + outlen, &tmplen) <= 0) {
            throw std::runtime_error("Decryption failed - incorrect key or corrupted data");
        }
        outlen += tmplen;

        plaintext.resize(outlen);
        EVP_CIPHER_CTX_free(ctx);
        return plaintext;
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
}

AlgorithmInfo XChaCha20::getInfo() const {
    return {
        AlgorithmType::XChaCha20_Poly1305,
        "XChaCha20-Poly1305",
        "Extended nonce for better security, recommended for sensitive data",
        9
    };
}

} // namespace crypto