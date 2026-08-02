#include <oncrypto/oncrypto.hpp>
#include "CryptoRepository.hpp"

namespace crypto {

std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    CryptoRepository repo;
    auto result = repo.encrypt(data, password);
    return result.data;
}

std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    CryptoRepository repo;
    return repo.decrypt(data, password);
}

std::string getAlgorithmName() {
    // می‌توانید آخرین الگوریتم استفاده شده را برگردانید
    return "AES-256-GCM / ChaCha20 / XChaCha20 (auto-selected)";
}

std::string getVersion() {
    return "1.3.0";
}

} // namespace crypto