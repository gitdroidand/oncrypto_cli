#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <array>
#include <cstddef>
#include <span>

namespace onc::format {

// ============================================================
// OnC Format Version 1
// ============================================================

constexpr std::array<uint8_t, 4> MAGIC = {'O', 'N', 'C', '1'};
constexpr uint8_t FORMAT_VERSION = 1;

enum class AlgorithmID : uint8_t {
    AES256_GCM         = 1,
    ChaCha20_Poly1305  = 2,
    XChaCha20_Poly1305 = 3
};

enum class KDFID : uint8_t {
    PBKDF2_SHA256 = 1
};

constexpr uint32_t DEFAULT_ITERATIONS = 100000;

// ============================================================
// ONC1 Header
// ============================================================
//
// Binary layout:
//
//   0..3   magic           4 bytes
//   4      version         1 byte
//   5      algorithmId     1 byte
//   6      kdfId           1 byte
//   7..10  iterations      4 bytes
//   11..26 salt            16 bytes
//   27     nonceLength     1 byte
//
// Total: 28 bytes
//
// IMPORTANT:
// This structure is serialized directly as binary data.
// Therefore its layout MUST NOT depend on compiler padding.
//
// ============================================================

#pragma pack(push, 1)

struct OnCHeader {
    std::array<uint8_t, 4> magic;   // "ONC1"       : 4
    uint8_t version;                 // format       : 1
    uint8_t algorithmId;             // algorithm    : 1
    uint8_t kdfId;                   // KDF          : 1
    uint32_t iterations;             // PBKDF2       : 4
    std::array<uint8_t, 16> salt;    // salt         : 16
    uint8_t nonceLength;             // nonce length : 1

    // nonce follows header
    // tag follows nonce
    // ciphertext follows tag
};

#pragma pack(pop)

// ONC1 binary header is exactly 28 bytes.
static_assert(
    sizeof(OnCHeader) == 28,
    "OnCHeader layout changed; ONC1 compatibility would break"
);

static_assert(
    offsetof(OnCHeader, magic) == 0,
    "Invalid OnCHeader::magic offset"
);

static_assert(
    offsetof(OnCHeader, version) == 4,
    "Invalid OnCHeader::version offset"
);

static_assert(
    offsetof(OnCHeader, algorithmId) == 5,
    "Invalid OnCHeader::algorithmId offset"
);

static_assert(
    offsetof(OnCHeader, kdfId) == 6,
    "Invalid OnCHeader::kdfId offset"
);

static_assert(
    offsetof(OnCHeader, iterations) == 7,
    "Invalid OnCHeader::iterations offset"
);

static_assert(
    offsetof(OnCHeader, salt) == 11,
    "Invalid OnCHeader::salt offset"
);

static_assert(
    offsetof(OnCHeader, nonceLength) == 27,
    "Invalid OnCHeader::nonceLength offset"
);

// ============================================================
// Metadata
// ============================================================

struct OnCMetadata {
    std::string algorithmName;
    std::string kdfName;
    uint32_t iterations = DEFAULT_ITERATIONS;

    std::vector<uint8_t> salt;
    std::vector<uint8_t> nonce;
    std::vector<uint8_t> tag;
};

struct OnCView {
    std::span<const std::uint8_t> data;
    std::span<const std::uint8_t> nonce;
    std::span<const std::uint8_t> tag;
    std::span<const std::uint8_t> salt;
    std::uint32_t iterations = DEFAULT_ITERATIONS;
    std::string algorithmName;
    std::string kdfName;
};

// ============================================================
// Serialization / Deserialization
// ============================================================

std::vector<uint8_t> serialize(
    const OnCMetadata& metadata,
    const std::vector<uint8_t>& ciphertext
);

void serialize_into(
    const OnCMetadata& metadata,
    std::span<const std::uint8_t> ciphertext,
    std::span<std::uint8_t> destination
);

OnCMetadata deserialize(
    const std::vector<uint8_t>& data,
    std::vector<uint8_t>& ciphertext
);

OnCView deserialize_view(
    std::span<const std::uint8_t> data
);

bool validateHeader(
    const std::vector<uint8_t>& data
);

std::string getAlgorithmName(
    uint8_t id
);

std::string getKDFName(
    uint8_t id
);

} // namespace onc::format