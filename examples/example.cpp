#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <oncrypto/oncrypto.hpp>

using namespace crypto;

// ============================================================
// Helper function to print hex
// ============================================================
std::string toHex(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char c : data) {
        ss << std::setw(2) << static_cast<int>(c);
    }
    return ss.str();
}

// ============================================================
// Layer 1: Simple API Test
// ============================================================
void testSimpleAPI() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "📌 Layer 1: Simple API Test\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::string plaintext = "Hello from OnCrypto!";
    std::string password = "mysecret123";
    
    std::cout << "📝 Plaintext: " << plaintext << "\n";
    std::cout << "🔑 Password: " << password << "\n";
    
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    auto encrypted = crypto::encrypt(data, password);
    std::cout << "🔐 Encrypted size: " << encrypted.size() << " bytes\n";
    
    auto decrypted = crypto::decrypt(encrypted, password);
    std::string result(decrypted.begin(), decrypted.end());
    std::cout << "🔓 Decrypted: " << result << "\n";
    
    if (result == plaintext) {
        std::cout << "✅ Simple API: PASSED\n";
    } else {
        std::cout << "❌ Simple API: FAILED\n";
    }
}

// ============================================================
// Layer 1: File API Test
// ============================================================
void testFileAPI() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "📌 Layer 1: File API Test\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::string inputFile = "test_input.txt";
    std::string encFile = "test_encrypted.bin";
    std::string decFile = "test_decrypted.txt";
    
    // Create test file
    std::string content = "This is a test file content!";
    std::ofstream out(inputFile);
    out << content;
    out.close();
    std::cout << "📁 Created: " << inputFile << "\n";
    
    // Encrypt file
    bool encResult = crypto::encryptFile(inputFile, encFile, "filepass");
    if (encResult) {
        std::cout << "🔐 Encrypted: " << inputFile << " -> " << encFile << "\n";
    } else {
        std::cout << "❌ File encryption failed\n";
        return;
    }
    
    // Decrypt file
    bool decResult = crypto::decryptFile(encFile, decFile, "filepass");
    if (decResult) {
        std::cout << "🔓 Decrypted: " << encFile << " -> " << decFile << "\n";
    } else {
        std::cout << "❌ File decryption failed\n";
        return;
    }
    
    // Verify
    std::ifstream in(decFile);
    std::string decryptedContent((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
    in.close();
    
    std::cout << "📄 Original: " << content << "\n";
    std::cout << "📄 Decrypted: " << decryptedContent << "\n";
    
    if (content == decryptedContent) {
        std::cout << "✅ File API: PASSED\n";
    } else {
        std::cout << "❌ File API: FAILED\n";
    }
    
    std::remove(inputFile.c_str());
    std::remove(encFile.c_str());
    std::remove(decFile.c_str());
}

// ============================================================
// Layer 2: Builder API Test
// ============================================================
void testBuilderAPI() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "📌 Layer 2: Builder API Test\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::string plaintext = "Builder API test!";
    std::string password = "builderpass";
    
    std::cout << "📝 Plaintext: " << plaintext << "\n";
    std::cout << "🔑 Password: " << password << "\n";
    
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    
    auto encrypted = crypto::builder::Encryptor()
        .password(password)
        .algorithm(crypto::builder::Algorithm::XChaCha20)
        .iterations(100000)
        .encrypt(data);
    
    std::cout << "🔐 Encrypted size: " << encrypted.size() << " bytes\n";
    
    auto decrypted = crypto::builder::Decryptor()
        .password(password)
        .algorithm(crypto::builder::Algorithm::XChaCha20)
        .decrypt(encrypted);
    
    std::string result(decrypted.begin(), decrypted.end());
    std::cout << "🔓 Decrypted: " << result << "\n";
    
    if (result == plaintext) {
        std::cout << "✅ Builder API: PASSED\n";
    } else {
        std::cout << "❌ Builder API: FAILED\n";
    }
}

// ============================================================
// Layer 3: Advanced API Test
// ============================================================
void testAdvancedAPI() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "📌 Layer 3: Advanced API Test\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::string plaintext = "Advanced API test with full options!";
    std::string password = "advpass";
    
    std::cout << "📝 Plaintext: " << plaintext << "\n";
    std::cout << "🔑 Password: " << password << "\n";
    
    crypto::advanced::EncryptionOptions encOptions;
    encOptions.algorithm = crypto::builder::Algorithm::AES256_GCM;
    encOptions.kdf = crypto::advanced::KDF::PBKDF2;
    encOptions.iterations = 100000;
    encOptions.outputFormat = crypto::advanced::OutputFormat::Binary;
    encOptions.storeMetadata = true;
    
    crypto::advanced::DecryptionOptions decOptions;
    decOptions.algorithm = crypto::builder::Algorithm::AES256_GCM;
    decOptions.verifyIntegrity = true;
    
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    
    auto encrypted = crypto::advanced::encrypt(data, password, encOptions);
    std::cout << "🔐 Encrypted size: " << encrypted.size() << " bytes\n";
    
    auto decrypted = crypto::advanced::decrypt(encrypted, password, decOptions);
    std::string result(decrypted.begin(), decrypted.end());
    std::cout << "🔓 Decrypted: " << result << "\n";
    
    if (result == plaintext) {
        std::cout << "✅ Advanced API: PASSED\n";
    } else {
        std::cout << "❌ Advanced API: FAILED\n";
    }
}

// ============================================================
// Main
// ============================================================
int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║        OnCrypto v" << crypto::getVersion() << " - Library Test        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    
    std::cout << "\n📚 Library Info:\n";
    std::cout << "  Version:   " << crypto::getVersion() << "\n";
    std::cout << "  Algorithm: " << crypto::getAlgorithmName() << "\n";
    
    testSimpleAPI();
    testFileAPI();
    testBuilderAPI();
    testAdvancedAPI();
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "✅ All tests completed!\n";
    std::cout << std::string(60, '=') << "\n";
    
    return 0;
}
