// include/gui/ui/core/onc_service.hpp

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "oncrypto/oncrypto.hpp"

namespace gui::core
{

    class OncService final
    {
    public:
        using Byte = unsigned char;
        using Bytes = std::vector<Byte>;

        // ============================================================
        // Types
        // ============================================================

        enum class Algorithm
        {
            Auto,
            AES256_GCM,
            ChaCha20,
            XChaCha20
        };

        enum class KDF
        {
            PBKDF2,
            Argon2
        };

        enum class OutputFormat
        {
            Binary,
            Hex,
            Base64
        };

        enum class Operation
        {
            Encrypt,
            Decrypt,
            StreamEncrypt,
            StreamDecrypt
        };

        struct Progress
        {
            std::uint64_t processed = 0;
            std::uint64_t total = 0;

            [[nodiscard]]
            double percentage() const noexcept;
        };

        struct Result
        {
            bool success = false;
            Bytes data;

            std::string error;
            std::string message;

            [[nodiscard]]
            explicit operator bool() const noexcept
            {
                return success;
            }

            [[nodiscard]]
            bool failed() const noexcept
            {
                return !success;
            }

            static Result ok();

            static Result ok(Bytes data);

            static Result fail(std::string error);

            static Result fail(
                std::string error,
                std::string message);
        };

        using ProgressCallback =
            std::function<void(const Progress &)>;

        // ============================================================
        // Data DSL
        // ============================================================

        class DataBuilder final
        {
        public:
            explicit DataBuilder(OncService &service);

            DataBuilder &input(const Bytes &data);

            DataBuilder &input(Bytes &&data);

            DataBuilder &password(std::string password);

            DataBuilder &algorithm(Algorithm algorithm);

            [[nodiscard]]
            Result encrypt();

            [[nodiscard]]
            Result decrypt();

        private:
            OncService &service_;

            Bytes data_;
            std::string password_;

            Algorithm algorithm_ = Algorithm::Auto;
        };

        // ============================================================
        // File DSL
        // ============================================================

        class FileBuilder final
        {
        public:
            explicit FileBuilder(OncService &service);

            FileBuilder &input(std::string path);

            FileBuilder &output(std::string path);

            FileBuilder &password(std::string password);

            FileBuilder &algorithm(Algorithm algorithm);

            [[nodiscard]]
            Result encrypt();

            [[nodiscard]]
            Result decrypt();

        private:
            OncService &service_;

            std::string input_;
            std::string output_;
            std::string password_;

            Algorithm algorithm_ = Algorithm::Auto;
        };

        // ============================================================
        // Streaming DSL
        // ============================================================

        class StreamBuilder final
        {
        public:
            explicit StreamBuilder(OncService &service);

            StreamBuilder &input(std::string path);

            StreamBuilder &output(std::string path);

            StreamBuilder &password(std::string password);

            StreamBuilder &chunkSize(std::size_t size);

            StreamBuilder &progress(ProgressCallback callback);

            [[nodiscard]]
            Result encrypt();

            [[nodiscard]]
            Result decrypt();

        private:
            OncService &service_;

            std::string input_;
            std::string output_;
            std::string password_;

            std::size_t chunk_size_ = 1024 * 1024;

            ProgressCallback progress_callback_;
        };

        // ============================================================
        // Advanced DSL
        // ============================================================

        class AdvancedBuilder final
        {
        public:
            explicit AdvancedBuilder(OncService &service);

            AdvancedBuilder &input(const Bytes &data);

            AdvancedBuilder &input(Bytes &&data);

            AdvancedBuilder &password(std::string password);

            AdvancedBuilder &algorithm(Algorithm algorithm);

            AdvancedBuilder &kdf(KDF kdf);

            AdvancedBuilder &iterations(int iterations);

            AdvancedBuilder &format(OutputFormat format);

            AdvancedBuilder &storeMetadata(bool enabled);

            AdvancedBuilder &verifyIntegrity(bool enabled);

            [[nodiscard]]
            Result encrypt();

            [[nodiscard]]
            Result decrypt();

        private:
            OncService &service_;

            Bytes data_;
            std::string password_;

            Algorithm algorithm_ = Algorithm::Auto;
            KDF kdf_ = KDF::PBKDF2;

            int iterations_ = 100000;

            OutputFormat output_format_ =
                OutputFormat::Binary;

            bool store_metadata_ = true;
            bool verify_integrity_ = true;
        };

        // ============================================================
        // Service
        // ============================================================

        OncService() = default;

        ~OncService() = default;

        OncService(const OncService &) = delete;
        OncService &operator=(const OncService &) = delete;

        OncService(OncService &&) = delete;
        OncService &operator=(OncService &&) = delete;

        // ============================================================
        // DSL Entry Points
        // ============================================================

        [[nodiscard]]
        DataBuilder data();

        [[nodiscard]]
        FileBuilder file();

        [[nodiscard]]
        StreamBuilder stream();

        [[nodiscard]]
        AdvancedBuilder advanced();

        // ============================================================
        // Information
        // ============================================================

        [[nodiscard]]
        static std::string version();

        [[nodiscard]]
        static std::string algorithmName();

        // ============================================================
        // Direct API
        // Useful for places that don't need the DSL.
        // ============================================================

        [[nodiscard]]
        Result encrypt(
            const Bytes &data,
            const std::string &password,
            Algorithm algorithm = Algorithm::Auto);

        [[nodiscard]]
        Result decrypt(
            const Bytes &data,
            const std::string &password,
            Algorithm algorithm = Algorithm::Auto);

        [[nodiscard]]
        Result encryptFile(
            const std::string &input,
            const std::string &output,
            const std::string &password,
            Algorithm algorithm = Algorithm::Auto);

        [[nodiscard]]
        Result decryptFile(
            const std::string &input,
            const std::string &output,
            const std::string &password,
            Algorithm algorithm = Algorithm::Auto);

        [[nodiscard]]
        Result encryptStream(
            const std::string &input,
            const std::string &output,
            const std::string &password,
            std::size_t chunkSize = 1024 * 1024,
            ProgressCallback callback = nullptr);

        [[nodiscard]]
        Result decryptStream(
            const std::string &input,
            const std::string &output,
            const std::string &password,
            std::size_t chunkSize = 1024 * 1024,
            ProgressCallback callback = nullptr);

    private:
        [[nodiscard]]
        static crypto::builder::Algorithm
        toCoreAlgorithm(Algorithm algorithm);

        [[nodiscard]]
        static crypto::advanced::KDF
        toCoreKDF(KDF kdf);

        [[nodiscard]]
        static crypto::advanced::OutputFormat
        toCoreOutputFormat(OutputFormat format);

        [[nodiscard]]
        static bool
        validatePassword(
            const std::string &password,
            std::string &error);

        [[nodiscard]]
        static bool
        validateData(
            const Bytes &data,
            std::string &error);

        [[nodiscard]]
        static bool
        validateFilePaths(
            const std::string &input,
            const std::string &output,
            std::string &error);

        [[nodiscard]]
        static bool
        validateChunkSize(
            std::size_t chunkSize,
            std::string &error);

        // NEW: Session tracking
        void recordSuccess(const std::string &operation, std::uint64_t bytes);

        // NEW: Use FileUtils for auto path generation
        static std::string autoEncryptPath(const std::string &input);
        static std::string autoDecryptPath(const std::string &input);
    };

} // namespace gui::core