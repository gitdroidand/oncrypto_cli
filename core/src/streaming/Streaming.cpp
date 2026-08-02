#include "oncrypto/streaming/Streaming.hpp"
#include "oncrypto/backend/Backend.hpp"
#include "format/OnCFormat.hpp"
#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>

namespace onc::streaming {

// ============================================================
// EncryptStream
// ============================================================

EncryptStream::EncryptStream() = default;
EncryptStream::~EncryptStream() = default;

EncryptStream& EncryptStream::input(const std::string& path) {
    inputPath_ = path;
    return *this;
}

EncryptStream& EncryptStream::output(const std::string& path) {
    outputPath_ = path;
    return *this;
}

EncryptStream& EncryptStream::password(const std::string& pwd) {
    password_ = pwd;
    return *this;
}

EncryptStream& EncryptStream::chunkSize(size_t size) {
    chunkSize_ = size;
    return *this;
}

EncryptStream& EncryptStream::onProgress(ProgressCallback callback) {
    progressCallback_ = callback;
    return *this;
}

bool EncryptStream::process() {
    if (inputPath_.empty() || outputPath_.empty() || password_.empty()) {
        throw std::runtime_error("Streaming: Input, output, and password must be set");
    }

    std::ifstream in(inputPath_, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Streaming: Cannot open input file");
    }

    std::ofstream out(outputPath_, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Streaming: Cannot open output file");
    }

    // Get file size
    in.seekg(0, std::ios::end);
    totalBytes_ = in.tellg();
    in.seekg(0, std::ios::beg);

    // Generate salt
    auto salt = onc::core::backend::randomBytes(16);

    // Derive key once
    auto key = onc::core::backend::deriveKey(password_, salt, 32, 100000);

    // Choose algorithm (ChaCha20 for streaming)
    std::string algorithm = "ChaCha20-Poly1305";
    size_t nonceSize = 12;

    // Write header (salt + algorithm info)
    // For streaming, we store salt and algorithm in a simple header
    // Full OnC Format will be added later
    out.write(reinterpret_cast<const char*>(salt.data()), salt.size());

    // Write algorithm ID (1 = ChaCha20)
    uint8_t algoId = 2;  // ChaCha20
    out.write(reinterpret_cast<const char*>(&algoId), 1);

    // Process chunks
    std::vector<unsigned char> buffer(chunkSize_);
    std::vector<unsigned char> nonce(nonceSize);
    uint64_t counter = 0;

    while (in) {
        in.read(reinterpret_cast<char*>(buffer.data()), chunkSize_);
        size_t bytesRead = in.gcount();

        if (bytesRead == 0) break;

        // Generate nonce from counter
        std::memset(nonce.data(), 0, nonceSize);
        std::memcpy(nonce.data(), &counter, sizeof(counter));

        // Encrypt chunk
        auto result = onc::core::backend::encrypt(
            std::vector<unsigned char>(buffer.begin(), buffer.begin() + bytesRead),
            key,
            nonce,
            algorithm
        );

        // Write chunk size, tag, and ciphertext
        uint32_t chunkSize = static_cast<uint32_t>(result.ciphertext.size());
        out.write(reinterpret_cast<const char*>(&chunkSize), sizeof(chunkSize));
        out.write(reinterpret_cast<const char*>(result.tag.data()), result.tag.size());
        out.write(reinterpret_cast<const char*>(result.ciphertext.data()), result.ciphertext.size());

        processedBytes_ += bytesRead;
        counter++;

        if (progressCallback_) {
            progressCallback_(processedBytes_, totalBytes_);
        }
    }

    return true;
}

size_t EncryptStream::getProgress() const {
    if (totalBytes_ == 0) return 0;
    return (processedBytes_ * 100) / totalBytes_;
}

size_t EncryptStream::getTotalBytes() const {
    return totalBytes_;
}

size_t EncryptStream::getProcessedBytes() const {
    return processedBytes_;
}

// ============================================================
// DecryptStream
// ============================================================

DecryptStream::DecryptStream() = default;
DecryptStream::~DecryptStream() = default;

DecryptStream& DecryptStream::input(const std::string& path) {
    inputPath_ = path;
    return *this;
}

DecryptStream& DecryptStream::output(const std::string& path) {
    outputPath_ = path;
    return *this;
}

DecryptStream& DecryptStream::password(const std::string& pwd) {
    password_ = pwd;
    return *this;
}

DecryptStream& DecryptStream::chunkSize(size_t size) {
    chunkSize_ = size;
    return *this;
}

DecryptStream& DecryptStream::onProgress(ProgressCallback callback) {
    progressCallback_ = callback;
    return *this;
}

bool DecryptStream::process() {
    if (inputPath_.empty() || outputPath_.empty() || password_.empty()) {
        throw std::runtime_error("Streaming: Input, output, and password must be set");
    }

    std::ifstream in(inputPath_, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Streaming: Cannot open input file");
    }

    std::ofstream out(outputPath_, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Streaming: Cannot open output file");
    }

    // Get file size
    in.seekg(0, std::ios::end);
    totalBytes_ = in.tellg();
    in.seekg(0, std::ios::beg);

    // Read salt
    std::vector<unsigned char> salt(16);
    in.read(reinterpret_cast<char*>(salt.data()), salt.size());

    // Read algorithm ID
    uint8_t algoId;
    in.read(reinterpret_cast<char*>(&algoId), 1);

    std::string algorithm = "ChaCha20-Poly1305";
    size_t nonceSize = 12;

    // Derive key
    auto key = onc::core::backend::deriveKey(password_, salt, 32, 100000);

    // Process chunks
    std::vector<unsigned char> nonce(nonceSize);
    uint64_t counter = 0;

    while (in) {
        // Read chunk size
        uint32_t chunkSize;
        in.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));
        if (in.eof()) break;

        // Read tag
        std::vector<unsigned char> tag(16);
        in.read(reinterpret_cast<char*>(tag.data()), tag.size());

        // Read ciphertext
        std::vector<unsigned char> ciphertext(chunkSize);
        in.read(reinterpret_cast<char*>(ciphertext.data()), chunkSize);

        // Generate nonce
        std::memset(nonce.data(), 0, nonceSize);
        std::memcpy(nonce.data(), &counter, sizeof(counter));

        // Decrypt chunk
        auto plaintext = onc::core::backend::decrypt(
            ciphertext,
            key,
            nonce,
            tag,
            algorithm
        );

        // Write plaintext
        out.write(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());

        processedBytes_ += plaintext.size();
        counter++;

        if (progressCallback_) {
            progressCallback_(processedBytes_, totalBytes_);
        }
    }

    return true;
}

size_t DecryptStream::getProgress() const {
    if (totalBytes_ == 0) return 0;
    return (processedBytes_ * 100) / totalBytes_;
}

size_t DecryptStream::getTotalBytes() const {
    return totalBytes_;
}

size_t DecryptStream::getProcessedBytes() const {
    return processedBytes_;
}

} // namespace onc::streaming
