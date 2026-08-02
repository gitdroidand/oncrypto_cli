#include "CryptoRepository.hpp"
#include "algo/AES256GCM.hpp"
#include "algo/ChaCha20.hpp"
#include "algo/XChaCha20.hpp"
#include "utils/KeyDerivation.hpp"
#include <memory>
#include <stdexcept>

namespace crypto {

CryptoRepository::CryptoRepository() = default;
CryptoRepository::~CryptoRepository() = default;

EncryptionResult CryptoRepository::encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    auto algorithm = selectAlgorithm(data);
    auto info = algorithm->getInfo();
    
    // Generate salt
    auto salt = generateSalt(16);
    
    // Derive key from password
    auto key = deriveKey(password, algorithm->getKeySize(), salt);
    
    // Encrypt
    auto encrypted = algorithm->encrypt(data, key);
    
    // Prepend salt to result
    std::vector<unsigned char> result;
    result.reserve(salt.size() + encrypted.size());
    result.insert(result.end(), salt.begin(), salt.end());
    result.insert(result.end(), encrypted.begin(), encrypted.end());
    
    return {
        result,
        info.name,
        info.reason,
        salt
    };
}

std::vector<unsigned char> CryptoRepository::decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    if (data.size() < 16) {
        throw std::runtime_error("Data too short");
    }
    
    // Extract salt
    std::vector<unsigned char> salt(data.begin(), data.begin() + 16);
    std::vector<unsigned char> encryptedData(data.begin() + 16, data.end());
    
    // Try each algorithm
    std::vector<std::unique_ptr<Algorithm>> algorithms;
    algorithms.push_back(std::make_unique<AES256GCM>());
    algorithms.push_back(std::make_unique<ChaCha20>());
    algorithms.push_back(std::make_unique<XChaCha20>());
    
    for (auto& algo : algorithms) {
        try {
            auto key = deriveKey(password, algo->getKeySize(), salt);
            return algo->decrypt(encryptedData, key);
        } catch (...) {
            // Try next algorithm
            continue;
        }
    }
    
    throw std::runtime_error("Failed to decrypt with any algorithm");
}

std::unique_ptr<Algorithm> CryptoRepository::selectAlgorithm(
    const std::vector<unsigned char>& data
) {
    // Determine algorithm based on data characteristics
    
    // If data is small (< 1KB), use XChaCha20 for better security
    if (data.size() < 1024) {
        return std::make_unique<XChaCha20>();
    }
    
    // If data is large, use AES-256-GCM for hardware acceleration
    if (data.size() > 1024 * 1024) { // > 1MB
        return std::make_unique<AES256GCM>();
    }
    
    // Default: ChaCha20 (good balance)
    return std::make_unique<ChaCha20>();
}

std::string CryptoRepository::getAlgorithmReason(
    const std::vector<unsigned char>& data,
    AlgorithmType type
) {
    switch (type) {
        case AlgorithmType::AES256_GCM:
            return "Large data (>1MB), hardware acceleration available";
        case AlgorithmType::ChaCha20_Poly1305:
            return "Medium size data, software-optimized algorithm";
        case AlgorithmType::XChaCha20_Poly1305:
            return "Small data (<1KB), maximum security with extended nonce";
        default:
            return "Unknown algorithm";
    }
}

} // namespace crypto