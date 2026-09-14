
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
    (void)options;
    CryptoRepository repo;
    return repo.encrypt(ByteView(data), password).data;
}

std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password,
    const DecryptionOptions& options
) {
    (void)options;
    CryptoRepository repo;
    return repo.decrypt(ByteView(data), password);
}

} // namespace crypto::advanced
