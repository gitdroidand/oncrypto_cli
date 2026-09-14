#ifndef GUI_UI_CORE_PASSWORD_VALIDATOR_HPP
#define GUI_UI_CORE_PASSWORD_VALIDATOR_HPP

#include <QString>
#include <QVector>

namespace gui::core
{

    struct PasswordStrength
    {
        enum class Level
        {
            Empty,
            VeryWeak,
            Weak,
            Fair,
            Good,
            Strong
        };

        Level level = Level::Empty;
        int score = 0; // 0-100
        int length = 0;
        bool has_upper = false;
        bool has_lower = false;
        bool has_digit = false;
        bool has_special = false;
        QVector<QString> suggestions;
    };

    class PasswordValidator
    {
    public:
        static constexpr int kMinLength = 8;
        static constexpr int kStrongLength = 16;

        static PasswordStrength analyze(const QString &password);
        static bool isValid(const QString &password, QString &error);
        static QString strengthLabel(PasswordStrength::Level level);
        static QString strengthColor(PasswordStrength::Level level);

    private:
        static int calculateScore(const QString &password);
    };

} // namespace gui::core

#endif