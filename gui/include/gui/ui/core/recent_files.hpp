#ifndef GUI_UI_CORE_RECENT_FILES_HPP
#define GUI_UI_CORE_RECENT_FILES_HPP

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QMutex>

namespace gui::core
{

    struct RecentFileEntry
    {
        QString file_path;
        QString operation; // "encrypt" | "decrypt" | "encrypt_text" | "decrypt_text"
        QString algorithm;
        qint64 file_size = 0;
        QDateTime timestamp;
        bool success = true;
        QString output_path; // Where result was saved
    };

    class RecentFiles
    {
    public:
        static RecentFiles &instance();

        void add(const RecentFileEntry &entry);
        QVector<RecentFileEntry> list(int max_count = 10) const;
        void clear();

        // Stats derived from recent
        int totalEncrypted() const;
        int totalDecrypted() const;
        qint64 totalBytesProcessed() const;

    private:
        RecentFiles();
        ~RecentFiles();

        void save();
        void load();

        mutable QMutex mutex_;
        QVector<RecentFileEntry> entries_;
        static constexpr int kMaxEntries = 50;
    };

} // namespace gui::core

#endif