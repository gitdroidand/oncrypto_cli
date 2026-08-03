#include "algo/AES256GCM.hpp"
#include "oncrypto/backend/Backend.hpp"
#include <stdexcept>

namespace crypto {

std::vector<unsigned char> AES256GCM::encrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::runtime_error("AES-256 requires 32-byte key");
    }

    auto iv = onc::core::backend::randomBytes(12);
    auto result = onc::core::backend::encrypt(data, key, iv, "AES-256-GCM");

    std::vector<unsigned char> output;
    output.reserve(iv.size() + result.tag.size() + result.ciphertext.size());
    output.insert(output.end(), iv.begin(), iv.end());
    output.insert(output.end(), result.tag.begin(), result.tag.end());
    output.insert(output.end(), result.ciphertext.begin(), result.ciphertext.end());
    return output;
}

std::vector<unsigned char> AES256GCM::decrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& key
) {
    if (key.size() != 32) {
        throw std::runtime_error("AES-256 requires 32-byte key");
    }
    if (data.size() < 28) {
        throw std::runtime_error("Data too short");
    }

    std::vector<unsigned char> iv(data.begin(), data.begin() + 12);
    std::vector<unsigned char> tag(data.begin() + 12, data.begin() + 28);
    std::vector<unsigned char> ciphertext(data.begin() + 28, data.end());

    return onc::core::backend::decrypt(ciphertext, key, iv, tag, "AES-256-GCM");
}

AlgorithmInfo AES256GCM::getInfo() const {
    return {
        AlgorithmType::AES256_GCM,
        "AES-256-GCM",
        "NIST standard, hardware accelerated on most CPUs",
        10
    };
}

} // namespace crypto