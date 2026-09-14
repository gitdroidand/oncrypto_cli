#include "gui/ui/core/recent_files.hpp"

#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace gui::core
{

    RecentFiles &RecentFiles::instance()
    {
        static RecentFiles inst;
        return inst;
    }

    RecentFiles::RecentFiles()
    {
        load();
    }

    RecentFiles::~RecentFiles()
    {
        save();
    }

    void RecentFiles::add(const RecentFileEntry &entry)
    {
        QMutexLocker lock(&mutex_);

        entries_.prepend(entry);

        while (entries_.size() > kMaxEntries)
            entries_.removeLast();

        save();
    }

    QVector<RecentFileEntry> RecentFiles::list(int max_count) const
    {
        QMutexLocker lock(&mutex_);
        return entries_.mid(0, max_count);
    }

    void RecentFiles::clear()
    {
        QMutexLocker lock(&mutex_);
        entries_.clear();
        save();
    }

    int RecentFiles::totalEncrypted() const
    {
        QMutexLocker lock(&mutex_);
        int count = 0;
        for (const auto &e : entries_)
        {
            if (e.operation.startsWith("encrypt") && e.success)
                count++;
        }
        return count;
    }

    int RecentFiles::totalDecrypted() const
    {
        QMutexLocker lock(&mutex_);
        int count = 0;
        for (const auto &e : entries_)
        {
            if (e.operation.startsWith("decrypt") && e.success)
                count++;
        }
        return count;
    }

    qint64 RecentFiles::totalBytesProcessed() const
    {
        QMutexLocker lock(&mutex_);
        qint64 total = 0;
        for (const auto &e : entries_)
        {
            if (e.success)
                total += e.file_size;
        }
        return total;
    }

    void RecentFiles::save()
    {
        QSettings settings("OnCrypto", "OnCryptoDesktop");

        QJsonArray array;
        for (const auto &e : entries_)
        {
            QJsonObject obj;
            obj["file_path"] = e.file_path;
            obj["operation"] = e.operation;
            obj["algorithm"] = e.algorithm;
            obj["file_size"] = e.file_size;
            obj["timestamp"] = e.timestamp.toString(Qt::ISODate);
            obj["success"] = e.success;
            obj["output_path"] = e.output_path;
            array.append(obj);
        }

        settings.setValue("recent_files", QJsonDocument(array).toJson(QJsonDocument::Compact));
    }

    void RecentFiles::load()
    {
        QSettings settings("OnCrypto", "OnCryptoDesktop");

        QByteArray data = settings.value("recent_files").toByteArray();
        if (data.isEmpty())
            return;

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isArray())
            return;

        QJsonArray array = doc.array();
        for (const auto &val : array)
        {
            if (!val.isObject())
                continue;

            QJsonObject obj = val.toObject();
            RecentFileEntry e;
            e.file_path = obj["file_path"].toString();
            e.operation = obj["operation"].toString();
            e.algorithm = obj["algorithm"].toString();
            e.file_size = obj["file_size"].toVariant().toLongLong();
            e.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
            e.success = obj["success"].toBool();
            e.output_path = obj["output_path"].toString();
            entries_.append(e);
        }
    }

} // namespace gui::core