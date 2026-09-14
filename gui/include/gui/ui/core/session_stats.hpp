#ifndef GUI_UI_CORE_SESSION_STATS_HPP
#define GUI_UI_CORE_SESSION_STATS_HPP

#include <QMutex>
#include <cstdint>

namespace gui::core
{

    class SessionStats
    {
    public:
        static SessionStats &instance();

        int encryptedCount() const;
        int decryptedCount() const;
        std::uint64_t bytesProcessed() const;

        void recordEncrypt(std::uint64_t bytes);
        void recordDecrypt(std::uint64_t bytes);

    private:
        SessionStats() = default;

        mutable QMutex mutex_;
        int encrypted_count_ = 0;
        int decrypted_count_ = 0;
        std::uint64_t bytes_processed_ = 0;
    };

} // namespace gui::core

#endif