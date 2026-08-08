#pragma once

#include "oncrypto/Export.hpp"
#include <vector>
#include <string>
#include <memory>
#include "algo/Algorithm.hpp"

namespace crypto {

struct ONCRYPTO_API EncryptionResult {
    std::vector<unsigned char> data;
    std::string algorithmName;
    std::string reason;
    std::vector<unsigned char> salt;
};

class ONCRYPTO_API CryptoRepository {
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