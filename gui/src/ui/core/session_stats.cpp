#include "gui/ui/core/session_stats.hpp"

namespace gui::core
{

    SessionStats &SessionStats::instance()
    {
        static SessionStats inst;
        return inst;
    }

    int SessionStats::encryptedCount() const
    {
        QMutexLocker lock(&mutex_);
        return encrypted_count_;
    }

    int SessionStats::decryptedCount() const
    {
        QMutexLocker lock(&mutex_);
        return decrypted_count_;
    }

    std::uint64_t SessionStats::bytesProcessed() const
    {
        QMutexLocker lock(&mutex_);
        return bytes_processed_;
    }

    void SessionStats::recordEncrypt(std::uint64_t bytes)
    {
        QMutexLocker lock(&mutex_);
        encrypted_count_++;
        bytes_processed_ += bytes;
    }

    void SessionStats::recordDecrypt(std::uint64_t bytes)
    {
        QMutexLocker lock(&mutex_);
        decrypted_count_++;
        bytes_processed_ += bytes;
    }

} // namespace gui::core