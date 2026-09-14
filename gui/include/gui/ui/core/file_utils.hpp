#ifndef GUI_UI_CORE_FILE_UTILS_HPP
#define GUI_UI_CORE_FILE_UTILS_HPP

#include <QString>
#include <QFileInfo>
#include <optional>
#include <QObject>

namespace gui::core
{

    struct FileOperationResult
    {
        bool success = false;
        QString path;
        QString error;
        bool was_renamed = false;
        QString original_path;
    };

    class FileUtils
    {
    public:
        // Auto-generate encrypted output path: file.txt → file.txt.onc
        static QString autoEncryptPath(const QString &input_path);

        // Auto-generate decrypted output path: file.txt.onc → file.txt
        static QString autoDecryptPath(const QString &input_path);

        // Check if file exists, auto-rename with (1), (2), etc
        static FileOperationResult resolveOutputPath(
            const QString &desired_path,
            bool allow_overwrite = false);

        // Validate input file exists and is readable
        static bool validateInputFile(const QString &path, QString &error);

        // Check available disk space (platform-agnostic via Qt)
        static bool hasEnoughSpace(const QString &path, qint64 required_bytes, QString &error);

        // Format bytes to human readable: 1024 → "1 KB", 1536000 → "1.5 MB"
        static QString formatBytes(qint64 bytes);

        // Get file size safely
        static qint64 fileSize(const QString &path);

        // Ensure directory exists for output
        static bool ensureDirectory(const QString &file_path, QString &error);

    private:
        static QString findUniqueName(const QString &base_path);
    };

} // namespace gui::core

#endif