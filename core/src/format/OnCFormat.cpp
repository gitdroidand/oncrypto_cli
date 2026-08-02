#include "format/OnCFormat.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace onc::format {

std::string getAlgorithmName(uint8_t id) {
    switch (static_cast<AlgorithmID>(id)) {
        case AlgorithmID::AES256_GCM:        return "AES-256-GCM";
        case AlgorithmID::ChaCha20_Poly1305: return "ChaCha20-Poly1305";
        case AlgorithmID::XChaCha20_Poly1305:return "XChaCha20-Poly1305";
        default: return "Unknown";
    }
}

std::string getKDFName(uint8_t id) {
    switch (static_cast<KDFID>(id)) {
        case KDFID::PBKDF2_SHA256: return "PBKDF2-SHA256";
        default: return "Unknown";
    }
}

std::vector<uint8_t> serialize(
    const OnCMetadata& metadata,
    const std::vector<uint8_t>& ciphertext
) {
    // Determine algorithm ID from name
    uint8_t algoId = 1;
    if (metadata.algorithmName.find("ChaCha20") != std::string::npos) {
        if (metadata.algorithmName.find("XChaCha20") != std::string::npos) {
            algoId = 3;
        } else {
            algoId = 2;
        }
    }
    
    OnCHeader header;
    header.magic = MAGIC;
    header.version = FORMAT_VERSION;
    header.algorithmId = algoId;
    header.kdfId = static_cast<uint8_t>(KDFID::PBKDF2_SHA256);
    header.iterations = metadata.iterations;
    header.nonceLength = static_cast<uint8_t>(metadata.nonce.size());
    
    std::copy(metadata.salt.begin(), metadata.salt.end(), header.salt.begin());
    
    std::vector<uint8_t> result;
    result.reserve(
        sizeof(OnCHeader) +
        metadata.nonce.size() +
        metadata.tag.size() +
        ciphertext.size()
    );
    
    const uint8_t* headerPtr = reinterpret_cast<const uint8_t*>(&header);
    result.insert(result.end(), headerPtr, headerPtr + sizeof(OnCHeader));
    result.insert(result.end(), metadata.nonce.begin(), metadata.nonce.end());
    result.insert(result.end(), metadata.tag.begin(), metadata.tag.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    
    return result;
}

OnCMetadata deserialize(
    const std::vector<uint8_t>& data,
    std::vector<uint8_t>& ciphertext
) {
    if (data.size() < sizeof(OnCHeader)) {
        throw std::runtime_error("OnC Format: Data too short for header");
    }
    
    OnCHeader header;
    std::memcpy(&header, data.data(), sizeof(OnCHeader));
    
    if (header.magic != MAGIC) {
        throw std::runtime_error("OnC Format: Invalid magic number");
    }
    
    if (header.version != FORMAT_VERSION) {
        throw std::runtime_error("OnC Format: Unsupported version");
    }
    
    OnCMetadata metadata;
    metadata.algorithmName = getAlgorithmName(header.algorithmId);
    metadata.kdfName = getKDFName(header.kdfId);
    metadata.iterations = header.iterations;
    metadata.salt.assign(header.salt.begin(), header.salt.end());
    
    size_t offset = sizeof(OnCHeader);
    size_t nonceLen = header.nonceLength;
    if (offset + nonceLen > data.size()) {
        throw std::runtime_error("OnC Format: Invalid nonce length");
    }
    metadata.nonce.assign(data.begin() + offset, data.begin() + offset + nonceLen);
    offset += nonceLen;
    
    size_t tagLen = 16;
    if (offset + tagLen > data.size()) {
        throw std::runtime_error("OnC Format: Invalid tag length");
    }
    metadata.tag.assign(data.begin() + offset, data.begin() + offset + tagLen);
    offset += tagLen;
    
    ciphertext.assign(data.begin() + offset, data.end());
    
    return metadata;
}

bool validateHeader(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(OnCHeader)) {
        return false;
    }
    
    OnCHeader header;
    std::memcpy(&header, data.data(), sizeof(OnCHeader));
    
    return (header.magic == MAGIC && header.version == FORMAT_VERSION);
}

} // namespace onc::format
