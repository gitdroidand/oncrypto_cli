#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include "oncrypto/c_abi/c_abi.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// ============================================================================
// Test helpers
// ============================================================================

namespace {

constexpr const char* TEST_KEY = "oncrypto-test-key-1.6.0";
constexpr const char* WRONG_KEY = "definitely-wrong-key";

// --------------------------------------------------------------------------
// Convert C string to byte vector
// --------------------------------------------------------------------------

std::vector<std::uint8_t> bytes(const char* str) {
    REQUIRE(str != nullptr);

    const std::size_t len = std::strlen(str);

    return std::vector<std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(str),
        reinterpret_cast<const std::uint8_t*>(str) + len
    );
}

// --------------------------------------------------------------------------
// Convert std::string to byte vector
// --------------------------------------------------------------------------

std::vector<std::uint8_t> bytes(const std::string& str) {
    return std::vector<std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(str.data()),
        reinterpret_cast<const std::uint8_t*>(str.data()) + str.size()
    );
}

// --------------------------------------------------------------------------
// Convert filesystem path to UTF-8.
//
// The C ABI uses const char* paths. Therefore the test must explicitly
// convert std::filesystem::path to UTF-8 instead of passing path.c_str().
//
// On Windows, std::filesystem::path::value_type is wchar_t.
// On POSIX systems, path::value_type is char.
// --------------------------------------------------------------------------

std::string path_to_utf8(const std::filesystem::path& path) {
#ifdef _WIN32
    const auto value = path.u8string();

    return std::string(
        reinterpret_cast<const char*>(value.data()),
        value.size()
    );
#else
    return path.string();
#endif
}

// --------------------------------------------------------------------------
// BufferGuard
// --------------------------------------------------------------------------

struct BufferGuard {
    onc_buffer buffer{nullptr, 0};

    BufferGuard() = default;

    BufferGuard(const BufferGuard&) = delete;
    BufferGuard& operator=(const BufferGuard&) = delete;

    ~BufferGuard() {
        onc_buffer_free(&buffer);
    }

    onc_buffer* get() {
        return &buffer;
    }

    const onc_buffer* get() const {
        return &buffer;
    }
};

// --------------------------------------------------------------------------
// Check onc_buffer reset state
// --------------------------------------------------------------------------

void require_buffer_reset(const onc_buffer& buffer) {
    CHECK(buffer.data == nullptr);
    CHECK(buffer.size == 0);
}

// --------------------------------------------------------------------------
// Temporary test directory
// --------------------------------------------------------------------------

std::filesystem::path make_test_directory() {
    const auto dir =
        std::filesystem::temp_directory_path() /
        "oncrypto_c_tests";

    std::error_code ec;

    std::filesystem::create_directories(dir, ec);

    REQUIRE(!ec);

    return dir;
}

// --------------------------------------------------------------------------
// Temporary file path
// --------------------------------------------------------------------------

std::filesystem::path make_temp_file(const std::string& name) {
    return make_test_directory() / name;
}

// --------------------------------------------------------------------------
// Write binary file
// --------------------------------------------------------------------------

void write_file(
    const std::filesystem::path& path,
    const std::vector<std::uint8_t>& data
) {
    std::ofstream file(
        path,
        std::ios::binary | std::ios::trunc
    );

    REQUIRE(file.is_open());

    if (!data.empty()) {
        file.write(
            reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size())
        );
    }

    const bool write_ok = file.good();

    CHECK(write_ok);

    file.close();
}

// --------------------------------------------------------------------------
// Read binary file
// --------------------------------------------------------------------------

std::vector<std::uint8_t> read_file(
    const std::filesystem::path& path
) {
    std::ifstream file(
        path,
        std::ios::binary
    );

    REQUIRE(file.is_open());

    file.seekg(0, std::ios::end);

    REQUIRE(file.good());

    const std::streamoff size = file.tellg();

    REQUIRE(size >= 0);

    file.seekg(0, std::ios::beg);

    REQUIRE(file.good());

    if (size == 0) {
        return {};
    }

    std::vector<std::uint8_t> result(
        static_cast<std::size_t>(size)
    );

    file.read(
        reinterpret_cast<char*>(result.data()),
        static_cast<std::streamsize>(result.size())
    );

    // doctest 2.5 does not allow || directly inside CHECK/REQUIRE.
    const bool read_ok = file.good() || file.eof();

    CHECK(read_ok);

    return result;
}

// --------------------------------------------------------------------------
// Cleanup helper
// --------------------------------------------------------------------------

void remove_if_exists(
    const std::filesystem::path& path
) {
    std::error_code ec;

    std::filesystem::remove(path, ec);

    // Cleanup failure should not turn a successful crypto test into a
    // hard test failure because the actual operation already completed.
}

} // namespace

// ============================================================================
// Metadata
// ============================================================================

TEST_CASE("C ABI - version") {
    const char* version = onc_version();

    REQUIRE(version != nullptr);
    CHECK(std::strlen(version) > 0);
}

// ============================================================================
// Status strings
// ============================================================================

TEST_CASE("C ABI - status strings") {
    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_SUCCESS),
            "Success"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_ERROR_INVALID_ARGUMENT),
            "Invalid Argument"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_ERROR_ENCRYPTION_FAILED),
            "Encryption Failed"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_ERROR_DECRYPTION_FAILED),
            "Decryption Failed"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_ERROR_AUTHENTICATION_FAILED),
            "Authentication Failed"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_ERROR_INVALID_FORMAT),
            "Invalid Format"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_ERROR_IO),
            "I/O Error"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_ERROR_OUT_OF_MEMORY),
            "Out of Memory"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(ONC_ERROR_INTERNAL),
            "Internal Error"
        ) == 0
    );

    CHECK(
        std::strcmp(
            onc_status_to_string(
                static_cast<onc_status>(999999)
            ),
            "Unknown Error"
        ) == 0
    );
}

// ============================================================================
// Last error
// ============================================================================

TEST_CASE("C ABI - last error starts empty") {
    const char* error = onc_get_last_error();

    REQUIRE(error != nullptr);
    CHECK(std::strlen(error) == 0);
}

TEST_CASE("C ABI - invalid argument updates last error") {
    onc_buffer buffer{nullptr, 0};

    const onc_status status = onc_encrypt_buffer(
        nullptr,
        1,
        nullptr,
        0,
        &buffer
    );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);

    const char* error = onc_get_last_error();

    REQUIRE(error != nullptr);
    CHECK(std::strlen(error) > 0);

    require_buffer_reset(buffer);
}

// ============================================================================
// Buffer memory management
// ============================================================================

TEST_CASE("C ABI - buffer free is safe") {
    onc_buffer buffer{nullptr, 0};

    onc_buffer_free(nullptr);
    onc_buffer_free(&buffer);

    CHECK(buffer.data == nullptr);
    CHECK(buffer.size == 0);
}

TEST_CASE("C ABI - buffer can be freed twice safely") {
    const auto plaintext = bytes("double free test");
    const auto key = bytes(TEST_KEY);

    BufferGuard output;

    REQUIRE(
        onc_encrypt_buffer(
            plaintext.data(),
            plaintext.size(),
            key.data(),
            key.size(),
            output.get()
        ) == ONC_SUCCESS
    );

    REQUIRE(output.buffer.data != nullptr);
    REQUIRE(output.buffer.size > 0);

    onc_buffer_free(&output.buffer);

    CHECK(output.buffer.data == nullptr);
    CHECK(output.buffer.size == 0);

    // Must remain safe.
    onc_buffer_free(&output.buffer);

    CHECK(output.buffer.data == nullptr);
    CHECK(output.buffer.size == 0);
}

// ============================================================================
// Basic encryption / decryption
// ============================================================================

TEST_CASE("C ABI - encrypt and decrypt buffer") {
    const auto plaintext =
        bytes("Hello from OnCrypto C ABI!");

    const auto key =
        bytes(TEST_KEY);

    BufferGuard encrypted;
    BufferGuard decrypted;

    const onc_status encrypt_status =
        onc_encrypt_buffer(
            plaintext.data(),
            plaintext.size(),
            key.data(),
            key.size(),
            encrypted.get()
        );

    REQUIRE(encrypt_status == ONC_SUCCESS);

    REQUIRE(encrypted.buffer.data != nullptr);
    REQUIRE(encrypted.buffer.size > 0);

    const onc_status decrypt_status =
        onc_decrypt_buffer(
            encrypted.buffer.data,
            encrypted.buffer.size,
            key.data(),
            key.size(),
            decrypted.get()
        );

    REQUIRE(decrypt_status == ONC_SUCCESS);

    REQUIRE(decrypted.buffer.data != nullptr);

    CHECK(decrypted.buffer.size == plaintext.size());

    CHECK(
        std::memcmp(
            decrypted.buffer.data,
            plaintext.data(),
            plaintext.size()
        ) == 0
    );
}

// ============================================================================
// Binary data
// ============================================================================

TEST_CASE("C ABI - binary data round trip") {
    const std::vector<std::uint8_t> plaintext{
        0x00,
        0x01,
        0x02,
        0x7F,
        0x80,
        0xFE,
        0xFF,
        0x00,
        0xAA,
        0x55,
        0x00
    };

    const auto key =
        bytes(TEST_KEY);

    BufferGuard encrypted;
    BufferGuard decrypted;

    const onc_status encrypt_status =
        onc_encrypt_buffer(
            plaintext.data(),
            plaintext.size(),
            key.data(),
            key.size(),
            encrypted.get()
        );

    REQUIRE(encrypt_status == ONC_SUCCESS);
    REQUIRE(encrypted.buffer.data != nullptr);

    const onc_status decrypt_status =
        onc_decrypt_buffer(
            encrypted.buffer.data,
            encrypted.buffer.size,
            key.data(),
            key.size(),
            decrypted.get()
        );

    REQUIRE(decrypt_status == ONC_SUCCESS);
    REQUIRE(decrypted.buffer.data != nullptr);

    CHECK(decrypted.buffer.size == plaintext.size());

    CHECK(
        std::memcmp(
            decrypted.buffer.data,
            plaintext.data(),
            plaintext.size()
        ) == 0
    );
}

// ============================================================================
// Empty plaintext
// ============================================================================

TEST_CASE("C ABI - empty plaintext") {
    const auto key =
        bytes(TEST_KEY);

    BufferGuard encrypted;
    BufferGuard decrypted;

    const onc_status encrypt_status =
        onc_encrypt_buffer(
            nullptr,
            0,
            key.data(),
            key.size(),
            encrypted.get()
        );

    REQUIRE(encrypt_status == ONC_SUCCESS);

    const onc_status decrypt_status =
        onc_decrypt_buffer(
            encrypted.buffer.data,
            encrypted.buffer.size,
            key.data(),
            key.size(),
            decrypted.get()
        );

    REQUIRE(decrypt_status == ONC_SUCCESS);

    CHECK(decrypted.buffer.size == 0);
}

// ============================================================================
// Argument validation
// ============================================================================

TEST_CASE("C ABI - null output buffer") {
    const auto input =
        bytes("test");

    const auto key =
        bytes(TEST_KEY);

    const onc_status status =
        onc_encrypt_buffer(
            input.data(),
            input.size(),
            key.data(),
            key.size(),
            nullptr
        );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);

    const char* error =
        onc_get_last_error();

    REQUIRE(error != nullptr);
    CHECK(std::strlen(error) > 0);
}

TEST_CASE("C ABI - null input with non-zero length") {
    const auto key =
        bytes(TEST_KEY);

    onc_buffer output{nullptr, 0};

    const onc_status status =
        onc_encrypt_buffer(
            nullptr,
            10,
            key.data(),
            key.size(),
            &output
        );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);

    require_buffer_reset(output);
}

TEST_CASE("C ABI - null key with non-zero length") {
    const auto input =
        bytes("test");

    onc_buffer output{nullptr, 0};

    const onc_status status =
        onc_encrypt_buffer(
            input.data(),
            input.size(),
            nullptr,
            32,
            &output
        );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);

    require_buffer_reset(output);
}

TEST_CASE("C ABI - decrypt null output buffer") {
    const auto encrypted =
        bytes("not-real-ciphertext");

    const auto key =
        bytes(TEST_KEY);

    const onc_status status =
        onc_decrypt_buffer(
            encrypted.data(),
            encrypted.size(),
            key.data(),
            key.size(),
            nullptr
        );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("C ABI - decrypt invalid ciphertext") {
    const auto invalid =
        bytes("this-is-not-encrypted-data");

    const auto key =
        bytes(TEST_KEY);

    BufferGuard output;

    const onc_status status =
        onc_decrypt_buffer(
            invalid.data(),
            invalid.size(),
            key.data(),
            key.size(),
            output.get()
        );

    CHECK(status != ONC_SUCCESS);

    require_buffer_reset(output.buffer);
}

// ============================================================================
// Authentication / wrong key
// ============================================================================

TEST_CASE("C ABI - wrong key fails decryption") {
    const auto plaintext =
        bytes("Authentication must fail with another key.");

    const auto key =
        bytes(TEST_KEY);

    const auto wrong_key =
        bytes(WRONG_KEY);

    BufferGuard encrypted;
    BufferGuard decrypted;

    REQUIRE(
        onc_encrypt_buffer(
            plaintext.data(),
            plaintext.size(),
            key.data(),
            key.size(),
            encrypted.get()
        ) == ONC_SUCCESS
    );

    const onc_status status =
        onc_decrypt_buffer(
            encrypted.buffer.data,
            encrypted.buffer.size,
            wrong_key.data(),
            wrong_key.size(),
            decrypted.get()
        );

    CHECK(status != ONC_SUCCESS);

    // doctest 2.5 rejects logical OR inside CHECK/REQUIRE.
    const bool authentication_failure =
        status == ONC_ERROR_DECRYPTION_FAILED ||
        status == ONC_ERROR_AUTHENTICATION_FAILED;

    CHECK(authentication_failure);

    require_buffer_reset(decrypted.buffer);
}

// ============================================================================
// Builder API
// ============================================================================

TEST_CASE("C ABI - builder create and destroy") {
    onc_builder_t builder =
        onc_builder_create();

    REQUIRE(builder != nullptr);

    onc_builder_destroy(builder);

    // Destroying nullptr must be safe.
    onc_builder_destroy(nullptr);
}

TEST_CASE("C ABI - builder null validation") {
    const auto key =
        bytes(TEST_KEY);

    CHECK(
        onc_builder_set_key(
            nullptr,
            key.data(),
            key.size()
        ) == ONC_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        onc_builder_set_algorithm(
            nullptr,
            "AES256_GCM"
        ) == ONC_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        onc_builder_set_iterations(
            nullptr,
            100000
        ) == ONC_ERROR_INVALID_ARGUMENT
    );
}

TEST_CASE("C ABI - builder set key and algorithm") {
    onc_builder_t builder =
        onc_builder_create();

    REQUIRE(builder != nullptr);

    const auto key =
        bytes(TEST_KEY);

    CHECK(
        onc_builder_set_key(
            builder,
            key.data(),
            key.size()
        ) == ONC_SUCCESS
    );

    CHECK(
        onc_builder_set_algorithm(
            builder,
            "Auto"
        ) == ONC_SUCCESS
    );

    CHECK(
        onc_builder_set_algorithm(
            builder,
            "AES256_GCM"
        ) == ONC_SUCCESS
    );

    CHECK(
        onc_builder_set_algorithm(
            builder,
            "ChaCha20"
        ) == ONC_SUCCESS
    );

    CHECK(
        onc_builder_set_algorithm(
            builder,
            "XChaCha20"
        ) == ONC_SUCCESS
    );

    onc_builder_destroy(builder);
}

TEST_CASE("C ABI - builder rejects unknown algorithm") {
    onc_builder_t builder =
        onc_builder_create();

    REQUIRE(builder != nullptr);

    const onc_status status =
        onc_builder_set_algorithm(
            builder,
            "NOT_AN_ALGORITHM"
        );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);

    const char* error =
        onc_get_last_error();

    REQUIRE(error != nullptr);
    CHECK(std::strlen(error) > 0);

    onc_builder_destroy(builder);
}

TEST_CASE("C ABI - builder encrypt") {
    onc_builder_t builder =
        onc_builder_create();

    REQUIRE(builder != nullptr);

    const auto key =
        bytes(TEST_KEY);

    const auto plaintext =
        bytes("Builder encryption test.");

    REQUIRE(
        onc_builder_set_key(
            builder,
            key.data(),
            key.size()
        ) == ONC_SUCCESS
    );

    REQUIRE(
        onc_builder_set_algorithm(
            builder,
            "AES256_GCM"
        ) == ONC_SUCCESS
    );

    REQUIRE(
        onc_builder_set_iterations(
            builder,
            100000
        ) == ONC_SUCCESS
    );

    BufferGuard encrypted;

    const onc_status status =
        onc_builder_encrypt(
            builder,
            plaintext.data(),
            plaintext.size(),
            encrypted.get()
        );

    REQUIRE(status == ONC_SUCCESS);

    REQUIRE(encrypted.buffer.data != nullptr);
    CHECK(encrypted.buffer.size > 0);

    onc_builder_destroy(builder);
}

// ============================================================================
// File API
// ============================================================================

TEST_CASE("C ABI - file encryption and decryption") {
    const auto source =
        make_temp_file(
            "oncrypto_c_source.bin"
        );

    const auto encrypted =
        make_temp_file(
            "oncrypto_c_encrypted.onc"
        );

    const auto decrypted =
        make_temp_file(
            "oncrypto_c_decrypted.bin"
        );

    const auto plaintext =
        bytes(
            "OnCrypto C ABI file encryption test.\n"
            "Binary-safe content: \x00\x01\x02\xFF"
        );

    const auto key =
        bytes(TEST_KEY);

    write_file(
        source,
        plaintext
    );

    // The C ABI accepts UTF-8 paths, not filesystem::path::value_type.
    const std::string source_utf8 =
        path_to_utf8(source);

    const std::string encrypted_utf8 =
        path_to_utf8(encrypted);

    const std::string decrypted_utf8 =
        path_to_utf8(decrypted);

    const onc_status encrypt_status =
        onc_encrypt_file(
            source_utf8.c_str(),
            encrypted_utf8.c_str(),
            key.data(),
            key.size()
        );

    REQUIRE(encrypt_status == ONC_SUCCESS);

    REQUIRE(
        std::filesystem::exists(encrypted)
    );

    const onc_status decrypt_status =
        onc_decrypt_file(
            encrypted_utf8.c_str(),
            decrypted_utf8.c_str(),
            key.data(),
            key.size()
        );

    REQUIRE(decrypt_status == ONC_SUCCESS);

    REQUIRE(
        std::filesystem::exists(decrypted)
    );

    const auto result =
        read_file(decrypted);

    CHECK(result == plaintext);

    remove_if_exists(source);
    remove_if_exists(encrypted);
    remove_if_exists(decrypted);
}

TEST_CASE("C ABI - file null path validation") {
    const auto key =
        bytes(TEST_KEY);

    CHECK(
        onc_encrypt_file(
            nullptr,
            "output.onc",
            key.data(),
            key.size()
        ) == ONC_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        onc_encrypt_file(
            "input.bin",
            nullptr,
            key.data(),
            key.size()
        ) == ONC_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        onc_decrypt_file(
            nullptr,
            "output.bin",
            key.data(),
            key.size()
        ) == ONC_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        onc_decrypt_file(
            "input.onc",
            nullptr,
            key.data(),
            key.size()
        ) == ONC_ERROR_INVALID_ARGUMENT
    );
}

// ============================================================================
// Streaming API
// ============================================================================

TEST_CASE("C ABI - streaming encryptor creation") {
    const auto key =
        bytes(TEST_KEY);

    onc_stream_t stream =
        onc_stream_create_encryptor(
            key.data(),
            key.size()
        );

    REQUIRE(stream != nullptr);

    onc_stream_destroy(stream);
}

TEST_CASE("C ABI - streaming decryptor creation") {
    const auto key =
        bytes(TEST_KEY);

    onc_stream_t stream =
        onc_stream_create_decryptor(
            key.data(),
            key.size()
        );

    REQUIRE(stream != nullptr);

    onc_stream_destroy(stream);
}

TEST_CASE("C ABI - streaming null key validation") {
    onc_stream_t encryptor =
        onc_stream_create_encryptor(
            nullptr,
            32
        );

    CHECK(encryptor == nullptr);

    const char* encrypt_error =
        onc_get_last_error();

    REQUIRE(encrypt_error != nullptr);
    CHECK(std::strlen(encrypt_error) > 0);

    onc_stream_t decryptor =
        onc_stream_create_decryptor(
            nullptr,
            32
        );

    CHECK(decryptor == nullptr);

    const char* decrypt_error =
        onc_get_last_error();

    REQUIRE(decrypt_error != nullptr);
    CHECK(std::strlen(decrypt_error) > 0);
}

TEST_CASE("C ABI - streaming update succeeds for encryptor") {
    const auto key =
        bytes(TEST_KEY);

    const auto chunk =
        bytes("stream chunk");

    onc_stream_t stream =
        onc_stream_create_encryptor(
            key.data(),
            key.size()
        );

    REQUIRE(stream != nullptr);

    onc_buffer output{nullptr, 0};
    onc_buffer final_output{nullptr, 0};

    const onc_status status =
        onc_stream_update(
            stream,
            chunk.data(),
            chunk.size(),
            &output
        );

    CHECK(status == ONC_SUCCESS);
    REQUIRE(output.data != nullptr);
    CHECK(output.size > 0);

    const onc_status final_status =
        onc_stream_final(
            stream,
            &final_output
        );

    CHECK(final_status == ONC_SUCCESS);
    require_buffer_reset(final_output);
    onc_buffer_free(&output);

    onc_stream_destroy(stream);
}

TEST_CASE("C ABI - streaming final succeeds for decryptor lifecycle") {
    const auto key =
        bytes(TEST_KEY);

    const auto chunk =
        bytes("stream chunk");

    onc_stream_t encryptor =
        onc_stream_create_encryptor(
            key.data(),
            key.size()
        );

    REQUIRE(encryptor != nullptr);

    onc_buffer encrypted{nullptr, 0};
    onc_buffer final_output{nullptr, 0};

    const onc_status encrypt_status =
        onc_stream_update(
            encryptor,
            chunk.data(),
            chunk.size(),
            &encrypted
        );

    REQUIRE(encrypt_status == ONC_SUCCESS);
    REQUIRE(encrypted.data != nullptr);
    REQUIRE(encrypted.size > 0);

    const onc_status final_encrypt_status =
        onc_stream_final(
            encryptor,
            &final_output
        );

    CHECK(final_encrypt_status == ONC_SUCCESS);
    require_buffer_reset(final_output);

    onc_stream_t decryptor =
        onc_stream_create_decryptor(
            key.data(),
            key.size()
        );

    REQUIRE(decryptor != nullptr);

    onc_buffer plaintext{nullptr, 0};

    const onc_status decrypt_status =
        onc_stream_update(
            decryptor,
            encrypted.data,
            encrypted.size,
            &plaintext
        );

    CHECK(decrypt_status == ONC_SUCCESS);
    REQUIRE(plaintext.data != nullptr);
    CHECK(plaintext.size == chunk.size());
    CHECK(
        std::memcmp(
            plaintext.data,
            chunk.data(),
            chunk.size()
        ) == 0
    );

    const onc_status decrypt_final_status =
        onc_stream_final(
            decryptor,
            &final_output
        );

    CHECK(decrypt_final_status == ONC_SUCCESS);
    require_buffer_reset(final_output);

    onc_buffer_free(&encrypted);
    onc_buffer_free(&plaintext);
    onc_stream_destroy(decryptor);
    onc_stream_destroy(encryptor);
}

// ============================================================================
// Stream null validation
// ============================================================================

TEST_CASE("C ABI - streaming update null validation") {
    const auto chunk =
        bytes("test");

    onc_buffer output{nullptr, 0};

    const onc_status status =
        onc_stream_update(
            nullptr,
            chunk.data(),
            chunk.size(),
            &output
        );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);

    require_buffer_reset(output);
}

TEST_CASE("C ABI - streaming final null validation") {
    onc_buffer output{nullptr, 0};

    const onc_status status =
        onc_stream_final(
            nullptr,
            &output
        );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);

    require_buffer_reset(output);
}

TEST_CASE("C ABI - streaming update null chunk with non-zero length") {
    const auto key =
        bytes(TEST_KEY);

    onc_stream_t stream =
        onc_stream_create_encryptor(
            key.data(),
            key.size()
        );

    REQUIRE(stream != nullptr);

    onc_buffer output{nullptr, 0};

    const onc_status status =
        onc_stream_update(
            stream,
            nullptr,
            10,
            &output
        );

    CHECK(status == ONC_ERROR_INVALID_ARGUMENT);

    require_buffer_reset(output);

    onc_stream_destroy(stream);
}

// ============================================================================
// Repeated round-trip sanity
// ============================================================================

TEST_CASE("C ABI - repeated round trip sanity") {
    const auto key =
        bytes(TEST_KEY);

    const std::string text =
        "OnCrypto performance sanity payload. "
        "This test intentionally stays small.";

    const auto plaintext =
        bytes(text);

    for (int i = 0; i < 5; ++i) {
        BufferGuard encrypted;
        BufferGuard decrypted;

        const onc_status encrypt_status =
            onc_encrypt_buffer(
                plaintext.data(),
                plaintext.size(),
                key.data(),
                key.size(),
                encrypted.get()
            );

        REQUIRE(encrypt_status == ONC_SUCCESS);

        const onc_status decrypt_status =
            onc_decrypt_buffer(
                encrypted.buffer.data,
                encrypted.buffer.size,
                key.data(),
                key.size(),
                decrypted.get()
            );

        REQUIRE(decrypt_status == ONC_SUCCESS);

        CHECK(
            decrypted.buffer.size ==
            plaintext.size()
        );

        CHECK(
            std::memcmp(
                decrypted.buffer.data,
                plaintext.data(),
                plaintext.size()
            ) == 0
        );
    }
}