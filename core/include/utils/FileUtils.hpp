#pragma once

#include "oncrypto/Export.hpp"
#include <vector>
#include <string>
#include <optional>

namespace crypto {

ONCRYPTO_API std::vector<unsigned char> readFile(const std::string& filename);
ONCRYPTO_API bool writeFile(const std::string& filename, const std::vector<unsigned char>& data);
ONCRYPTO_API std::string bytesToHex(const std::vector<unsigned char>& bytes);
ONCRYPTO_API std::vector<unsigned char> hexToBytes(const std::string& hex);

} // namespace crypto