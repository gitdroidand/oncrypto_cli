#include "oncrypto/streaming/Streaming.hpp"
#include "oncrypto/backend/Backend.hpp"
#include "format/OnCFormat.hpp"

#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <limits>

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
        throw std::runtime_error(
            "Streaming: Input, output, and password must be set"
        );
    }

    if (chunkSize_ == 0) {
        throw std::runtime_error(
            "Streaming: Chunk size must be greater than zero"
        );
    }

    std::ifstream in(inputPath_, std::ios::binary);
    if (!in) {
        throw std::runtime_error(
            "Streaming: Cannot open input file"
        );
    }

    std::ofstream out(outputPath_, std::ios::binary);
    if (!out) {
        throw std::runtime_error(
            "Streaming: Cannot open output file"
        );
    }

    // Get file size.
    in.seekg(0, std::ios::end);
    const auto fileSize = in.tellg();

    if (fileSize < 0) {
        throw std::runtime_error(
            "Streaming: Failed to determine input file size"
        );
    }

    totalBytes_ = static_cast<size_t>(fileSize);

    in.seekg(0, std::ios::beg);

    // Generate salt.
    auto salt = onc::core::backend::randomBytes(16);

    // Derive key once.
    auto key = onc::core::backend::deriveKey(
        password_,
        salt,
        32,
        100000
    );

    // Streaming algorithm.
    const std::string algorithm = "ChaCha20-Poly1305";
    constexpr size_t nonceSize = 12;

    // Header.
    out.write(
        reinterpret_cast<const char*>(salt.data()),
        static_cast<std::streamsize>(salt.size())
    );

    const uint8_t algoId = 2; // ChaCha20

    out.write(
        reinterpret_cast<const char*>(&algoId),
        sizeof(algoId)
    );

    // --------------------------------------------------------
    // Reusable buffers
    // --------------------------------------------------------
    //
    // IMPORTANT:
    // The previous implementation created a temporary vector
    // for every chunk:
    //
    // std::vector<unsigned char>(
    //     buffer.begin(),
    //     buffer.begin() + bytesRead
    // )
    //
    // That caused an unnecessary allocation + copy.
    //
    // We instead resize the existing buffer and pass it directly
    // to backend::encrypt().
    //
    std::vector<unsigned char> buffer;
    buffer.resize(chunkSize_);

    std::vector<unsigned char> nonce(nonceSize);

    uint64_t counter = 0;

    while (in) {
        in.read(
            reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
        );

        const std::streamsize readCount = in.gcount();

        if (readCount <= 0) {
            break;
        }

        const size_t bytesRead =
            static_cast<size_t>(readCount);

        // Keep the same buffer allocation.
        // No temporary vector is created here.
        buffer.resize(bytesRead);

        // Generate nonce from counter.
        std::memset(
            nonce.data(),
            0,
            nonce.size()
        );

        std::memcpy(
            nonce.data(),
            &counter,
            sizeof(counter)
        );

        // Encrypt directly from the reusable buffer.
        auto result = onc::core::backend::encrypt(
            buffer,
            key,
            nonce,
            algorithm
        );

        const uint32_t encryptedSize =
            static_cast<uint32_t>(result.ciphertext.size());

        // Write chunk size.
        out.write(
            reinterpret_cast<const char*>(&encryptedSize),
            sizeof(encryptedSize)
        );

        // Write authentication tag.
        out.write(
            reinterpret_cast<const char*>(result.tag.data()),
            static_cast<std::streamsize>(result.tag.size())
        );

        // Write ciphertext.
        out.write(
            reinterpret_cast<const char*>(
                result.ciphertext.data()
            ),
            static_cast<std::streamsize>(
                result.ciphertext.size()
            )
        );

        if (!out) {
            throw std::runtime_error(
                "Streaming: Failed to write encrypted output"
            );
        }

        processedBytes_ += bytesRead;
        ++counter;

        if (progressCallback_) {
            progressCallback_(
                processedBytes_,
                totalBytes_
            );
        }

        // Restore the reusable input buffer capacity for the
        // next chunk without forcing a new allocation.
        buffer.resize(chunkSize_);
    }

    return true;
}

size_t EncryptStream::getProgress() const {
    if (totalBytes_ == 0) {
        return 0;
    }

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
        throw std::runtime_error(
            "Streaming: Input, output, and password must be set"
        );
    }

    std::ifstream in(inputPath_, std::ios::binary);
    if (!in) {
        throw std::runtime_error(
            "Streaming: Cannot open input file"
        );
    }

    std::ofstream out(outputPath_, std::ios::binary);
    if (!out) {
        throw std::runtime_error(
            "Streaming: Cannot open output file"
        );
    }

    // Get file size.
    in.seekg(0, std::ios::end);
    const auto fileSize = in.tellg();

    if (fileSize < 0) {
        throw std::runtime_error(
            "Streaming: Failed to determine input file size"
        );
    }

    totalBytes_ = static_cast<size_t>(fileSize);

    in.seekg(0, std::ios::beg);

    // Read salt.
    std::vector<unsigned char> salt(16);

    in.read(
        reinterpret_cast<char*>(salt.data()),
        static_cast<std::streamsize>(salt.size())
    );

    if (in.gcount() != static_cast<std::streamsize>(salt.size())) {
        throw std::runtime_error(
            "Streaming: Invalid or truncated salt"
        );
    }

    // Read algorithm ID.
    uint8_t algoId = 0;

    in.read(
        reinterpret_cast<char*>(&algoId),
        sizeof(algoId)
    );

    if (!in) {
        throw std::runtime_error(
            "Streaming: Invalid or truncated algorithm header"
        );
    }

    if (algoId != 2) {
        throw std::runtime_error(
            "Streaming: Unsupported streaming algorithm"
        );
    }

    const std::string algorithm = "ChaCha20-Poly1305";
    constexpr size_t nonceSize = 12;
    constexpr size_t tagSize = 16;

    // Derive key once.
    auto key = onc::core::backend::deriveKey(
        password_,
        salt,
        32,
        100000
    );

    // --------------------------------------------------------
    // Reusable buffers
    // --------------------------------------------------------

    std::vector<unsigned char> nonce(nonceSize);

    // Reused between chunks.
    std::vector<unsigned char> tag(tagSize);

    // Reused between chunks.
    std::vector<unsigned char> ciphertext;

    uint64_t counter = 0;

    while (in) {
        // Read chunk size.
        uint32_t encryptedSize = 0;

        in.read(
            reinterpret_cast<char*>(&encryptedSize),
            sizeof(encryptedSize)
        );

        if (in.eof()) {
            break;
        }

        if (!in) {
            throw std::runtime_error(
                "Streaming: Failed to read chunk size"
            );
        }

        // Basic sanity check.
        //
        // Prevent pathological allocations caused by malformed
        // encrypted files.
        if (encryptedSize == 0) {
            throw std::runtime_error(
                "Streaming: Invalid encrypted chunk size"
            );
        }

        // Reuse tag allocation.
        in.read(
            reinterpret_cast<char*>(tag.data()),
            static_cast<std::streamsize>(tag.size())
        );

        if (!in) {
            throw std::runtime_error(
                "Streaming: Truncated authentication tag"
            );
        }

        // Reuse ciphertext capacity.
        ciphertext.resize(encryptedSize);

        in.read(
            reinterpret_cast<char*>(ciphertext.data()),
            static_cast<std::streamsize>(ciphertext.size())
        );

        if (!in) {
            throw std::runtime_error(
                "Streaming: Truncated ciphertext"
            );
        }

        // Generate nonce.
        std::memset(
            nonce.data(),
            0,
            nonce.size()
        );

        std::memcpy(
            nonce.data(),
            &counter,
            sizeof(counter)
        );

        // Decrypt directly from reusable ciphertext buffer.
        auto plaintext = onc::core::backend::decrypt(
            ciphertext,
            key,
            nonce,
            tag,
            algorithm
        );

        // Write plaintext.
        out.write(
            reinterpret_cast<const char*>(plaintext.data()),
            static_cast<std::streamsize>(plaintext.size())
        );

        if (!out) {
            throw std::runtime_error(
                "Streaming: Failed to write decrypted output"
            );
        }

        processedBytes_ += plaintext.size();
        ++counter;

        if (progressCallback_) {
            progressCallback_(
                processedBytes_,
                totalBytes_
            );
        }
    }

    return true;
}

size_t DecryptStream::getProgress() const {
    if (totalBytes_ == 0) {
        return 0;
    }

    return (processedBytes_ * 100) / totalBytes_;
}

size_t DecryptStream::getTotalBytes() const {
    return totalBytes_;
}

size_t DecryptStream::getProcessedBytes() const {
    return processedBytes_;
}

} // namespace onc::streaming