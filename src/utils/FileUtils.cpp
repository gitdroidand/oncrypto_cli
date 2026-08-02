#include "utils/FileUtils.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace crypto {

std::vector<unsigned char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        return {};
    }
    
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<unsigned char> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

bool writeFile(const std::string& filename, const std::vector<unsigned char>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

std::string bytesToHex(const std::vector<unsigned char>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<unsigned char> hexToBytes(const std::string& hex) {
    if (hex.length() % 2 != 0) {
        throw std::runtime_error("Hex string must have even length");
    }
    
    std::vector<unsigned char> bytes;
    bytes.reserve(hex.length() / 2);
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(
            std::stoi(byteString, nullptr, 16)
        );
        bytes.push_back(byte);
    }
    return bytes;
}
} // namespace crypto