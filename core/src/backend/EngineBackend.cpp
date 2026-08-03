#include "oncrypto/backend/Backend.hpp"
#include "oncrypto/backend/EngineBackend.hpp"
#include <stdexcept>
#include <vector>
#include <cstring>

// Include the engine C ABI header (top-level include/ directory)
#include "oncrypto_engine.h"

namespace onc::core::backend {

// EngineBackend translates the core backend interface into the stable engine C ABI.
// This keeps the core implementation backend-agnostic and avoids direct dependency
// on a specific provider implementation.
std::vector<unsigned char> engineRandomBytes(size_t size) {
    std::vector<unsigned char> out(size);
    if (oncrypto_engine_random_bytes(out.data(), out.size()) != 0) {
        throw std::runtime_error("EngineBackend: random_bytes failed");
    }
    return out;
}

std::vector<unsigned char> engineDeriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
) {
    std::vector<unsigned char> out(keySize);
    int rv = oncrypto_engine_pbkdf2_hmac_sha256(
        password.c_str(),
        salt.data(),
        salt.size(),
        iterations,
        out.data(),
        out.size()
    );
    if (rv != 0) throw std::runtime_error("EngineBackend: deriveKey failed");
    return out;
}

EncryptResult engineEncrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
) {
    std::vector<unsigned char> ciphertext(plaintext.size());
    size_t ct_len = ciphertext.size();
    std::vector<unsigned char> tag(16);
    size_t tag_len = tag.size();

    int rv = oncrypto_engine_aead_encrypt(
        algorithm.c_str(),
        key.data(), key.size(),
        nonce.data(), nonce.size(),
        plaintext.data(), plaintext.size(),
        ciphertext.data(), &ct_len,
        tag.data(), &tag_len
    );
    if (rv != 0) throw std::runtime_error("EngineBackend: aead_encrypt failed");
    ciphertext.resize(ct_len);
    tag.resize(tag_len);
    return {ciphertext, tag};
}

std::vector<unsigned char> engineDecrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
) {
    std::vector<unsigned char> plaintext(ciphertext.size());
    size_t pt_len = plaintext.size();
    int rv = oncrypto_engine_aead_decrypt(
        algorithm.c_str(),
        key.data(), key.size(),
        nonce.data(), nonce.size(),
        ciphertext.data(), ciphertext.size(),
        tag.data(), tag.size(),
        plaintext.data(), &pt_len
    );
    if (rv != 0) throw std::runtime_error("EngineBackend: aead_decrypt failed");
    plaintext.resize(pt_len);
    return plaintext;
}

// Public backend dispatch implementations that core uses.
std::vector<unsigned char> randomBytes(size_t size) {
    return engineRandomBytes(size);
}

std::vector<unsigned char> deriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
) {
    return engineDeriveKey(password, salt, keySize, iterations);
}

EncryptResult encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
) {
    return engineEncrypt(plaintext, key, nonce, algorithm);
}

std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
) {
    return engineDecrypt(ciphertext, key, nonce, tag, algorithm);
}

} // namespace onc::core::backend
