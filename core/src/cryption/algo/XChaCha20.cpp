#include "algo/XChaCha20.hpp"
#include "oncrypto/backend/Backend.hpp"
#include <stdexcept>

namespace crypto {

// OpenSSL doesn't have XChaCha20-Poly1305 natively, so we simulate it
// by using the backend transport for extended nonces.
std::vector<unsigned char> XChaCha20::encrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::runtime_error("XChaCha20 requires 32-byte key");
    }

    auto iv = onc::core::backend::randomBytes(24);
    auto result = onc::core::backend::encrypt(data, key, iv, "XChaCha20-Poly1305");

    std::vector<unsigned char> output;
    output.reserve(iv.size() + result.tag.size() + result.ciphertext.size());
    output.insert(output.end(), iv.begin(), iv.end());
    output.insert(output.end(), result.tag.begin(), result.tag.end());
    output.insert(output.end(), result.ciphertext.begin(), result.ciphertext.end());
    return output;
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
    std::vector<unsigned char> tag(data.begin() + 24, data.begin() + 40);
    std::vector<unsigned char> ciphertext(data.begin() + 40, data.end());

    return onc::core::backend::decrypt(ciphertext, key, iv, tag, "XChaCha20-Poly1305");
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