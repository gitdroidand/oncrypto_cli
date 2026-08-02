#include "utils/KeyDerivation.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>

namespace crypto {

std::vector<unsigned char> deriveKey(
    const std::string& password,
    size_t keySize,
    const std::vector<unsigned char>& salt
) {
    std::vector<unsigned char> key(keySize);
    
    if (salt.size() != 16) {
        throw std::runtime_error("Salt must be 16 bytes");
    }
    
    if (PKCS5_PBKDF2_HMAC(
        password.c_str(),
        password.length(),
        salt.data(),
        salt.size(),
        100000, // iterations
        EVP_sha256(),
        keySize,
        key.data()
    ) != 1) {
        throw std::runtime_error("Key derivation failed");
    }
    
    return key;
}

std::vector<unsigned char> generateSalt(size_t size) {
    std::vector<unsigned char> salt(size);
    if (RAND_bytes(salt.data(), salt.size()) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }
    return salt;
}

} // namespace crypto