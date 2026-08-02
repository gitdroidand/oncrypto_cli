#include "oncrypto/oncrypto.hpp"
#include "CryptoRepository.hpp"
#include "utils/FileUtils.hpp"

namespace crypto::advanced {

// ============================================================
// Advanced API Implementation
// ============================================================

std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password,
    const EncryptionOptions& options
) {
    // For now, use simple API with password
    // Future: use all options (algorithm, kdf, iterations, etc.)
    (void)options; // Suppress unused warning for now
    
    return crypto::encrypt(data, password);
}

std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password,
    const DecryptionOptions& options
) {
    (void)options; // Suppress unused warning for now
    
    return crypto::decrypt(data, password);
}

} // namespace crypto::advanced
