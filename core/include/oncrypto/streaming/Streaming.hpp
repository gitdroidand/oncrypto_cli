#pragma once

#include <string>
#include <cstddef>
#include <functional>

namespace onc::streaming {

// ============================================================
// Progress callback
// ============================================================

using ProgressCallback = std::function<void(size_t processed, size_t total)>;

// ============================================================
// EncryptStream
// ============================================================

class EncryptStream {
public:
    EncryptStream();
    ~EncryptStream();

    EncryptStream& input(const std::string& path);
    EncryptStream& output(const std::string& path);
    EncryptStream& password(const std::string& pwd);
    EncryptStream& chunkSize(size_t size);
    EncryptStream& onProgress(ProgressCallback callback);

    bool process();

    size_t getProgress() const;
    size_t getTotalBytes() const;
    size_t getProcessedBytes() const;

private:
    std::string inputPath_;
    std::string outputPath_;
    std::string password_;
    size_t chunkSize_ = 1024 * 1024;  // 1MB
    ProgressCallback progressCallback_;

    size_t totalBytes_ = 0;
    size_t processedBytes_ = 0;
};

// ============================================================
// DecryptStream
// ============================================================

class DecryptStream {
public:
    DecryptStream();
    ~DecryptStream();

    DecryptStream& input(const std::string& path);
    DecryptStream& output(const std::string& path);
    DecryptStream& password(const std::string& pwd);
    DecryptStream& chunkSize(size_t size);
    DecryptStream& onProgress(ProgressCallback callback);

    bool process();

    size_t getProgress() const;
    size_t getTotalBytes() const;
    size_t getProcessedBytes() const;

private:
    std::string inputPath_;
    std::string outputPath_;
    std::string password_;
    size_t chunkSize_ = 1024 * 1024;
    ProgressCallback progressCallback_;

    size_t totalBytes_ = 0;
    size_t processedBytes_ = 0;
};

} // namespace onc::streaming
