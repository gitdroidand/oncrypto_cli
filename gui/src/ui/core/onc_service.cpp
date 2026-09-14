// src/ui/core/onc_service.cpp

#include "gui/ui/core/onc_service.hpp"
#include "gui/ui/core/session_stats.hpp"
#include "gui/ui/core/recent_files.hpp"

#include <QFileInfo>

#include <algorithm>
#include <exception>
#include <limits>
#include <utility>

namespace gui::core
{

    // ============================================================
    // Progress
    // ============================================================

    double OncService::Progress::percentage() const noexcept
    {
        if (total == 0)
            return 0.0;

        if (processed >= total)
            return 100.0;

        return static_cast<double>(processed) *
               100.0 /
               static_cast<double>(total);
    }

    // ============================================================
    // Result
    // ============================================================

    OncService::Result
    OncService::Result::ok()
    {
        Result result;

        result.success = true;
        result.message = "Operation completed successfully.";

        return result;
    }

    OncService::Result
    OncService::Result::ok(Bytes data)
    {
        Result result;

        result.success = true;
        result.data = std::move(data);
        result.message = "Operation completed successfully.";

        return result;
    }

    OncService::Result
    OncService::Result::fail(std::string error)
    {
        Result result;

        result.success = false;
        result.error = std::move(error);

        return result;
    }

    OncService::Result
    OncService::Result::fail(
        std::string error,
        std::string message)
    {
        Result result;

        result.success = false;
        result.error = std::move(error);
        result.message = std::move(message);

        return result;
    }

    // ============================================================
    // DataBuilder
    // ============================================================

    OncService::DataBuilder::DataBuilder(
        OncService &service)
        : service_(service)
    {
    }

    OncService::DataBuilder &
    OncService::DataBuilder::input(
        const Bytes &data)
    {
        data_ = data;
        return *this;
    }

    OncService::DataBuilder &
    OncService::DataBuilder::input(
        Bytes &&data)
    {
        data_ = std::move(data);
        return *this;
    }

    OncService::DataBuilder &
    OncService::DataBuilder::password(
        std::string password)
    {
        password_ = std::move(password);
        return *this;
    }

    OncService::DataBuilder &
    OncService::DataBuilder::algorithm(
        Algorithm algorithm)
    {
        algorithm_ = algorithm;
        return *this;
    }

    OncService::Result
    OncService::DataBuilder::encrypt()
    {
        return service_.encrypt(
            data_,
            password_,
            algorithm_);
    }

    OncService::Result
    OncService::DataBuilder::decrypt()
    {
        return service_.decrypt(
            data_,
            password_,
            algorithm_);
    }

    // ============================================================
    // FileBuilder
    // ============================================================

    OncService::FileBuilder::FileBuilder(
        OncService &service)
        : service_(service)
    {
    }

    OncService::FileBuilder &
    OncService::FileBuilder::input(
        std::string path)
    {
        input_ = std::move(path);
        return *this;
    }

    OncService::FileBuilder &
    OncService::FileBuilder::output(
        std::string path)
    {
        output_ = std::move(path);
        return *this;
    }

    OncService::FileBuilder &
    OncService::FileBuilder::password(
        std::string password)
    {
        password_ = std::move(password);
        return *this;
    }

    OncService::FileBuilder &
    OncService::FileBuilder::algorithm(
        Algorithm algorithm)
    {
        algorithm_ = algorithm;
        return *this;
    }

    OncService::Result
    OncService::FileBuilder::encrypt()
    {
        return service_.encryptFile(
            input_,
            output_,
            password_,
            algorithm_);
    }

    OncService::Result
    OncService::FileBuilder::decrypt()
    {
        return service_.decryptFile(
            input_,
            output_,
            password_,
            algorithm_);
    }

    // ============================================================
    // StreamBuilder
    // ============================================================

    OncService::StreamBuilder::StreamBuilder(
        OncService &service)
        : service_(service)
    {
    }

    OncService::StreamBuilder &
    OncService::StreamBuilder::input(
        std::string path)
    {
        input_ = std::move(path);
        return *this;
    }

    OncService::StreamBuilder &
    OncService::StreamBuilder::output(
        std::string path)
    {
        output_ = std::move(path);
        return *this;
    }

    OncService::StreamBuilder &
    OncService::StreamBuilder::password(
        std::string password)
    {
        password_ = std::move(password);
        return *this;
    }

    OncService::StreamBuilder &
    OncService::StreamBuilder::chunkSize(
        std::size_t size)
    {
        chunk_size_ = size;
        return *this;
    }

    OncService::StreamBuilder &
    OncService::StreamBuilder::progress(
        ProgressCallback callback)
    {
        progress_callback_ = std::move(callback);
        return *this;
    }

    OncService::Result
    OncService::StreamBuilder::encrypt()
    {
        return service_.encryptStream(
            input_,
            output_,
            password_,
            chunk_size_,
            progress_callback_);
    }

    OncService::Result
    OncService::StreamBuilder::decrypt()
    {
        return service_.decryptStream(
            input_,
            output_,
            password_,
            chunk_size_,
            progress_callback_);
    }

    // ============================================================
    // AdvancedBuilder
    // ============================================================

    OncService::AdvancedBuilder::AdvancedBuilder(
        OncService &service)
        : service_(service)
    {
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::input(
        const Bytes &data)
    {
        data_ = data;
        return *this;
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::input(
        Bytes &&data)
    {
        data_ = std::move(data);
        return *this;
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::password(
        std::string password)
    {
        password_ = std::move(password);
        return *this;
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::algorithm(
        Algorithm algorithm)
    {
        algorithm_ = algorithm;
        return *this;
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::kdf(
        KDF kdf)
    {
        kdf_ = kdf;
        return *this;
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::iterations(
        int iterations)
    {
        iterations_ = iterations;
        return *this;
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::format(
        OutputFormat format)
    {
        output_format_ = format;
        return *this;
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::storeMetadata(
        bool enabled)
    {
        store_metadata_ = enabled;
        return *this;
    }

    OncService::AdvancedBuilder &
    OncService::AdvancedBuilder::verifyIntegrity(
        bool enabled)
    {
        verify_integrity_ = enabled;
        return *this;
    }

    OncService::Result
    OncService::AdvancedBuilder::encrypt()
    {
        std::string error;

        if (!OncService::validatePassword(password_, error))
            return Result::fail(std::move(error));

        if (!OncService::validateData(data_, error))
            return Result::fail(std::move(error));

        if (iterations_ <= 0)
            return Result::fail(
                "PBKDF2 iterations must be greater than zero.");

        crypto::advanced::EncryptionOptions options;

        options.algorithm =
            OncService::toCoreAlgorithm(algorithm_);

        options.kdf =
            OncService::toCoreKDF(kdf_);

        options.iterations =
            iterations_;

        options.outputFormat =
            OncService::toCoreOutputFormat(output_format_);

        options.storeMetadata =
            store_metadata_;

        try
        {
            auto result =
                crypto::advanced::encrypt(
                    data_,
                    password_,
                    options);

            return Result::ok(
                std::move(result));
        }
        catch (const std::exception &exception)
        {
            return Result::fail(
                exception.what(),
                "OnCrypto encryption failed.");
        }
        catch (...)
        {
            return Result::fail(
                "Unknown encryption error.",
                "OnCrypto encryption failed.");
        }
    }

    OncService::Result
    OncService::AdvancedBuilder::decrypt()
    {
        std::string error;

        if (!OncService::validatePassword(password_, error))
            return Result::fail(std::move(error));

        if (!OncService::validateData(data_, error))
            return Result::fail(std::move(error));

        crypto::advanced::DecryptionOptions options;

        options.algorithm =
            OncService::toCoreAlgorithm(algorithm_);

        options.verifyIntegrity =
            verify_integrity_;

        try
        {
            auto result =
                crypto::advanced::decrypt(
                    data_,
                    password_,
                    options);

            return Result::ok(
                std::move(result));
        }
        catch (const std::exception &exception)
        {
            return Result::fail(
                exception.what(),
                "OnCrypto decryption failed.");
        }
        catch (...)
        {
            return Result::fail(
                "Unknown decryption error.",
                "OnCrypto decryption failed.");
        }
    }

    // ============================================================
    // Service Entry Points
    // ============================================================

    OncService::DataBuilder
    OncService::data()
    {
        return DataBuilder(*this);
    }

    OncService::FileBuilder
    OncService::file()
    {
        return FileBuilder(*this);
    }

    OncService::StreamBuilder
    OncService::stream()
    {
        return StreamBuilder(*this);
    }

    OncService::AdvancedBuilder
    OncService::advanced()
    {
        return AdvancedBuilder(*this);
    }

    // ============================================================
    // Information
    // ============================================================

    std::string
    OncService::version()
    {
        try
        {
            return crypto::getVersion();
        }
        catch (...)
        {
            return {};
        }
    }

    std::string
    OncService::algorithmName()
    {
        try
        {
            return crypto::getAlgorithmName();
        }
        catch (...)
        {
            return {};
        }
    }

    // ============================================================
    // Direct Data API
    // ============================================================

    OncService::Result
    OncService::encrypt(
        const Bytes &data,
        const std::string &password,
        Algorithm algorithm)
    {
        std::string error;

        if (!validatePassword(password, error))
            return Result::fail(std::move(error));

        if (!validateData(data, error))
            return Result::fail(std::move(error));

        try
        {
            if (algorithm == Algorithm::Auto)
            {
                return Result::ok(
                    crypto::encrypt(
                        data,
                        password));
            }

            crypto::builder::Encryptor encryptor;

            encryptor
                .password(password)
                .algorithm(
                    toCoreAlgorithm(algorithm));

            return Result::ok(
                encryptor.encrypt(data));
        }
        catch (const std::exception &exception)
        {
            return Result::fail(
                exception.what(),
                "OnCrypto encryption failed.");
        }
        catch (...)
        {
            return Result::fail(
                "Unknown encryption error.",
                "OnCrypto encryption failed.");
        }
    }

    OncService::Result
    OncService::decrypt(
        const Bytes &data,
        const std::string &password,
        Algorithm algorithm)
    {
        std::string error;

        if (!validatePassword(password, error))
            return Result::fail(std::move(error));

        if (!validateData(data, error))
            return Result::fail(std::move(error));

        try
        {
            if (algorithm == Algorithm::Auto)
            {
                return Result::ok(
                    crypto::decrypt(
                        data,
                        password));
            }

            crypto::builder::Decryptor decryptor;

            decryptor
                .password(password)
                .algorithm(
                    toCoreAlgorithm(algorithm));

            return Result::ok(
                decryptor.decrypt(data));
        }
        catch (const std::exception &exception)
        {
            return Result::fail(
                exception.what(),
                "OnCrypto decryption failed.");
        }
        catch (...)
        {
            return Result::fail(
                "Unknown decryption error.",
                "OnCrypto decryption failed.");
        }
    }

    // ============================================================
    // File API
    // ============================================================

    OncService::Result
    OncService::encryptFile(
        const std::string &input,
        const std::string &output,
        const std::string &password,
        Algorithm algorithm)
    {
        std::string error;

        if (!validatePassword(password, error))
            return Result::fail(std::move(error));

        if (!validateFilePaths(input, output, error))
            return Result::fail(std::move(error));

        try
        {
            bool success = false;

            if (algorithm == Algorithm::Auto)
            {
                success =
                    crypto::encryptFile(
                        input,
                        output,
                        password);
            }
            else
            {
                crypto::builder::Encryptor encryptor;

                encryptor
                    .password(password)
                    .algorithm(
                        toCoreAlgorithm(algorithm));

                success =
                    encryptor.encryptFile(
                        input,
                        output);
            }

            if (!success)
            {
                return Result::fail(
                    "OnCrypto failed to encrypt the file.",
                    "File encryption failed.");
            }

            // NEW: Record session stats
            QFileInfo fileInfo(QString::fromStdString(input));
            recordSuccess("encrypt_file", fileInfo.size());

            return Result::ok();
        }
        catch (const std::exception &exception)
        {
            return Result::fail(
                exception.what(),
                "File encryption failed.");
        }
        catch (...)
        {
            return Result::fail(
                "Unknown file encryption error.",
                "File encryption failed.");
        }
    }

    OncService::Result
    OncService::decryptFile(
        const std::string &input,
        const std::string &output,
        const std::string &password,
        Algorithm algorithm)
    {
        std::string error;

        if (!validatePassword(password, error))
            return Result::fail(std::move(error));

        if (!validateFilePaths(input, output, error))
            return Result::fail(std::move(error));

        try
        {
            bool success = false;

            if (algorithm == Algorithm::Auto)
            {
                success =
                    crypto::decryptFile(
                        input,
                        output,
                        password);
            }
            else
            {
                crypto::builder::Decryptor decryptor;

                decryptor
                    .password(password)
                    .algorithm(
                        toCoreAlgorithm(algorithm));

                success =
                    decryptor.decryptFile(
                        input,
                        output);
            }

            if (!success)
            {
                return Result::fail(
                    "OnCrypto failed to decrypt the file.",
                    "File decryption failed.");
            }

            // NEW: Record session stats
            QFileInfo fileInfo(QString::fromStdString(input));
            recordSuccess("decrypt_file", fileInfo.size());

            return Result::ok();
        }
        catch (const std::exception &exception)
        {
            return Result::fail(
                exception.what(),
                "File decryption failed.");
        }
        catch (...)
        {
            return Result::fail(
                "Unknown file decryption error.",
                "File decryption failed.");
        }
    }

    // ============================================================
    // Streaming API
    // ============================================================

    OncService::Result
    OncService::encryptStream(
        const std::string &input,
        const std::string &output,
        const std::string &password,
        std::size_t chunkSize,
        ProgressCallback callback)
    {
        std::string error;

        if (!validatePassword(password, error))
            return Result::fail(std::move(error));

        if (!validateFilePaths(input, output, error))
            return Result::fail(std::move(error));

        if (!validateChunkSize(chunkSize, error))
            return Result::fail(std::move(error));

        try
        {
            ::onc::streaming::ProgressCallback coreCallback = nullptr;

            if (callback)
            {
                coreCallback =
                    [callback](
                        std::uint64_t processed,
                        std::uint64_t total)
                {
                    Progress progress;

                    progress.processed = processed;
                    progress.total = total;

                    callback(progress);
                };
            }

            const bool success =
                ::onc::encryptStream(
                    input,
                    output,
                    password,
                    chunkSize,
                    coreCallback);

            if (!success)
            {
                return Result::fail(
                    "OnCrypto failed to encrypt the stream.",
                    "Streaming encryption failed.");
            }

            if (callback)
            {
                Progress progress;

                progress.processed = 1;
                progress.total = 1;

                callback(progress);
            }

            // NEW: Record session stats
            QFileInfo fileInfo(QString::fromStdString(input));
            recordSuccess("encrypt_stream", fileInfo.size());

            return Result::ok();
        }
        catch (const std::exception &exception)
        {
            return Result::fail(
                exception.what(),
                "Streaming encryption failed.");
        }
        catch (...)
        {
            return Result::fail(
                "Unknown streaming encryption error.",
                "Streaming encryption failed.");
        }
    }

    OncService::Result
    OncService::decryptStream(
        const std::string &input,
        const std::string &output,
        const std::string &password,
        std::size_t chunkSize,
        ProgressCallback callback)
    {
        std::string error;

        if (!validatePassword(password, error))
            return Result::fail(std::move(error));

        if (!validateFilePaths(input, output, error))
            return Result::fail(std::move(error));

        if (!validateChunkSize(chunkSize, error))
            return Result::fail(std::move(error));

        try
        {
            ::onc::streaming::ProgressCallback coreCallback = nullptr;

            if (callback)
            {
                coreCallback =
                    [callback](
                        std::uint64_t processed,
                        std::uint64_t total)
                {
                    Progress progress;

                    progress.processed = processed;
                    progress.total = total;

                    callback(progress);
                };
            }

            const bool success =
                ::onc::decryptStream(
                    input,
                    output,
                    password,
                    chunkSize,
                    coreCallback);

            if (!success)
            {
                return Result::fail(
                    "OnCrypto failed to decrypt the stream.",
                    "Streaming decryption failed.");
            }

            if (callback)
            {
                Progress progress;

                progress.processed = 1;
                progress.total = 1;

                callback(progress);
            }

            // NEW: Record session stats
            QFileInfo fileInfo(QString::fromStdString(input));
            recordSuccess("decrypt_stream", fileInfo.size());

            return Result::ok();
        }
        catch (const std::exception &exception)
        {
            return Result::fail(
                exception.what(),
                "Streaming decryption failed.");
        }
        catch (...)
        {
            return Result::fail(
                "Unknown streaming decryption error.",
                "Streaming decryption failed.");
        }
    }

    // ============================================================
    // Session Tracking
    // ============================================================

    void OncService::recordSuccess(const std::string &operation, std::uint64_t bytes)
    {
        if (operation.find("encrypt") != std::string::npos)
        {
            SessionStats::instance().recordEncrypt(bytes);
        }
        else if (operation.find("decrypt") != std::string::npos)
        {
            SessionStats::instance().recordDecrypt(bytes);
        }

        // TODO: Add to RecentFiles when UI is ready
        // RecentFiles::instance().add({...});
    }

    // ============================================================
    // Mapping
    // ============================================================

    crypto::builder::Algorithm
    OncService::toCoreAlgorithm(
        Algorithm algorithm)
    {
        switch (algorithm)
        {
        case Algorithm::Auto:
            return crypto::builder::Algorithm::Auto;

        case Algorithm::AES256_GCM:
            return crypto::builder::Algorithm::AES256_GCM;

        case Algorithm::ChaCha20:
            return crypto::builder::Algorithm::ChaCha20;

        case Algorithm::XChaCha20:
            return crypto::builder::Algorithm::XChaCha20;
        }

        return crypto::builder::Algorithm::Auto;
    }

    crypto::advanced::KDF
    OncService::toCoreKDF(
        KDF kdf)
    {
        switch (kdf)
        {
        case KDF::PBKDF2:
            return crypto::advanced::KDF::PBKDF2;

        case KDF::Argon2:
            return crypto::advanced::KDF::Argon2;
        }

        return crypto::advanced::KDF::PBKDF2;
    }

    crypto::advanced::OutputFormat
    OncService::toCoreOutputFormat(
        OutputFormat format)
    {
        switch (format)
        {
        case OutputFormat::Binary:
            return crypto::advanced::OutputFormat::Binary;

        case OutputFormat::Hex:
            return crypto::advanced::OutputFormat::Hex;

        case OutputFormat::Base64:
            return crypto::advanced::OutputFormat::Base64;
        }

        return crypto::advanced::OutputFormat::Binary;
    }

    // ============================================================
    // Validation
    // ============================================================

    bool
    OncService::validatePassword(
        const std::string &password,
        std::string &error)
    {
        if (password.empty())
        {
            error = "Password cannot be empty.";
            return false;
        }

        return true;
    }

    bool
    OncService::validateData(
        const Bytes &data,
        std::string &error)
    {
        if (data.empty())
        {
            error = "Input data cannot be empty.";
            return false;
        }

        return true;
    }

    bool
    OncService::validateFilePaths(
        const std::string &input,
        const std::string &output,
        std::string &error)
    {
        if (input.empty())
        {
            error = "Input file path cannot be empty.";
            return false;
        }

        if (output.empty())
        {
            error = "Output file path cannot be empty.";
            return false;
        }

        if (input == output)
        {
            error = "Input and output paths must be different.";
            return false;
        }

        return true;
    }

    bool
    OncService::validateChunkSize(
        std::size_t chunkSize,
        std::string &error)
    {
        if (chunkSize == 0)
        {
            error = "Chunk size cannot be zero.";
            return false;
        }

        constexpr std::size_t maxChunkSize =
            64ULL * 1024ULL * 1024ULL;

        if (chunkSize > maxChunkSize)
        {
            error = "Chunk size cannot exceed 64 MiB.";
            return false;
        }

        return true;
    }

} // namespace gui::core