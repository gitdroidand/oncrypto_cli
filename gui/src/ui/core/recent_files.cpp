#include "gui/ui/core/recent_files.hpp"

#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>
#include <QDateTime>
#include <algorithm>

namespace gui::core
{
    namespace
    {
        constexpr auto kOrg = "OnCrypto";
        constexpr auto kApp = "OnCryptoDesktop";
        constexpr auto kKey = "recent_files";
    }

    RecentFiles &RecentFiles::instance()
    {
        static RecentFiles inst;
        return inst;
    }

    RecentFiles::RecentFiles()
    {
        load();
    }

    // save() بعد از هر تغییر (add/clear) انجام می‌شود؛
    // ساخت QSettings هنگام نابودی singleton لازم نیست و ریسک دارد.
    RecentFiles::~RecentFiles() = default;

    void RecentFiles::add(const RecentFileEntry &entry)
    {
        QMutexLocker lock(&mutex_);

        entries_.prepend(entry); // جدیدترین اول
        while (entries_.size() > kMaxEntries)
            entries_.removeLast();

        save();
    }

    QVector<RecentFileEntry> RecentFiles::list(int max_count) const
    {
        QMutexLocker lock(&mutex_);
        return entries_.mid(0, qMax(0, max_count));
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
        return int(std::count_if(entries_.cbegin(), entries_.cend(),
                                 [](const RecentFileEntry &e)
                                 { return e.success && e.operation.startsWith("encrypt"); }));
    }

    int RecentFiles::totalDecrypted() const
    {
        QMutexLocker lock(&mutex_);
        return int(std::count_if(entries_.cbegin(), entries_.cend(),
                                 [](const RecentFileEntry &e)
                                 { return e.success && e.operation.startsWith("decrypt"); }));
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

    // فرض: caller قفل mutex_ را دارد (add/clear)
    void RecentFiles::save()
    {
        QJsonArray array;
        for (const auto &e : entries_)
        {
            QJsonObject obj;
            obj["file_path"] = e.file_path;
            obj["operation"] = e.operation;
            obj["algorithm"] = e.algorithm;
            obj["file_size"] = e.file_size;
            obj["timestamp"] = e.timestamp.toString(Qt::ISODateWithMs);
            obj["success"] = e.success;
            obj["output_path"] = e.output_path;
            array.append(obj);
        }

        QSettings settings(kOrg, kApp);
        settings.setValue(kKey, QJsonDocument(array).toJson(QJsonDocument::Compact));
        settings.sync();
    }

    void RecentFiles::load()
    {
        QMutexLocker lock(&mutex_);
        entries_.clear();

        QSettings settings(kOrg, kApp);
        const QByteArray data = settings.value(kKey).toByteArray();
        if (data.isEmpty())
            return;

        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isArray())
            return;

        // همان ترتیب ذخیره‌شده (جدیدترین اول) را می‌خوانیم؛
        // نسخه‌ی قبلی آرایه را معکوس می‌خواند و ترتیب در هر اجرا برعکس می‌شد.
        for (const QJsonValue &val : doc.array())
        {
            if (!val.isObject())
                continue;

            const QJsonObject obj = val.toObject();

            RecentFileEntry e;
            e.file_path   = obj["file_path"].toString();
            e.operation   = obj["operation"].toString();
            e.algorithm   = obj["algorithm"].toString();
            e.file_size   = obj["file_size"].toVariant().toLongLong();
            e.timestamp   = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
            e.success     = obj["success"].toBool();
            e.output_path = obj["output_path"].toString();

            if (e.operation.isEmpty())
                continue; // ردیف خراب

            entries_.append(e);
        }

        // ترمیم داده‌های قبلاً به‌هم‌ریخته: جدیدترین اول، تایم‌استمپ نامعتبر = قدیمی‌ترین
        std::stable_sort(entries_.begin(), entries_.end(),
                         [](const RecentFileEntry &a, const RecentFileEntry &b)
                         {
                             if (a.timestamp.isValid() != b.timestamp.isValid())
                                 return a.timestamp.isValid();
                             return a.timestamp > b.timestamp;
                         });

        while (entries_.size() > kMaxEntries)
            entries_.removeLast();

        // تایم‌استمپ نامعتبر را بعد از مرتب‌سازی پر می‌کنیم تا ترتیب به‌هم نخورد
        for (auto &e : entries_)
        {
            if (!e.timestamp.isValid())
                e.timestamp = QDateTime::currentDateTime();
        }
    }

} // namespace gui::core