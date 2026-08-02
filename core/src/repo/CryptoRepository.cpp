#include "CryptoRepository.hpp"
#include "algo/AES256GCM.hpp"
#include "algo/ChaCha20.hpp"
#include "algo/XChaCha20.hpp"
#include "utils/KeyDerivation.hpp"
#include "format/OnCFormat.hpp"
#include "oncrypto/backend/Backend.hpp"
#include <memory>
#include <stdexcept>
#include <cstring>

namespace crypto {

CryptoRepository::CryptoRepository() = default;
CryptoRepository::~CryptoRepository() = default;

std::unique_ptr<Algorithm> CryptoRepository::selectAlgorithm(
    const std::vector<unsigned char>& data
) {
    // XChaCha20 is now supported via Backend hack
    if (data.size() < 1024) {
        return std::make_unique<XChaCha20>();  // ← برگشت به XChaCha20
    }
    if (data.size() > 1024 * 1024) {
        return std::make_unique<AES256GCM>();
    }
    return std::make_unique<ChaCha20>();
}

EncryptionResult CryptoRepository::encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    auto algorithm = selectAlgorithm(data);
    auto info = algorithm->getInfo();
    
    auto salt = onc::core::backend::randomBytes(16);
    auto nonce = onc::core::backend::randomBytes(algorithm->getNonceSize());
    
    auto key = onc::core::backend::deriveKey(
        password,
        salt,
        algorithm->getKeySize(),
        100000
    );
    
    auto result = onc::core::backend::encrypt(
        data,
        key,
        nonce,
        info.name
    );
    
    onc::format::OnCMetadata metadata;
    metadata.algorithmName = info.name;
    metadata.kdfName = "PBKDF2-SHA256";
    metadata.iterations = 100000;
    metadata.salt = salt;
    metadata.nonce = nonce;
    metadata.tag = result.tag;
    
    auto serialized = onc::format::serialize(metadata, result.ciphertext);
    
    EncryptionResult output;
    output.data = serialized;
    output.algorithmName = info.name;
    output.reason = info.reason;
    output.salt = salt;
    
    return output;
}

std::vector<unsigned char> CryptoRepository::decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    std::vector<unsigned char> ciphertext;
    auto metadata = onc::format::deserialize(data, ciphertext);
    
    auto key = onc::core::backend::deriveKey(
        password,
        metadata.salt,
        32,
        metadata.iterations
    );
    
    return onc::core::backend::decrypt(
        ciphertext,
        key,
        metadata.nonce,
        metadata.tag,
        metadata.algorithmName
    );
}

} // namespace crypto
