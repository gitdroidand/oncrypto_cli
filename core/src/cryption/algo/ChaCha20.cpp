#include "algo/ChaCha20.hpp"
#include "oncrypto/backend/Backend.hpp"
#include <stdexcept>

namespace crypto {

std::vector<unsigned char> ChaCha20::encrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::runtime_error("ChaCha20 requires 32-byte key");
    }

    auto iv = onc::core::backend::randomBytes(12);
    auto result = onc::core::backend::encrypt(data, key, iv, "ChaCha20-Poly1305");

    std::vector<unsigned char> output;
    output.reserve(iv.size() + result.tag.size() + result.ciphertext.size());
    output.insert(output.end(), iv.begin(), iv.end());
    output.insert(output.end(), result.tag.begin(), result.tag.end());
    output.insert(output.end(), result.ciphertext.begin(), result.ciphertext.end());
    return output;
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

    return onc::core::backend::decrypt(ciphertext, key, iv, tag, "ChaCha20-Poly1305");
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