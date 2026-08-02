#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <array>

namespace onc::format {

// ============================================================
// OnC Format Version 1
// ============================================================

constexpr std::array<uint8_t, 4> MAGIC = {'O', 'N', 'C', '1'};
constexpr uint8_t FORMAT_VERSION = 1;

enum class AlgorithmID : uint8_t {
    AES256_GCM = 1,
    ChaCha20_Poly1305 = 2,
    XChaCha20_Poly1305 = 3
};

enum class KDFID : uint8_t {
    PBKDF2_SHA256 = 1
};

constexpr uint32_t DEFAULT_ITERATIONS = 100000;

struct OnCHeader {
    std::array<uint8_t, 4> magic;          // "ONC1"
    uint8_t version;                        // 1
    uint8_t algorithmId;                    // AlgorithmID
    uint8_t kdfId;                          // KDFID
    uint32_t iterations;                    // PBKDF2 iterations
    std::array<uint8_t, 16> salt;           // 16 bytes
    uint8_t nonceLength;                    // 12 or 24
    // nonce follows header
    // tag follows nonce
    // ciphertext follows tag
};

struct OnCMetadata {
    std::string algorithmName;
    std::string kdfName;
    uint32_t iterations;
    std::vector<uint8_t> salt;
    std::vector<uint8_t> nonce;
    std::vector<uint8_t> tag;
};

// ============================================================
// Serialization / Deserialization
// ============================================================

std::vector<uint8_t> serialize(
    const OnCMetadata& metadata,
    const std::vector<uint8_t>& ciphertext
);

OnCMetadata deserialize(
    const std::vector<uint8_t>& data,
    std::vector<uint8_t>& ciphertext
);

bool validateHeader(const std::vector<uint8_t>& data);

std::string getAlgorithmName(uint8_t id);
std::string getKDFName(uint8_t id);

} // namespace onc::format
