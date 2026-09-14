#include "oncrypto/backend/Backend.hpp"
#include "oncrypto/backend/EngineBackend.hpp"
#include "oncrypto_engine.h"

#include <stdexcept>
#include <vector>

namespace onc::core::backend {

// ============================================================
// Engine Backend
// ============================================================
//
// This layer is intentionally thin.
//
// Public Backend API remains unchanged:
//
//   randomBytes()
//   deriveKey()
//   encrypt()
//   decrypt()
//
// No public signature or data structure is changed.
//
// The engine C ABI receives raw pointers directly from the
// existing vectors. The actual elimination of internal input
// copies belongs to the engine implementation itself.
// ============================================================

// ============================================================
// Random
// ============================================================

std::vector<unsigned char> engineRandomBytes(size_t size) {
    std::vector<unsigned char> out(size);

    if (size == 0) {
        return out;
    }

    if (oncrypto_engine_random_bytes(out.data(), size) != 0) {
        throw std::runtime_error(
            "EngineBackend: random_bytes failed"
        );
    }

    return out;
}

// ============================================================
// PBKDF2
// ============================================================

std::vector<unsigned char> engineDeriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
) {
    std::vector<unsigned char> out(keySize);

    if (keySize == 0) {
        return out;
    }

    const int rv = oncrypto_engine_pbkdf2_hmac_sha256(
        password.c_str(),
        salt.empty() ? nullptr : salt.data(),
        salt.size(),
        static_cast<unsigned int>(iterations),
        out.data(),
        out.size()
    );

    if (rv != 0) {
        throw std::runtime_error(
            "EngineBackend: deriveKey failed"
        );
    }

    return out;
}

// ============================================================
// Encrypt
// ============================================================

EncryptResult engineEncrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
) {
    // AEAD ciphertext has the same size as plaintext for the
    // algorithms currently supported by OnCrypto.
    std::vector<unsigned char> ciphertext(plaintext.size());

    // Poly1305/GCM authentication tag is currently 16 bytes.
    std::vector<unsigned char> tag(16);

    size_t ciphertextLength = ciphertext.size();
    size_t tagLength = tag.size();

    const int rv = oncrypto_engine_aead_encrypt(
        algorithm.c_str(),

        key.empty() ? nullptr : key.data(),
        key.size(),

        nonce.empty() ? nullptr : nonce.data(),
        nonce.size(),

        plaintext.empty() ? nullptr : plaintext.data(),
        plaintext.size(),

        ciphertext.empty() ? nullptr : ciphertext.data(),
        &ciphertextLength,

        tag.data(),
        &tagLength
    );

    if (rv != 0) {
        throw std::runtime_error(
            "EngineBackend: aead_encrypt failed"
        );
    }

    // Normally these are already exact-size. Keep resize because
    // the stable C ABI explicitly returns the produced lengths.
    ciphertext.resize(ciphertextLength);
    tag.resize(tagLength);

    return EncryptResult{
        std::move(ciphertext),
        std::move(tag)
    };
}

// ============================================================
// Decrypt
// ============================================================

std::vector<unsigned char> engineDecrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
) {
    // AEAD decryption output cannot be larger than ciphertext for
    // the currently supported algorithms.
    std::vector<unsigned char> plaintext(ciphertext.size());

    size_t plaintextLength = plaintext.size();

    const int rv = oncrypto_engine_aead_decrypt(
        algorithm.c_str(),

        key.empty() ? nullptr : key.data(),
        key.size(),

        nonce.empty() ? nullptr : nonce.data(),
        nonce.size(),

        ciphertext.empty() ? nullptr : ciphertext.data(),
        ciphertext.size(),

        tag.empty() ? nullptr : tag.data(),
        tag.size(),

        plaintext.empty() ? nullptr : plaintext.data(),
        &plaintextLength
    );

    if (rv != 0) {
        throw std::runtime_error(
            "EngineBackend: aead_decrypt failed"
        );
    }

    plaintext.resize(plaintextLength);

    return plaintext;
}

// ============================================================
// Public Backend Dispatch
// ============================================================

std::vector<unsigned char> randomBytes(size_t size) {
    return engineRandomBytes(size);
}

std::vector<unsigned char> deriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
) {
    return engineDeriveKey(
        password,
        salt,
        keySize,
        iterations
    );
}

EncryptResult encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
) {
    return engineEncrypt(
        plaintext,
        key,
        nonce,
        algorithm
    );
}

std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
) {
    return engineDecrypt(
        ciphertext,
        key,
        nonce,
        tag,
        algorithm
    );
}

} // namespace onc::core::backend