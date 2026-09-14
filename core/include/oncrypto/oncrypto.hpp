#pragma once

/**
 * OnCrypto Core Library
 * Version 1.4.0
 *
 * Main public API for OnCrypto encryption library
 */

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "oncrypto/Export.hpp"
#include "oncrypto/streaming/Streaming.hpp"

namespace crypto {

using ByteView = std::span<const std::uint8_t>;
using MutableByteView = std::span<std::uint8_t>;

// ============================================================
// Existing API is preserved and routed through the new onc API.
// ============================================================

[[deprecated("Use onc::encrypt() instead.")]]
ONCRYPTO_API std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

[[deprecated("Use onc::decrypt() instead.")]]
ONCRYPTO_API std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

ONCRYPTO_API std::string getAlgorithmName();
ONCRYPTO_API std::string getVersion();

ONCRYPTO_API bool encryptFile(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password
);

ONCRYPTO_API bool decryptFile(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password
);

} // namespace crypto

namespace crypto::builder {

enum class Algorithm {
    Auto,
    AES256_GCM,
    ChaCha20,
    XChaCha20
};

class ONCRYPTO_API Encryptor {
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

class ONCRYPTO_API Decryptor {
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

ONCRYPTO_API std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password,
    const EncryptionOptions& options
);

ONCRYPTO_API std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password,
    const DecryptionOptions& options
);

} // namespace crypto::advanced

namespace onc {

using Byte = std::uint8_t;
using ByteView = std::span<const std::uint8_t>;
using MutableByteView = std::span<std::uint8_t>;

enum class Algorithm {
    Auto,
    AES256_GCM,
    ChaCha20_Poly1305,
    XChaCha20_Poly1305
};

enum class OutputOwnership {
    LibraryOwned,
    CallerOwned
};

std::vector<unsigned char> encrypt(
    ByteView data,
    std::string_view password
);

std::vector<unsigned char> decrypt(
    ByteView data,
    std::string_view password
);

ONCRYPTO_API std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

ONCRYPTO_API std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

ONCRYPTO_API bool encryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize = 1024 * 1024,
    streaming::ProgressCallback callback = nullptr
);

ONCRYPTO_API bool decryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize = 1024 * 1024,
    streaming::ProgressCallback callback = nullptr
);

namespace extreme {

enum class Algorithm {
    AES256_GCM,
    ChaCha20_Poly1305,
    XChaCha20_Poly1305
};

enum class Kdf {
    PBKDF2_SHA256
};

struct EncryptOptions {
    Algorithm algorithm = Algorithm::AES256_GCM;
    Kdf kdf = Kdf::PBKDF2_SHA256;
    std::uint32_t iterations = 100000;
    std::size_t key_length = 0;
    std::size_t nonce_length = 0;
    std::vector<std::uint8_t> salt;
    std::vector<std::uint8_t> nonce;
    std::vector<std::uint8_t> key;
    bool use_raw_key = false;
    bool store_metadata = true;
    OutputOwnership ownership = OutputOwnership::LibraryOwned;
};

struct DecryptOptions {
    Algorithm algorithm = Algorithm::AES256_GCM;
    Kdf kdf = Kdf::PBKDF2_SHA256;
    std::uint32_t iterations = 100000;
    std::size_t key_length = 0;
    std::size_t nonce_length = 0;
    std::vector<std::uint8_t> salt;
    std::vector<std::uint8_t> nonce;
    std::vector<std::uint8_t> key;
    bool use_raw_key = false;
    bool verify_integrity = true;
    OutputOwnership ownership = OutputOwnership::LibraryOwned;
};

std::size_t required_output_size(std::size_t plaintext_size, bool include_metadata = true);

bool encrypt_into(
    ByteView data,
    std::string_view password,
    const EncryptOptions& options,
    MutableByteView output,
    std::size_t* bytes_written = nullptr
);

bool decrypt_into(
    ByteView data,
    std::string_view password,
    const DecryptOptions& options,
    MutableByteView output,
    std::size_t* bytes_written = nullptr
);

std::vector<unsigned char> encrypt(
    ByteView data,
    std::string_view password,
    const EncryptOptions& options
);

std::vector<unsigned char> decrypt(
    ByteView data,
    std::string_view password,
    const DecryptOptions& options
);

class EncryptContext {
public:
    explicit EncryptContext(EncryptOptions options = {});

    void reset(EncryptOptions options);
    std::vector<unsigned char> update(ByteView chunk);
    std::vector<unsigned char> final();

private:
    EncryptOptions options_;
    std::vector<unsigned char> key_;
    std::vector<unsigned char> salt_;
    std::vector<unsigned char> nonce_;
    std::vector<unsigned char> buffer_;
    bool initialized_ = false;
};

class DecryptContext {
public:
    explicit DecryptContext(DecryptOptions options = {});

    void reset(DecryptOptions options);
    std::vector<unsigned char> update(ByteView chunk);
    std::vector<unsigned char> final();

private:
    DecryptOptions options_;
    std::vector<unsigned char> key_;
    std::vector<unsigned char> salt_;
    std::vector<unsigned char> nonce_;
    std::vector<unsigned char> buffer_;
    bool initialized_ = false;
};

} // namespace extreme

} // namespace onc
