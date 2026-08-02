#include <iostream>
#include <vector>
#include <string>
#include <oncrypto/oncrypto.hpp>

int main() {
    std::cout << "OnCrypto v" << crypto::getVersion() << std::endl;
    std::cout << "Algorithm: " << crypto::getAlgorithmName() << std::endl;
    
    std::string plaintext = "Hello from OnCrypto!";
    std::string password = "mysecret";
    
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    auto encrypted = crypto::encrypt(data, password);
    
    std::cout << "Encrypted size: " << encrypted.size() << " bytes" << std::endl;
    
    auto decrypted = crypto::decrypt(encrypted, password);
    std::string result(decrypted.begin(), decrypted.end());
    
    std::cout << "Decrypted: " << result << std::endl;
    
    return 0;
}
