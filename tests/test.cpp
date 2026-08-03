#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "CryptoRepository.hpp"
#include "utils/FileUtils.hpp"
#include "utils/KeyDerivation.hpp"
#include "format/OnCFormat.hpp"
#include "oncrypto_engine.h"

using namespace crypto;

TEST_CASE("Key Derivation") {
    std::string password = "mySecurePassword123!";
    auto salt = generateSalt(16);
    REQUIRE(salt.size() == 16);
    
    auto key = deriveKey(password, 32, salt);
    REQUIRE(key.size() == 32);
    
    auto key2 = deriveKey(password, 32, salt);
    REQUIRE(key == key2);
    
    auto key3 = deriveKey("differentPassword", 32, salt);
    REQUIRE(key != key3);
}

TEST_CASE("Engine C ABI version API") {
    REQUIRE(oncrypto_engine_version_major() >= 1);
    REQUIRE(oncrypto_engine_version_minor() >= 0);
    REQUIRE(oncrypto_engine_version_string() != nullptr);
}

TEST_CASE("Engine C ABI AEAD and HMAC") {
    std::vector<unsigned char> nonce(12);
    REQUIRE(oncrypto_engine_random_bytes(nonce.data(), nonce.size()) == 0);

    std::string password = "engineTestPassword";
    std::vector<unsigned char> key(32);
    REQUIRE(oncrypto_engine_pbkdf2_hmac_sha256(
        password.c_str(),
        nonce.data(),
        nonce.size(),
        100000,
        key.data(),
        key.size()
    ) == 0);

    std::string plaintext = "Engine C ABI AEAD test";
    std::vector<unsigned char> ciphertext(plaintext.size());
    std::vector<unsigned char> tag(16);
    size_t ct_len = ciphertext.size();
    size_t tag_len = tag.size();

    REQUIRE(oncrypto_engine_aead_encrypt(
        "AES-256-GCM",
        key.data(), key.size(),
        nonce.data(), nonce.size(),
        reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size(),
        ciphertext.data(), &ct_len,
        tag.data(), &tag_len
    ) == 0);

    ciphertext.resize(ct_len);
    tag.resize(tag_len);

    std::vector<unsigned char> decrypted(ciphertext.size());
    size_t pt_len = decrypted.size();
    REQUIRE(oncrypto_engine_aead_decrypt(
        "AES-256-GCM",
        key.data(), key.size(),
        nonce.data(), nonce.size(),
        ciphertext.data(), ciphertext.size(),
        tag.data(), tag.size(),
        decrypted.data(), &pt_len
    ) == 0);

    decrypted.resize(pt_len);
    std::string decryptedText(decrypted.begin(), decrypted.end());
    REQUIRE(decryptedText == plaintext);

    std::vector<unsigned char> hmac(32);
    size_t hmac_len = hmac.size();
    REQUIRE(oncrypto_engine_hmac_sha256(
        key.data(), key.size(),
        reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size(),
        hmac.data(), &hmac_len
    ) == 0);
    REQUIRE(hmac_len == 32);
}

TEST_CASE("Hex conversion") {
    std::vector<unsigned char> bytes = {0x01, 0x02, 0x03, 0x04, 0xFF};
    auto hex = bytesToHex(bytes);
    REQUIRE(hex == "01020304ff");
    
    auto decoded = hexToBytes(hex);
    REQUIRE(decoded == bytes);
}

TEST_CASE("OnC Format validation") {
    CryptoRepository repo;
    std::string password = "formatTest";
    std::string plaintext = "Testing OnC Format!";
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    
    auto result = repo.encrypt(data, password);
    
    // Validate header
    REQUIRE(onc::format::validateHeader(result.data));
    
    // Extract and verify
    std::vector<unsigned char> ciphertext;
    auto metadata = onc::format::deserialize(result.data, ciphertext);
    
    bool isValidAlgo = (metadata.algorithmName == "XChaCha20-Poly1305") ||
                       (metadata.algorithmName == "ChaCha20-Poly1305") ||
                       (metadata.algorithmName == "AES-256-GCM");
    REQUIRE(isValidAlgo);
    
    REQUIRE(metadata.kdfName == "PBKDF2-SHA256");
    REQUIRE(metadata.iterations == 100000);
    REQUIRE(metadata.salt.size() == 16);
    
    bool isValidNonce = (metadata.nonce.size() == 12) || (metadata.nonce.size() == 24);
    REQUIRE(isValidNonce);
    
    REQUIRE(metadata.tag.size() == 16);
    REQUIRE(ciphertext.size() == data.size());
}

TEST_CASE("Wrong password rejection with OnC Format") {
    CryptoRepository repo;
    std::string password = "correct";
    std::string wrong = "wrong";
    std::string plaintext = "Secret";
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    
    auto result = repo.encrypt(data, password);
    REQUIRE(onc::format::validateHeader(result.data));
    
    bool caught = false;
    try {
        repo.decrypt(result.data, wrong);
    } catch (const std::exception&) {
        caught = true;
    }
    REQUIRE(caught);
}

TEST_CASE("Invalid OnC header rejects malformed data") {
    std::vector<unsigned char> invalid = {'N', 'O', 'N', 'C'};
    REQUIRE(!onc::format::validateHeader(invalid));

    CryptoRepository repo;
    bool caught = false;
    try {
        repo.decrypt(invalid, "password");
    } catch (const std::exception&) {
        caught = true;
    }
    REQUIRE(caught);
}

TEST_CASE("Truncated encrypted data fails decryption") {
    CryptoRepository repo;
    std::string password = "truncate";
    std::vector<unsigned char> data = {'T', 'e', 's', 't'};

    auto encrypted = repo.encrypt(data, password);
    REQUIRE(onc::format::validateHeader(encrypted.data));

    encrypted.data.resize(encrypted.data.size() / 2);
    bool caught = false;
    try {
        repo.decrypt(encrypted.data, password);
    } catch (const std::exception&) {
        caught = true;
    }
    REQUIRE(caught);
}

TEST_CASE("Corrupted ciphertext fails authentication") {
    CryptoRepository repo;
    std::string password = "corrupt";
    std::vector<unsigned char> data = {'d','a','t','a'};

    auto encrypted = repo.encrypt(data, password);
    REQUIRE(onc::format::validateHeader(encrypted.data));
    encrypted.data.back() ^= 0xFF;

    bool caught = false;
    try {
        repo.decrypt(encrypted.data, password);
    } catch (const std::exception&) {
        caught = true;
    }
    REQUIRE(caught);
}

TEST_CASE("Empty data with OnC Format") {
    CryptoRepository repo;
    std::string password = "empty";
    std::vector<unsigned char> empty;
    
    auto result = repo.encrypt(empty, password);
    REQUIRE(!result.data.empty());
    REQUIRE(onc::format::validateHeader(result.data));
    
    auto decrypted = repo.decrypt(result.data, password);
    REQUIRE(decrypted.empty());
}

TEST_CASE("Encryption/Decryption") {
    CryptoRepository repo;
    std::string password = "testPassword123";
    std::string plaintext = "Hello, World!";
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    
    auto result = repo.encrypt(data, password);
    REQUIRE(!result.data.empty());
    REQUIRE(onc::format::validateHeader(result.data));
    
    auto decrypted = repo.decrypt(result.data, password);
    REQUIRE(!decrypted.empty());
    
    std::string decryptedText(decrypted.begin(), decrypted.end());
    REQUIRE(decryptedText == plaintext);
}

TEST_CASE("File I/O with encryption") {
    CryptoRepository repo;
    std::string password = "fileTestPass";
    std::string plaintext = "This is a test file content!";
    std::vector<unsigned char> data(plaintext.begin(), plaintext.end());
    
    auto result = repo.encrypt(data, password);
    REQUIRE(!result.data.empty());
    REQUIRE(onc::format::validateHeader(result.data));
    
    std::string tempFile = "test_encrypted.bin";
    writeFile(tempFile, result.data);
    
    auto encryptedData = readFile(tempFile);
    REQUIRE(!encryptedData.empty());
    REQUIRE(onc::format::validateHeader(encryptedData));
    
    auto decrypted = repo.decrypt(encryptedData, password);
    REQUIRE(!decrypted.empty());
    
    std::string decryptedText(decrypted.begin(), decrypted.end());
    REQUIRE(decryptedText == plaintext);
}

TEST_CASE("Algorithm selection") {
    CryptoRepository repo;
    std::string password = "algoTest";
    
    // Small data -> XChaCha20
    std::string smallData = "Small";
    std::vector<unsigned char> small(smallData.begin(), smallData.end());
    auto result1 = repo.encrypt(small, password);
    REQUIRE(!result1.data.empty());
    REQUIRE(onc::format::validateHeader(result1.data));
    
    // Large data -> AES-256-GCM
    std::vector<unsigned char> large(2 * 1024 * 1024, 'A');
    auto result2 = repo.encrypt(large, password);
    REQUIRE(!result2.data.empty());
    REQUIRE(onc::format::validateHeader(result2.data));
}

TEST_CASE("Large data (10MB)") {
    CryptoRepository repo;
    std::string password = "largeTest";
    
    std::vector<unsigned char> largeData(10 * 1024 * 1024);
    for (size_t i = 0; i < largeData.size(); ++i) {
        largeData[i] = static_cast<unsigned char>(i % 256);
    }
    
    auto result = repo.encrypt(largeData, password);
    REQUIRE(!result.data.empty());
    REQUIRE(onc::format::validateHeader(result.data));
    
    auto decrypted = repo.decrypt(result.data, password);
    REQUIRE(decrypted.size() == largeData.size());
    
    for (size_t i = 0; i < 1000; ++i) {
        REQUIRE(decrypted[i] == largeData[i]);
    }
}
