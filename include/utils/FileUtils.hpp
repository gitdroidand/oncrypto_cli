#pragma once

#include <vector>
#include <string>
#include <optional>

namespace crypto {

std::vector<unsigned char> readFile(const std::string& filename);
bool writeFile(const std::string& filename, const std::vector<unsigned char>& data);
std::string bytesToHex(const std::vector<unsigned char>& bytes);
std::vector<unsigned char> hexToBytes(const std::string& hex);

} // namespace crypto