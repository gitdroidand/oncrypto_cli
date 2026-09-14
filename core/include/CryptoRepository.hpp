#pragma once

#include "oncrypto/Export.hpp"
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "algo/Algorithm.hpp"

namespace crypto {

using ByteView = std::span<const std::uint8_t>;

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
        ByteView data,
        std::string_view password
    );

    EncryptionResult encrypt(
        const std::vector<unsigned char>& data,
        const std::string& password
    );

    std::vector<unsigned char> decrypt(
        ByteView data,
        std::string_view password
    );

    std::vector<unsigned char> decrypt(
        const std::vector<unsigned char>& data,
        const std::string& password
    );

private:
    std::unique_ptr<Algorithm> selectAlgorithm(
        ByteView data
    );

    std::unique_ptr<Algorithm> selectAlgorithm(
        const std::vector<unsigned char>& data
    );
};

} // namespace crypto