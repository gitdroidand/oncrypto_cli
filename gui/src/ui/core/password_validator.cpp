#include "gui/ui/core/password_validator.hpp"

#include <QRegularExpression>

namespace gui::core
{

    PasswordStrength PasswordValidator::analyze(const QString &password)
    {
        PasswordStrength result;
        result.length = password.length();

        if (password.isEmpty())
            return result;

        result.has_lower = password.contains(QRegularExpression("[a-z]"));
        result.has_upper = password.contains(QRegularExpression("[A-Z]"));
        result.has_digit = password.contains(QRegularExpression("[0-9]"));
        result.has_special = password.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]{};':\"\\\\|,.<>\\/?]"));

        result.score = calculateScore(password);

        if (result.score < 20)
            result.level = PasswordStrength::Level::VeryWeak;
        else if (result.score < 40)
            result.level = PasswordStrength::Level::Weak;
        else if (result.score < 60)
            result.level = PasswordStrength::Level::Fair;
        else if (result.score < 80)
            result.level = PasswordStrength::Level::Good;
        else
            result.level = PasswordStrength::Level::Strong;

        // Suggestions
        if (result.length < kMinLength)
            result.suggestions.append(QObject::tr("Use at least %1 characters").arg(kMinLength));
        if (!result.has_upper)
            result.suggestions.append(QObject::tr("Add uppercase letters"));
        if (!result.has_lower)
            result.suggestions.append(QObject::tr("Add lowercase letters"));
        if (!result.has_digit)
            result.suggestions.append(QObject::tr("Add numbers"));
        if (!result.has_special)
            result.suggestions.append(QObject::tr("Add special characters (!@#$...)"));
        if (result.length < kStrongLength && result.score < 80)
            result.suggestions.append(QObject::tr("Consider using %1+ characters for stronger security").arg(kStrongLength));

        return result;
    }

    bool PasswordValidator::isValid(const QString &password, QString &error)
    {
        if (password.isEmpty())
        {
            error = QObject::tr("Password cannot be empty.");
            return false;
        }

        if (password.length() < kMinLength)
        {
            error = QObject::tr("Password must be at least %1 characters.").arg(kMinLength);
            return false;
        }

        auto strength = analyze(password);
        if (strength.level == PasswordStrength::Level::VeryWeak)
        {
            error = QObject::tr("Password is too weak.");
            return false;
        }

        return true;
    }

    QString PasswordValidator::strengthLabel(PasswordStrength::Level level)
    {
        switch (level)
        {
        case PasswordStrength::Level::Empty:
            return QObject::tr("Enter password");
        case PasswordStrength::Level::VeryWeak:
            return QObject::tr("Very Weak");
        case PasswordStrength::Level::Weak:
            return QObject::tr("Weak");
        case PasswordStrength::Level::Fair:
            return QObject::tr("Fair");
        case PasswordStrength::Level::Good:
            return QObject::tr("Good");
        case PasswordStrength::Level::Strong:
            return QObject::tr("Strong");
        }
        return QObject::tr("Unknown");
    }

    QString PasswordValidator::strengthColor(PasswordStrength::Level level)
    {
        switch (level)
        {
        case PasswordStrength::Level::Empty:
            return "#9CA3AF";
        case PasswordStrength::Level::VeryWeak:
            return "#EF4444";
        case PasswordStrength::Level::Weak:
            return "#F97316";
        case PasswordStrength::Level::Fair:
            return "#EAB308";
        case PasswordStrength::Level::Good:
            return "#22C55E";
        case PasswordStrength::Level::Strong:
            return "#15803D";
        }
        return "#9CA3AF";
    }

    int PasswordValidator::calculateScore(const QString &password)
    {
        int score = 0;
        int length = password.length();

        // Length score (up to 40)
        score += qMin(length * 4, 40);

        // Character variety (up to 40)
        if (password.contains(QRegularExpression("[a-z]")))
            score += 10;
        if (password.contains(QRegularExpression("[A-Z]")))
            score += 10;
        if (password.contains(QRegularExpression("[0-9]")))
            score += 10;
        if (password.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]{};':\"\\\\|,.<>\\/?]")))
            score += 10;

        // Bonus for mixed case + numbers + special (up to 20)
        int variety = 0;
        if (password.contains(QRegularExpression("[a-z]")))
            variety++;
        if (password.contains(QRegularExpression("[A-Z]")))
            variety++;
        if (password.contains(QRegularExpression("[0-9]")))
            variety++;
        if (password.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]{};':\"\\\\|,.<>\\/?]")))
            variety++;
        score += variety * 5;

        // Penalty for repetition
        QRegularExpression reRepeat("(.)\\1{2,}");
        auto match = reRepeat.match(password);
        if (match.hasMatch())
            score -= 10;

        // Penalty for sequential
        QRegularExpression reSeq("(abc|bcd|cde|def|efg|fgh|ghi|hij|ijk|jkl|klm|lmn|mno|nop|opq|pqr|qrs|rst|stu|tuv|uvw|vwx|wxy|xyz|012|123|234|345|456|567|678|789|890)", QRegularExpression::CaseInsensitiveOption);
        if (reSeq.match(password).hasMatch())
            score -= 10;

        return qBound(0, score, 100);
    }

} // namespace gui::core