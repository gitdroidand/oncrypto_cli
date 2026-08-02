#pragma once

#include <vector>
#include <string>

namespace onc::core::backend {

// ============================================================
// Backend Interface
// Core says: "Do this." Backend says: "Done."
// ============================================================

// 1. Random bytes
std::vector<unsigned char> randomBytes(size_t size);

// 2. Key Derivation
std::vector<unsigned char> deriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
);

// 3. Encrypt primitive
struct EncryptResult {
    std::vector<unsigned char> ciphertext;
    std::vector<unsigned char> tag;
};

EncryptResult encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm  // "AES-256-GCM" | "ChaCha20-Poly1305" | "XChaCha20-Poly1305"
);

// 4. Decrypt primitive
std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
);

} // namespace onc::core::backend
