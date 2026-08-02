#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "CryptoRepository.hpp"
#include "utils/FileUtils.hpp"
#include "utils/KeyDerivation.hpp"

using namespace crypto;

TEST_CASE("Encryption/Decryption") {
    CryptoRepository repo;
    std::string password = "test123";
    std::string plaintext = "Hello, World!";
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    
    auto result = repo.encrypt(data, password);
    REQUIRE(!result.data.empty());
    
    auto decrypted = repo.decrypt(result.data, password);
    REQUIRE(!decrypted.empty());
    
    std::string decryptedText(decrypted.begin(), decrypted.end());
    CHECK(decryptedText == plaintext);
}

TEST_CASE("File I/O with encryption") {
    CryptoRepository repo;
    std::string password = "fileTest";
    std::string plaintext = "This is a test file content!";
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    
    auto result = repo.encrypt(data, password);
    REQUIRE(!result.data.empty());
    
    std::string tempFile = "test_encrypted.bin";
    REQUIRE(writeFile(tempFile, result.data));
    
    auto encryptedData = readFile(tempFile);
    REQUIRE(!encryptedData.empty());
    
    auto decrypted = repo.decrypt(encryptedData, password);
    std::string decryptedText(decrypted.begin(), decrypted.end());
    CHECK(decryptedText == plaintext);
}

TEST_CASE("Algorithm selection") {
    CryptoRepository repo;
    std::string password = "algo";
    
    SUBCASE("Small data -> XChaCha20") {
        std::vector<unsigned char> small(100, 'A');
        auto result = repo.encrypt(small, password);
        CHECK(result.algorithmName.find("XChaCha20") != std::string::npos);
    }
    
    SUBCASE("Large data -> AES-256-GCM") {
        std::vector<unsigned char> large(2 * 1024 * 1024, 'A');
        auto result = repo.encrypt(large, password);
        CHECK(result.algorithmName.find("AES-256-GCM") != std::string::npos);
    }
}

TEST_CASE("Key Derivation") {
    std::string password = "myPassword";
    auto salt = generateSalt(16);
    REQUIRE(salt.size() == 16);
    
    auto key = deriveKey(password, 32, salt);
    REQUIRE(key.size() == 32);
    
    auto key2 = deriveKey(password, 32, salt);
    CHECK(key == key2);
    
    auto key3 = deriveKey("different", 32, salt);
    CHECK(key != key3);
}

TEST_CASE("Hex conversion") {
    std::vector<unsigned char> bytes = {0x01, 0x02, 0x03, 0x04, 0xFF};
    auto hex = bytesToHex(bytes);
    CHECK(hex == "01020304ff");
    
    auto decoded = hexToBytes(hex);
    CHECK(decoded == bytes);
}

TEST_CASE("Empty data handling") {
    CryptoRepository repo;
    std::string password = "empty";
    std::vector<unsigned char> empty;
    
    auto result = repo.encrypt(empty, password);
    REQUIRE(!result.data.empty());
    
    auto decrypted = repo.decrypt(result.data, password);
    CHECK(decrypted.empty());
}

TEST_CASE("Wrong password rejection") {
    CryptoRepository repo;
    std::string password = "correct";
    std::string wrong = "wrong";
    std::vector<unsigned char> data = {1, 2, 3, 4, 5};
    
    auto result = repo.encrypt(data, password);
    REQUIRE_THROWS_AS(repo.decrypt(result.data, wrong), std::runtime_error);
}

TEST_CASE("Large data (10MB)") {
    CryptoRepository repo;
    std::string password = "large";
    std::vector<unsigned char> largeData(10 * 1024 * 1024);
    
    for (size_t i = 0; i < largeData.size(); ++i) {
        largeData[i] = static_cast<unsigned char>(i % 256);
    }
    
    auto result = repo.encrypt(largeData, password);
    REQUIRE(!result.data.empty());
    
    auto decrypted = repo.decrypt(result.data, password);
    REQUIRE(decrypted.size() == largeData.size());
    
    for (size_t i = 0; i < 1000; ++i) {
        CHECK(decrypted[i] == largeData[i]);
    }
}