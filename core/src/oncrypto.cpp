#include "oncrypto/oncrypto.hpp"
#include "CryptoRepository.hpp"
#include "utils/FileUtils.hpp"
#include "format/OnCFormat.hpp"
#include "oncrypto/backend/Backend.hpp"
#include "oncrypto/streaming/Streaming.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

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

std::vector<unsigned char> onc::encrypt(
    std::span<const std::uint8_t> data,
    std::string_view password
) {
    crypto::CryptoRepository repo;
    return repo.encrypt(data, password).data;
}

std::vector<unsigned char> onc::decrypt(
    std::span<const std::uint8_t> data,
    std::string_view password
) {
    crypto::CryptoRepository repo;
    return repo.decrypt(data, password);
}

std::vector<unsigned char> onc::encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    return crypto::encrypt(data, password);
}

std::vector<unsigned char> onc::decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    return crypto::decrypt(data, password);
}

std::string crypto::getAlgorithmName() {
    return "AES-256-GCM / ChaCha20 / XChaCha20 (auto-selected)";
}

std::string crypto::getVersion() {
    return "1.6.0";
}

// ============================================================
// Layer 1: File API
//
// IMPORTANT:
// Public API is unchanged.
//
// The previous implementation loaded the entire file into
// memory:
//
//     readFile()
//       -> full input allocation
//     crypto::encrypt()
//       -> full encrypted allocation
//     writeFile()
//
// That creates a very large peak RSS for large files.
//
// File operations now use the existing streaming layer.
// ============================================================

namespace {

constexpr size_t DEFAULT_FILE_CHUNK_SIZE = 1024 * 1024; // 1 MiB

} // namespace

bool crypto::encryptFile(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password
) {
    if (inputFile.empty() ||
        outputFile.empty() ||
        password.empty()) {
        return false;
    }

    try {
        onc::streaming::EncryptStream stream;

        stream.input(inputFile)
              .output(outputFile)
              .password(password)
              .chunkSize(DEFAULT_FILE_CHUNK_SIZE);

        return stream.process();
    } catch (...) {
        return false;
    }
}

bool crypto::decryptFile(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password
) {
    if (inputFile.empty() ||
        outputFile.empty() ||
        password.empty()) {
        return false;
    }

    try {
        onc::streaming::DecryptStream stream;

        stream.input(inputFile)
              .output(outputFile)
              .password(password)
              .chunkSize(DEFAULT_FILE_CHUNK_SIZE);

        return stream.process();
    } catch (...) {
        return false;
    }
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

namespace onc::extreme {
namespace {

std::string algorithmName(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::AES256_GCM:
            return "AES-256-GCM";
        case Algorithm::ChaCha20_Poly1305:
            return "ChaCha20-Poly1305";
        case Algorithm::XChaCha20_Poly1305:
            return "XChaCha20-Poly1305";
        default:
            throw std::runtime_error("onc::extreme: unsupported algorithm");
    }
}

std::size_t defaultKeyLength(Algorithm algorithm) {
    (void)algorithm;
    return 32u;
}

std::size_t defaultNonceLength(Algorithm algorithm) {
    return (algorithm == Algorithm::XChaCha20_Poly1305) ? 24u : 12u;
}

std::vector<unsigned char> resolveSalt(const std::vector<unsigned char>& salt) {
    if (!salt.empty() && salt.size() != 16) {
        throw std::runtime_error("onc::extreme: salt must be exactly 16 bytes");
    }
    return salt.empty() ? onc::core::backend::randomBytes(16) : salt;
}

std::vector<unsigned char> resolveNonce(
    Algorithm algorithm,
    std::size_t requestedLength,
    const std::vector<unsigned char>& nonce
) {
    const std::size_t expected = (!nonce.empty() && requestedLength == 0)
        ? nonce.size()
        : (requestedLength == 0 ? defaultNonceLength(algorithm) : requestedLength);
    if (!nonce.empty() && nonce.size() != expected) {
        throw std::runtime_error("onc::extreme: nonce length does not match algorithm requirements");
    }
    return nonce.empty() ? onc::core::backend::randomBytes(expected) : nonce;
}

std::vector<unsigned char> deriveKeyForOptions(
    std::string_view password,
    const EncryptOptions& options,
    const std::vector<unsigned char>& salt
) {
    const std::size_t effectiveKeyLength = options.key_length == 0 ? defaultKeyLength(options.algorithm) : options.key_length;
    if (options.use_raw_key) {
        if (options.key.empty()) {
            throw std::runtime_error("onc::extreme: raw key mode requires a caller-provided key");
        }
        if (options.key.size() != effectiveKeyLength) {
            throw std::runtime_error("onc::extreme: provided key length does not match the selected algorithm requirements");
        }
        return options.key;
    }
    return onc::core::backend::deriveKey(std::string(password), salt, effectiveKeyLength, static_cast<std::size_t>(options.iterations));
}

std::vector<unsigned char> deriveKeyForOptions(
    std::string_view password,
    const DecryptOptions& options,
    const std::vector<unsigned char>& salt
) {
    const std::size_t effectiveKeyLength = options.key_length == 0 ? defaultKeyLength(options.algorithm) : options.key_length;
    if (options.use_raw_key) {
        if (options.key.empty()) {
            throw std::runtime_error("onc::extreme: raw key mode requires a caller-provided key");
        }
        if (options.key.size() != effectiveKeyLength) {
            throw std::runtime_error("onc::extreme: provided key length does not match the selected algorithm requirements");
        }
        return options.key;
    }
    return onc::core::backend::deriveKey(std::string(password), salt, effectiveKeyLength, static_cast<std::size_t>(options.iterations));
}

std::vector<unsigned char> rawAeadEncrypt(
    std::span<const std::uint8_t> data,
    std::string_view password,
    const EncryptOptions& options,
    const std::string& algorithmNameValue,
    const std::vector<unsigned char>& salt,
    const std::vector<unsigned char>& nonce
) {
    const std::vector<unsigned char> key = deriveKeyForOptions(password, options, salt);

    auto encrypted = onc::core::backend::encrypt(
        std::vector<unsigned char>(data.begin(), data.end()),
        key,
        nonce,
        algorithmNameValue
    );

    std::vector<unsigned char> output;
    output.reserve(encrypted.ciphertext.size() + encrypted.tag.size());
    output.insert(output.end(), encrypted.ciphertext.begin(), encrypted.ciphertext.end());
    output.insert(output.end(), encrypted.tag.begin(), encrypted.tag.end());
    return output;
}

std::vector<unsigned char> rawAeadDecrypt(
    std::span<const std::uint8_t> data,
    std::string_view password,
    const DecryptOptions& options,
    const std::string& algorithmNameValue,
    const std::vector<unsigned char>& salt,
    const std::vector<unsigned char>& nonce
) {
    if (data.size() < 16) {
        throw std::runtime_error("onc::extreme: raw payload is too short to contain an AEAD tag");
    }

    const std::vector<unsigned char> key = deriveKeyForOptions(password, options, salt);
    std::vector<unsigned char> ciphertext(data.begin(), data.end() - 16);
    std::vector<unsigned char> tag(data.end() - 16, data.end());
    return onc::core::backend::decrypt(ciphertext, key, nonce, tag, algorithmNameValue);
}

} // namespace

std::size_t required_output_size(std::size_t plaintext_size, bool include_metadata) {
    constexpr std::size_t header_size = sizeof(onc::format::OnCHeader);
    constexpr std::size_t tag_size = 16;
    constexpr std::size_t nonce_size = 12;
    const std::size_t metadata_size = include_metadata ? (header_size + nonce_size + tag_size) : 0;
    return plaintext_size + tag_size + metadata_size;
}

bool encrypt_into(
    ByteView data,
    std::string_view password,
    const EncryptOptions& options,
    MutableByteView output,
    std::size_t* bytes_written
) {
    const auto encrypted = encrypt(data, password, options);
    if (output.size() < encrypted.size()) {
        if (bytes_written) {
            *bytes_written = encrypted.size();
        }
        return false;
    }
    std::memcpy(output.data(), encrypted.data(), encrypted.size());
    if (bytes_written) {
        *bytes_written = encrypted.size();
    }
    return true;
}

bool decrypt_into(
    ByteView data,
    std::string_view password,
    const DecryptOptions& options,
    MutableByteView output,
    std::size_t* bytes_written
) {
    const auto decrypted = decrypt(data, password, options);
    if (output.size() < decrypted.size()) {
        if (bytes_written) {
            *bytes_written = decrypted.size();
        }
        return false;
    }
    std::memcpy(output.data(), decrypted.data(), decrypted.size());
    if (bytes_written) {
        *bytes_written = decrypted.size();
    }
    return true;
}

std::vector<unsigned char> encrypt(
    std::span<const std::uint8_t> data,
    std::string_view password,
    const EncryptOptions& options
) {
    if (options.kdf != Kdf::PBKDF2_SHA256) {
        throw std::runtime_error("onc::extreme: unsupported KDF");
    }

    const std::string algorithmNameValue = algorithmName(options.algorithm);
    const std::size_t effectiveNonceLength = options.nonce_length == 0 ? defaultNonceLength(options.algorithm) : options.nonce_length;
    const std::vector<unsigned char> salt = resolveSalt(options.salt);
    const std::vector<unsigned char> nonce = resolveNonce(options.algorithm, effectiveNonceLength, options.nonce);
    const std::vector<unsigned char> key = deriveKeyForOptions(password, options, salt);

    auto encrypted = onc::core::backend::encrypt(
        std::vector<unsigned char>(data.begin(), data.end()),
        key,
        nonce,
        algorithmNameValue
    );

    if (!options.store_metadata) {
        std::vector<unsigned char> output;
        output.reserve(encrypted.ciphertext.size() + encrypted.tag.size());
        output.insert(output.end(), encrypted.ciphertext.begin(), encrypted.ciphertext.end());
        output.insert(output.end(), encrypted.tag.begin(), encrypted.tag.end());
        return output;
    }

    onc::format::OnCMetadata metadata;
    metadata.algorithmName = algorithmNameValue;
    metadata.kdfName = "PBKDF2-SHA256";
    metadata.iterations = options.iterations == 0 ? 100000u : options.iterations;
    metadata.salt = salt;
    metadata.nonce = nonce;
    metadata.tag = encrypted.tag;

    std::vector<uint8_t> serialized(sizeof(onc::format::OnCHeader) + metadata.nonce.size() + metadata.tag.size() + encrypted.ciphertext.size());
    onc::format::serialize_into(metadata, encrypted.ciphertext, serialized);
    return serialized;
}

std::vector<unsigned char> decrypt(
    std::span<const std::uint8_t> data,
    std::string_view password,
    const DecryptOptions& options
) {
    if (options.kdf != Kdf::PBKDF2_SHA256) {
        throw std::runtime_error("onc::extreme: unsupported KDF");
    }

    if (onc::format::validateHeader(std::vector<unsigned char>(data.begin(), data.end()))) {
        const auto view = onc::format::deserialize_view(data);
        const std::string algorithmNameValue = view.algorithmName;
        const std::vector<unsigned char> salt(view.salt.begin(), view.salt.end());
        const std::vector<unsigned char> nonce(view.nonce.begin(), view.nonce.end());
        const std::vector<unsigned char> key = deriveKeyForOptions(password, options, salt);
        std::vector<unsigned char> ciphertext(view.data.begin(), view.data.end());
        return onc::core::backend::decrypt(ciphertext, key, nonce, std::vector<unsigned char>(view.tag.begin(), view.tag.end()), algorithmNameValue);
    }

    if (!options.use_raw_key && (options.salt.empty() || options.nonce.empty())) {
        throw std::runtime_error("onc::extreme: raw payload decryption requires explicit salt and nonce values");
    }
    if (options.nonce.empty()) {
        throw std::runtime_error("onc::extreme: raw payload decryption requires an explicit nonce");
    }

    const std::string algorithmNameValue = algorithmName(options.algorithm);
    const std::vector<unsigned char> salt = options.use_raw_key ? std::vector<unsigned char>() : resolveSalt(options.salt);
    const std::vector<unsigned char> nonce = resolveNonce(options.algorithm, options.nonce_length, options.nonce);
    return rawAeadDecrypt(data, password, options, algorithmNameValue, salt, nonce);
}

EncryptContext::EncryptContext(EncryptOptions options) {
    reset(std::move(options));
}

void EncryptContext::reset(EncryptOptions options) {
    options_ = std::move(options);
    initialized_ = false;
    buffer_.clear();
    key_.clear();
    salt_.clear();
    nonce_.clear();
}

std::vector<unsigned char> EncryptContext::update(ByteView chunk) {
    if (!initialized_) {
        salt_ = resolveSalt(options_.salt);
        nonce_ = resolveNonce(options_.algorithm, options_.nonce_length, options_.nonce);
        key_ = deriveKeyForOptions("", options_, salt_);
        initialized_ = true;
    }
    buffer_.insert(buffer_.end(), chunk.begin(), chunk.end());
    return {};
}

std::vector<unsigned char> EncryptContext::final() {
    if (!initialized_) {
        salt_ = resolveSalt(options_.salt);
        nonce_ = resolveNonce(options_.algorithm, options_.nonce_length, options_.nonce);
        key_ = deriveKeyForOptions("", options_, salt_);
        initialized_ = true;
    }

    const std::string algorithmNameValue = algorithmName(options_.algorithm);
    auto encrypted = onc::core::backend::encrypt(buffer_, key_, nonce_, algorithmNameValue);
    std::vector<unsigned char> out;
    out.reserve(encrypted.ciphertext.size() + encrypted.tag.size());
    out.insert(out.end(), encrypted.ciphertext.begin(), encrypted.ciphertext.end());
    out.insert(out.end(), encrypted.tag.begin(), encrypted.tag.end());
    buffer_.clear();
    return out;
}

DecryptContext::DecryptContext(DecryptOptions options) {
    reset(std::move(options));
}

void DecryptContext::reset(DecryptOptions options) {
    options_ = std::move(options);
    initialized_ = false;
    buffer_.clear();
    key_.clear();
    salt_.clear();
    nonce_.clear();
}

std::vector<unsigned char> DecryptContext::update(ByteView chunk) {
    if (!initialized_) {
        salt_ = resolveSalt(options_.salt);
        nonce_ = resolveNonce(options_.algorithm, options_.nonce_length, options_.nonce);
        key_ = deriveKeyForOptions("", options_, salt_);
        initialized_ = true;
    }
    buffer_.insert(buffer_.end(), chunk.begin(), chunk.end());
    return {};
}

std::vector<unsigned char> DecryptContext::final() {
    if (!initialized_) {
        salt_ = resolveSalt(options_.salt);
        nonce_ = resolveNonce(options_.algorithm, options_.nonce_length, options_.nonce);
        key_ = deriveKeyForOptions("", options_, salt_);
        initialized_ = true;
    }

    const std::string algorithmNameValue = algorithmName(options_.algorithm);
    if (buffer_.size() < 16) {
        throw std::runtime_error("onc::extreme: decrypt context buffer is too short");
    }
    std::vector<unsigned char> ciphertext(buffer_.begin(), buffer_.end() - 16);
    std::vector<unsigned char> tag(buffer_.end() - 16, buffer_.end());
    auto out = onc::core::backend::decrypt(ciphertext, key_, nonce_, tag, algorithmNameValue);
    buffer_.clear();
    return out;
}

} // namespace onc::extreme