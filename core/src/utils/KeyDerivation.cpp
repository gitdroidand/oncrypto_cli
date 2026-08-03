#include "utils/KeyDerivation.hpp"
#include "oncrypto/backend/Backend.hpp"
#include <stdexcept>
#include <stdexcept>

namespace crypto {

std::vector<unsigned char> deriveKey(
    const std::string& password,
    size_t keySize,
    const std::vector<unsigned char>& salt
) {
    if (salt.size() != 16) {
        throw std::runtime_error("Salt must be 16 bytes");
    }
    return onc::core::backend::deriveKey(password, salt, keySize, 100000);
}

std::vector<unsigned char> generateSalt(size_t size) {
    return onc::core::backend::randomBytes(size);
}

} // namespace crypto