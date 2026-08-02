#include "oncrypto/oncrypto.hpp"
#include "utils/FileUtils.hpp"
#include <stdexcept>

namespace crypto::builder {

// ============================================================
// Encryptor Implementation
// ============================================================

Encryptor& Encryptor::password(const std::string& pwd) {
    password_ = pwd;
    return *this;
}

Encryptor& Encryptor::algorithm(Algorithm algo) {
    algorithm_ = algo;
    return *this;
}

Encryptor& Encryptor::iterations(int iter) {
    iterations_ = iter;
    return *this;
}

std::vector<unsigned char> Encryptor::encrypt(
    const std::vector<unsigned char>& data
) {
    if (password_.empty()) {
        throw std::runtime_error("Password not set. Use .password()");
    }
    
    // For now, use simple API with the password
    // Future: use algorithm_ and iterations_
    return crypto::encrypt(data, password_);
}

bool Encryptor::encryptFile(
    const std::string& input,
    const std::string& output
) {
    auto data = readFile(input);
    if (data.empty()) return false;
    
    auto encrypted = encrypt(data);
    return writeFile(output, encrypted);
}

// ============================================================
// Decryptor Implementation
// ============================================================

Decryptor& Decryptor::password(const std::string& pwd) {
    password_ = pwd;
    return *this;
}

Decryptor& Decryptor::algorithm(Algorithm algo) {
    algorithm_ = algo;
    return *this;
}

std::vector<unsigned char> Decryptor::decrypt(
    const std::vector<unsigned char>& data
) {
    if (password_.empty()) {
        throw std::runtime_error("Password not set. Use .password()");
    }
    
    return crypto::decrypt(data, password_);
}

bool Decryptor::decryptFile(
    const std::string& input,
    const std::string& output
) {
    auto data = readFile(input);
    if (data.empty()) return false;
    
    auto decrypted = decrypt(data);
    return writeFile(output, decrypted);
}

} // namespace crypto::builder
