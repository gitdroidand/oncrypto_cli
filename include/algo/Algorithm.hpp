#pragma once

#include <vector>
#include <string>
#include <memory>

namespace crypto {

enum class AlgorithmType {
    AES256_GCM,
    ChaCha20_Poly1305,
    XChaCha20_Poly1305,
    Unknown
};

struct AlgorithmInfo {
    AlgorithmType type;
    std::string name;
    std::string reason;
    int securityLevel; // 1-10
};

class Algorithm {
public:
    virtual ~Algorithm() = default;
    
    virtual std::vector<unsigned char> encrypt(
        const std::vector<unsigned char>& data,
        const std::vector<unsigned char>& key
    ) = 0;
    
    virtual std::vector<unsigned char> decrypt(
        const std::vector<unsigned char>& data,
        const std::vector<unsigned char>& key
    ) = 0;
    
    virtual AlgorithmInfo getInfo() const = 0;
    virtual size_t getKeySize() const = 0;
    virtual size_t getNonceSize() const = 0;
};

} // namespace crypto