#pragma once

#include <vector>
#include <string>
#include <memory>
#include "algo/Algorithm.hpp"

namespace crypto {

struct EncryptionResult {
    std::vector<unsigned char> data;
    std::string algorithmName;
    std::string reason;
    std::vector<unsigned char> salt;
};

class CryptoRepository {
public:
    CryptoRepository();
    ~CryptoRepository();

    EncryptionResult encrypt(
        const std::vector<unsigned char>& data,
        const std::string& password
    );
    
    std::vector<unsigned char> decrypt(
        const std::vector<unsigned char>& data,
        const std::string& password
    );

private:
    std::unique_ptr<Algorithm> selectAlgorithm(
        const std::vector<unsigned char>& data
    );
};

} // namespace crypto
