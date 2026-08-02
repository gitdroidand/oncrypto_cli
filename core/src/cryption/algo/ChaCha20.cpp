#include "algo/ChaCha20.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <cstring>

namespace crypto {

std::vector<unsigned char> ChaCha20::encrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::runtime_error("ChaCha20 requires 32-byte key");
    }

    std::vector<unsigned char> iv(12);
    if (RAND_bytes(iv.data(), iv.size()) != 1) {
        throw std::runtime_error("Failed to generate nonce");
    }

    std::vector<unsigned char> tag(16);
    std::vector<unsigned char> ciphertext(data.size());
    int outlen = 0, tmplen = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP context");

    try {
        if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1 ||
            EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
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

std::vector<unsigned char> ChaCha20::decrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::runtime_error("ChaCha20 requires 32-byte key");
    }
    if (data.size() < 28) {
        throw std::runtime_error("Data too short");
    }

    std::vector<unsigned char> iv(data.begin(), data.begin() + 12);
    std::vector<unsigned char> tag(data.begin() + 12, data.begin() + 28);
    std::vector<unsigned char> ciphertext(data.begin() + 28, data.end());

    std::vector<unsigned char> plaintext(ciphertext.size());
    int outlen = 0, tmplen = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP context");

    try {
        if (EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1 ||
            EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
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

AlgorithmInfo ChaCha20::getInfo() const {
    return {
        AlgorithmType::ChaCha20_Poly1305,
        "ChaCha20-Poly1305",
        "Fast in software, no hardware acceleration needed",
        8
    };
}

} // namespace crypto