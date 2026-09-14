#include "gui/ui/core/file_utils.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QRegularExpression>

namespace gui::core
{

    QString FileUtils::autoEncryptPath(const QString &input_path)
    {
        QFileInfo info(input_path);
        QString dir = info.absolutePath();
        QString base = info.completeBaseName();
        QString suffix = info.suffix();

        // If already has .onc, don't double it
        if (suffix == "onc")
        {
            return dir + "/" + base + ".enc.onc";
        }

        QString new_name = base;
        if (!suffix.isEmpty())
        {
            new_name += "." + suffix;
        }
        new_name += ".onc";

        return dir + "/" + new_name;
    }

    QString FileUtils::autoDecryptPath(const QString &input_path)
    {
        QFileInfo info(input_path);
        QString dir = info.absolutePath();
        QString base = info.completeBaseName();
        QString suffix = info.suffix();

        // If input is .onc, remove it
        if (suffix == "onc")
        {
            // Check if there's another suffix before .onc
            QString full_base = info.fileName();
            full_base.chop(4); // Remove .onc

            QFileInfo base_info(full_base);
            QString inner_base = base_info.completeBaseName();
            QString inner_suffix = base_info.suffix();

            QString result = dir + "/" + inner_base;
            if (!inner_suffix.isEmpty() && inner_suffix != "onc")
            {
                result += "." + inner_suffix;
            }
            return result;
        }

        // Fallback: append .dec
        return dir + "/" + base + ".dec";
    }

    FileOperationResult FileUtils::resolveOutputPath(
        const QString &desired_path,
        bool allow_overwrite)
    {
        FileOperationResult result;
        result.original_path = desired_path;

        QFileInfo info(desired_path);

        // Ensure directory exists
        QString error;
        if (!ensureDirectory(desired_path, error))
        {
            result.error = error;
            return result;
        }

        // If file doesn't exist or overwrite allowed, use as-is
        if (!info.exists() || allow_overwrite)
        {
            result.success = true;
            result.path = desired_path;
            return result;
        }

        // Auto-rename with (1), (2), etc
        QString unique = findUniqueName(desired_path);
        result.success = true;
        result.path = unique;
        result.was_renamed = true;

        return result;
    }

    bool FileUtils::validateInputFile(const QString &path, QString &error)
    {
        if (path.isEmpty())
        {
            error = QObject::tr("No file selected.");
            return false;
        }

        QFileInfo info(path);
        if (!info.exists())
        {
            error = QObject::tr("File does not exist: %1").arg(path);
            return false;
        }

        if (!info.isFile())
        {
            error = QObject::tr("Path is not a file: %1").arg(path);
            return false;
        }

        if (!info.isReadable())
        {
            error = QObject::tr("File is not readable: %1").arg(path);
            return false;
        }

        if (info.size() == 0)
        {
            error = QObject::tr("File is empty: %1").arg(path);
            return false;
        }

        return true;
    }

    bool FileUtils::hasEnoughSpace(const QString &path, qint64 required_bytes, QString &error)
    {
        QFileInfo info(path);
        QStorageInfo storage(info.absolutePath());

        if (!storage.isValid())
        {
            error = QObject::tr("Cannot determine available disk space.");
            return false;
        }

        qint64 available = storage.bytesAvailable();
        if (available < required_bytes)
        {
            error = QObject::tr("Not enough disk space. Need %1, have %2.")
                        .arg(formatBytes(required_bytes))
                        .arg(formatBytes(available));
            return false;
        }

        return true;
    }

    QString FileUtils::formatBytes(qint64 bytes)
    {
        if (bytes < 0)
            return QObject::tr("Unknown");

        const char *units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit_index = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unit_index < 4)
        {
            size /= 1024.0;
            unit_index++;
        }

        if (unit_index == 0)
        {
            return QString("%1 %2").arg(bytes).arg(units[unit_index]);
        }

        return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unit_index]);
    }

    qint64 FileUtils::fileSize(const QString &path)
    {
        QFileInfo info(path);
        return info.exists() ? info.size() : -1;
    }

    bool FileUtils::ensureDirectory(const QString &file_path, QString &error)
    {
        QFileInfo info(file_path);
        QDir dir = info.absoluteDir();

        if (dir.exists())
            return true;

        if (!dir.mkpath("."))
        {
            error = QObject::tr("Cannot create directory: %1").arg(dir.absolutePath());
            return false;
        }

        return true;
    }

    QString FileUtils::findUniqueName(const QString &base_path)
    {
        QFileInfo info(base_path);
        QString dir = info.absolutePath();
        QString base = info.completeBaseName();
        QString suffix = info.suffix();
        QString ext = suffix.isEmpty() ? "" : "." + suffix;

        // Check if already has (N) pattern
        QRegularExpression re("\\((\\d+)\\)$");
        auto match = re.match(base);

        int counter = 1;
        QString clean_base = base;

        if (match.hasMatch())
        {
            counter = match.captured(1).toInt() + 1;
            clean_base = base.left(match.capturedStart());
        }

        while (true)
        {
            QString candidate = QString("%1/%2 (%3)%4")
                                    .arg(dir)
                                    .arg(clean_base)
                                    .arg(counter)
                                    .arg(ext);

            if (!QFile::exists(candidate))
                return candidate;

            counter++;

            // Safety limit
            if (counter > 9999)
                return base_path + ".duplicate";
        }
    }

} // namespace gui::core