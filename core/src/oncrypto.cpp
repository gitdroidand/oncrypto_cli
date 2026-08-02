#include "oncrypto/oncrypto.hpp"
#include "CryptoRepository.hpp"
#include "utils/FileUtils.hpp"
#include "format/OnCFormat.hpp"
#include "oncrypto/backend/Backend.hpp"
#include "oncrypto/streaming/Streaming.hpp"
#include <fstream>

// ============================================================
// Existing API (unchanged)
// ============================================================

std::vector<unsigned char> crypto::encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    CryptoRepository repo;
    auto result = repo.encrypt(data, password);
    return result.data;
}

std::vector<unsigned char> crypto::decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    CryptoRepository repo;
    return repo.decrypt(data, password);
}

std::string crypto::getAlgorithmName() {
    return "AES-256-GCM / ChaCha20 / XChaCha20 (auto-selected)";
}

std::string crypto::getVersion() {
    return "1.5.0";
}

// ============================================================
// Layer 1: File API
// ============================================================

bool crypto::encryptFile(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password
) {
    auto data = readFile(inputFile);
    if (data.empty()) return false;
    
    auto encrypted = crypto::encrypt(data, password);
    return writeFile(outputFile, encrypted);
}

bool crypto::decryptFile(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password
) {
    auto data = readFile(inputFile);
    if (data.empty()) return false;
    
    auto decrypted = crypto::decrypt(data, password);
    return writeFile(outputFile, decrypted);
}

// ============================================================
// Layer 4: Streaming API (v1.5.0)
// ============================================================

bool onc::encryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize,
    onc::streaming::ProgressCallback callback
) {
    onc::streaming::EncryptStream stream;
    stream.input(inputFile)
          .output(outputFile)
          .password(password)
          .chunkSize(chunkSize);
    
    if (callback) {
        stream.onProgress(callback);
    }
    
    return stream.process();
}

bool onc::decryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize,
    onc::streaming::ProgressCallback callback
) {
    onc::streaming::DecryptStream stream;
    stream.input(inputFile)
          .output(outputFile)
          .password(password)
          .chunkSize(chunkSize);
    
    if (callback) {
        stream.onProgress(callback);
    }
    
    return stream.process();
}
