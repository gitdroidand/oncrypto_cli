// core/include/oncrypto/backend/EngineBackend.hpp
#pragma once

#include "oncrypto/backend/Backend.hpp"  // برای EncryptResult و namespace

#include <vector>
#include <string>
#include <cstddef>

namespace onc::core::backend {

// functions
std::vector<unsigned char> engineRandomBytes(size_t size);

std::vector<unsigned char> engineDeriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
);

EncryptResult engineEncrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
);

std::vector<unsigned char> engineDecrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
);

// توابع عمومی که core ازشون استفاده میکنه
std::vector<unsigned char> randomBytes(size_t size);

std::vector<unsigned char> deriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t keySize,
    size_t iterations
);

EncryptResult encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::string& algorithm
);

std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    const std::vector<unsigned char>& tag,
    const std::string& algorithm
);

} // namespace onc::core::backend