#pragma once

/**
 * OnCrypto Core Library
 * Version 1.4.0
 * 
 * Main public API for OnCrypto encryption library
 */

#include <vector>
#include <string>
#include <optional>

namespace crypto {

// ============================================================
// ✅ Existing API (100% unchanged - Backward Compatible)
// ============================================================

/**
 * Encrypt data with password
 * @param data Raw data to encrypt
 * @param password User password
 * @return Encrypted data (includes salt + IV + tag + ciphertext)
 */
std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

/**
 * Decrypt data with password
 * @param data Encrypted data
 * @param password User password
 * @return Original decrypted data
 */
std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

/**
 * Get the algorithm name used
 */
std::string getAlgorithmName();

/**
 * Get library version
 */
std::string getVersion();

// ============================================================
// ✅ NEW: Layer 1 - Simple File API
// ============================================================

/**
 * Encrypt file to file
 * @param inputFile Path to input file
 * @param outputFile Path to output file
 * @param password User password
 * @return true on success, false on failure
 */
bool encryptFile(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password
);

/**
 * Decrypt file to file
 * @param inputFile Path to encrypted file
 * @param outputFile Path to output file
 * @param password User password
 * @return true on success, false on failure
 */
bool decryptFile(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password
);

} // namespace crypto

// ============================================================
// ✅ NEW: Layer 2 - Builder API
// ============================================================

namespace crypto::builder {

enum class Algorithm {
    Auto,
    AES256_GCM,
    ChaCha20,
    XChaCha20
};

class Encryptor {
public:
    Encryptor& password(const std::string& pwd);
    Encryptor& algorithm(Algorithm algo);
    Encryptor& iterations(int iter);
    
    std::vector<unsigned char> encrypt(const std::vector<unsigned char>& data);
    bool encryptFile(const std::string& input, const std::string& output);

private:
    std::string password_;
    Algorithm algorithm_ = Algorithm::Auto;
    int iterations_ = 100000;
};

class Decryptor {
public:
    Decryptor& password(const std::string& pwd);
    Decryptor& algorithm(Algorithm algo);
    
    std::vector<unsigned char> decrypt(const std::vector<unsigned char>& data);
    bool decryptFile(const std::string& input, const std::string& output);

private:
    std::string password_;
    Algorithm algorithm_ = Algorithm::Auto;
};

} // namespace crypto::builder

// ============================================================
// ✅ NEW: Layer 3 - Advanced API
// ============================================================

namespace crypto::advanced {

enum class KDF {
    PBKDF2,
    Argon2
};

enum class OutputFormat {
    Binary,
    Hex,
    Base64
};

struct EncryptionOptions {
    crypto::builder::Algorithm algorithm = crypto::builder::Algorithm::Auto;
    KDF kdf = KDF::PBKDF2;
    int iterations = 100000;
    OutputFormat outputFormat = OutputFormat::Binary;
    bool storeMetadata = true;
};

struct DecryptionOptions {
    crypto::builder::Algorithm algorithm = crypto::builder::Algorithm::Auto;
    bool verifyIntegrity = true;
};

/**
 * Advanced encrypt with full options
 */
std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password,
    const EncryptionOptions& options
);

/**
 * Advanced decrypt with full options
 */
std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password,
    const DecryptionOptions& options
);

} // namespace crypto::advanced


// ============================================================
// Streaming API (v1.5.0)
// ============================================================

#include "oncrypto/streaming/Streaming.hpp"

namespace onc {

/**
 * Stream encrypt a large file
 * @param inputFile Path to input file
 * @param outputFile Path to output file  
 * @param password User password
 * @param chunkSize Size of each chunk (default: 1MB)
 * @param callback Progress callback (optional)
 * @return true on success
 */
bool encryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize = 1024 * 1024,
    streaming::ProgressCallback callback = nullptr
);

/**
 * Stream decrypt a large file
 * @param inputFile Path to encrypted file
 * @param outputFile Path to output file
 * @param password User password
 * @param chunkSize Size of each chunk (default: 1MB)
 * @param callback Progress callback (optional)
 * @return true on success
 */
bool decryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize = 1024 * 1024,
    streaming::ProgressCallback callback = nullptr
);

} // namespace onc
