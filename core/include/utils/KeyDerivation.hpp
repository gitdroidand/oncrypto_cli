#pragma once

#include <vector>
#include <string>

namespace crypto {

std::vector<unsigned char> deriveKey(
    const std::string& password,
    size_t keySize,
    const std::vector<unsigned char>& salt
);

std::vector<unsigned char> generateSalt(size_t size = 16);

} // namespace crypto