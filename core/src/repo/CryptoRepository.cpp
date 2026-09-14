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
    ByteView data
) {
    if (data.size() < 1024) {
        return std::make_unique<XChaCha20>();
    }
    if (data.size() > 1024 * 1024) {
        return std::make_unique<AES256GCM>();
    }
    return std::make_unique<ChaCha20>();
}

std::unique_ptr<Algorithm> CryptoRepository::selectAlgorithm(
    const std::vector<unsigned char>& data
) {
    return selectAlgorithm(ByteView(data));
}

EncryptionResult CryptoRepository::encrypt(
    ByteView data,
    std::string_view password
) {
    auto algorithm = selectAlgorithm(data);
    auto info = algorithm->getInfo();

    auto salt = onc::core::backend::randomBytes(16);
    auto nonce = onc::core::backend::randomBytes(algorithm->getNonceSize());

    auto key = onc::core::backend::deriveKey(
        std::string(password),
        salt,
        algorithm->getKeySize(),
        100000
    );

    auto result = onc::core::backend::encrypt(
        std::vector<unsigned char>(data.begin(), data.end()),
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

    std::vector<uint8_t> serialized(
        sizeof(onc::format::OnCHeader) + metadata.nonce.size() + metadata.tag.size() + result.ciphertext.size()
    );
    onc::format::serialize_into(metadata, result.ciphertext, serialized);

    EncryptionResult output;
    output.data = std::move(serialized);
    output.algorithmName = info.name;
    output.reason = info.reason;
    output.salt = salt;

    return output;
}

EncryptionResult CryptoRepository::encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    return encrypt(ByteView(data), password);
}

std::vector<unsigned char> CryptoRepository::decrypt(
    ByteView data,
    std::string_view password
) {
    std::vector<unsigned char> ciphertext;
    auto metadata = onc::format::deserialize_view(data);
    ciphertext.assign(metadata.data.begin(), metadata.data.end());

    auto key = onc::core::backend::deriveKey(
        std::string(password),
        std::vector<unsigned char>(metadata.salt.begin(), metadata.salt.end()),
        32,
        metadata.iterations
    );

    return onc::core::backend::decrypt(
        ciphertext,
        key,
        std::vector<unsigned char>(metadata.nonce.begin(), metadata.nonce.end()),
        std::vector<unsigned char>(metadata.tag.begin(), metadata.tag.end()),
        metadata.algorithmName
    );
}

std::vector<unsigned char> CryptoRepository::decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
) {
    return decrypt(ByteView(data), password);
}

} // namespace crypto
