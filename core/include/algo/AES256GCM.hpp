#pragma once

#include "Algorithm.hpp"

namespace crypto {

class AES256GCM : public Algorithm {
public:
    std::vector<unsigned char> encrypt(
        const std::vector<unsigned char>& data,
        const std::vector<unsigned char>& key
    ) override;
    
    std::vector<unsigned char> decrypt(
        const std::vector<unsigned char>& data,
        const std::vector<unsigned char>& key
    ) override;
    
    AlgorithmInfo getInfo() const override;
    size_t getKeySize() const override { return 32; }
    size_t getNonceSize() const override { return 12; }
};

} // namespace crypto