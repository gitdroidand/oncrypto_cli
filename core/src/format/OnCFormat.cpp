#include "format/OnCFormat.hpp"

#include <cstring>
#include <stdexcept>
#include <limits>

namespace onc::format {

namespace {

constexpr size_t HEADER_SIZE = sizeof(OnCHeader);
constexpr size_t SALT_SIZE = 16;
constexpr size_t TAG_SIZE = 16;

constexpr bool isValidAlgorithmId(uint8_t id) noexcept {
    return id >= static_cast<uint8_t>(AlgorithmID::AES256_GCM) &&
           id <= static_cast<uint8_t>(AlgorithmID::XChaCha20_Poly1305);
}

constexpr bool isValidKdfId(uint8_t id) noexcept {
    return id == static_cast<uint8_t>(KDFID::PBKDF2_SHA256);
}

constexpr bool isValidNonceLength(uint8_t algorithmId,
                                  uint8_t nonceLength) noexcept {
    switch (static_cast<AlgorithmID>(algorithmId)) {
        case AlgorithmID::AES256_GCM:
        case AlgorithmID::ChaCha20_Poly1305:
            return nonceLength == 12;

        case AlgorithmID::XChaCha20_Poly1305:
            return nonceLength == 24;

        default:
            return false;
    }
}

uint8_t algorithmIdFromName(const std::string& name) {
    if (name == "AES-256-GCM") {
        return static_cast<uint8_t>(AlgorithmID::AES256_GCM);
    }

    if (name == "ChaCha20-Poly1305") {
        return static_cast<uint8_t>(AlgorithmID::ChaCha20_Poly1305);
    }

    if (name == "XChaCha20-Poly1305") {
        return static_cast<uint8_t>(AlgorithmID::XChaCha20_Poly1305);
    }

    throw std::runtime_error(
        "OnC Format: Unsupported algorithm"
    );
}

} // namespace

// ============================================================
// Algorithm / KDF names
// ============================================================

std::string getAlgorithmName(uint8_t id) {
    switch (static_cast<AlgorithmID>(id)) {
        case AlgorithmID::AES256_GCM:
            return "AES-256-GCM";

        case AlgorithmID::ChaCha20_Poly1305:
            return "ChaCha20-Poly1305";

        case AlgorithmID::XChaCha20_Poly1305:
            return "XChaCha20-Poly1305";

        default:
            return "Unknown";
    }
}

std::string getKDFName(uint8_t id) {
    switch (static_cast<KDFID>(id)) {
        case KDFID::PBKDF2_SHA256:
            return "PBKDF2-SHA256";

        default:
            return "Unknown";
    }
}

// ============================================================
// Serialization
// ============================================================

std::vector<uint8_t> serialize(
    const OnCMetadata& metadata,
    const std::vector<uint8_t>& ciphertext
) {
    std::vector<uint8_t> result(
        HEADER_SIZE + metadata.nonce.size() + metadata.tag.size() + ciphertext.size()
    );
    serialize_into(metadata, ciphertext, result);
    return result;
}

void serialize_into(
    const OnCMetadata& metadata,
    std::span<const std::uint8_t> ciphertext,
    std::span<std::uint8_t> destination
) {
    if (metadata.salt.size() != SALT_SIZE) {
        throw std::runtime_error("OnC Format: Salt must be exactly 16 bytes");
    }
    if (metadata.tag.size() != TAG_SIZE) {
        throw std::runtime_error("OnC Format: Authentication tag must be exactly 16 bytes");
    }
    if (metadata.nonce.empty()) {
        throw std::runtime_error("OnC Format: Nonce cannot be empty");
    }
    if (metadata.nonce.size() > std::numeric_limits<uint8_t>::max()) {
        throw std::runtime_error("OnC Format: Nonce is too large");
    }

    const uint8_t algoId = algorithmIdFromName(metadata.algorithmName);
    const uint8_t nonceLength = static_cast<uint8_t>(metadata.nonce.size());
    if (!isValidNonceLength(algoId, nonceLength)) {
        throw std::runtime_error("OnC Format: Invalid nonce length for algorithm");
    }
    if (metadata.iterations == 0) {
        throw std::runtime_error("OnC Format: PBKDF2 iterations must be greater than zero");
    }

    const size_t totalSize = HEADER_SIZE + metadata.nonce.size() + metadata.tag.size() + ciphertext.size();
    if (destination.size() < totalSize) {
        throw std::runtime_error("OnC Format: destination buffer is too small");
    }

    OnCHeader header{};
    header.magic = MAGIC;
    header.version = FORMAT_VERSION;
    header.algorithmId = algoId;
    header.kdfId = static_cast<uint8_t>(KDFID::PBKDF2_SHA256);
    header.iterations = metadata.iterations;
    std::memcpy(header.salt.data(), metadata.salt.data(), SALT_SIZE);
    header.nonceLength = nonceLength;

    size_t offset = 0;
    std::memcpy(destination.data() + offset, &header, HEADER_SIZE);
    offset += HEADER_SIZE;
    std::memcpy(destination.data() + offset, metadata.nonce.data(), metadata.nonce.size());
    offset += metadata.nonce.size();
    std::memcpy(destination.data() + offset, metadata.tag.data(), metadata.tag.size());
    offset += metadata.tag.size();
    if (!ciphertext.empty()) {
        std::memcpy(destination.data() + offset, ciphertext.data(), ciphertext.size());
    }
}

// ============================================================
// Deserialization
// ============================================================

OnCMetadata deserialize(
    const std::vector<uint8_t>& data,
    std::vector<uint8_t>& ciphertext
) {
    const auto view = deserialize_view(data);
    ciphertext.assign(view.data.begin(), view.data.end());

    OnCMetadata metadata;
    metadata.algorithmName = view.algorithmName;
    metadata.kdfName = view.kdfName;
    metadata.iterations = view.iterations;
    metadata.salt.assign(view.salt.begin(), view.salt.end());
    metadata.nonce.assign(view.nonce.begin(), view.nonce.end());
    metadata.tag.assign(view.tag.begin(), view.tag.end());
    return metadata;
}

OnCView deserialize_view(std::span<const std::uint8_t> data) {
    if (data.size() < HEADER_SIZE) {
        throw std::runtime_error("OnC Format: Data too short for header");
    }

    OnCHeader header{};
    std::memcpy(&header, data.data(), HEADER_SIZE);

    if (header.magic != MAGIC) {
        throw std::runtime_error("OnC Format: Invalid magic number");
    }
    if (header.version != FORMAT_VERSION) {
        throw std::runtime_error("OnC Format: Unsupported version");
    }
    if (!isValidAlgorithmId(header.algorithmId)) {
        throw std::runtime_error("OnC Format: Unsupported algorithm ID");
    }
    if (!isValidKdfId(header.kdfId)) {
        throw std::runtime_error("OnC Format: Unsupported KDF ID");
    }
    if (header.iterations == 0) {
        throw std::runtime_error("OnC Format: Invalid PBKDF2 iteration count");
    }
    if (!isValidNonceLength(header.algorithmId, header.nonceLength)) {
        throw std::runtime_error("OnC Format: Invalid nonce length");
    }

    const size_t nonceLen = static_cast<size_t>(header.nonceLength);
    size_t offset = HEADER_SIZE;
    if (nonceLen > data.size() - offset) {
        throw std::runtime_error("OnC Format: Invalid nonce length");
    }

    const auto nonce = data.subspan(offset, nonceLen);
    offset += nonceLen;

    if (TAG_SIZE > data.size() - offset) {
        throw std::runtime_error("OnC Format: Invalid authentication tag");
    }

    const auto tag = data.subspan(offset, TAG_SIZE);
    offset += TAG_SIZE;

    const auto ciphertext = data.subspan(offset, data.size() - offset);
    const auto salt = data.subspan(offsetof(OnCHeader, salt), SALT_SIZE);

    OnCView view{};
    view.data = ciphertext;
    view.nonce = nonce;
    view.tag = tag;
    view.salt = salt;
    view.iterations = header.iterations;
    view.algorithmName = getAlgorithmName(header.algorithmId);
    view.kdfName = getKDFName(header.kdfId);
    return view;
}

// ============================================================
// Header validation
// ============================================================

bool validateHeader(
    const std::vector<uint8_t>& data
) {
    if (data.size() < HEADER_SIZE) {
        return false;
    }

    OnCHeader header{};

    std::memcpy(
        &header,
        data.data(),
        HEADER_SIZE
    );

    if (header.magic != MAGIC) {
        return false;
    }

    if (header.version != FORMAT_VERSION) {
        return false;
    }

    if (!isValidAlgorithmId(header.algorithmId)) {
        return false;
    }

    if (!isValidKdfId(header.kdfId)) {
        return false;
    }

    if (header.iterations == 0) {
        return false;
    }

    if (!isValidNonceLength(
            header.algorithmId,
            header.nonceLength)) {
        return false;
    }

    const size_t nonceLength =
        static_cast<size_t>(header.nonceLength);

    const size_t required =
        HEADER_SIZE +
        nonceLength +
        TAG_SIZE;

    if (required < HEADER_SIZE) {
        return false;
    }

    return data.size() >= required;
}

} // namespace onc::format