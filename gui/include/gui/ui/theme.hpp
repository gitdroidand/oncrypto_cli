#pragma once

#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QString>

namespace oncrypto::ui {

enum class ThemeMode {
    System,
    Light,
    Dark
};

class Theme final
{
public:
    Theme() = delete;

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    static void initialize();

    // -------------------------------------------------------------------------
    // Theme mode
    // -------------------------------------------------------------------------

    static ThemeMode mode();

    static void set_mode(
        ThemeMode mode
    );

    static void refresh();

    static bool is_dark();

    // -------------------------------------------------------------------------
    // Semantic colors
    // -------------------------------------------------------------------------

    static QColor background();

    static QColor foreground();

    static QColor surface();

    static QColor surface_variant();

    static QColor foreground_secondary();

    static QColor foreground_disabled();

    static QColor border();

    static QColor border_strong();

    static QColor primary();

    static QColor primary_hover();

    static QColor primary_pressed();

    static QColor accent();

    static QColor accent_hover();

    static QColor selection();

    static QColor icon();

    // -------------------------------------------------------------------------
    // Qt palette
    // -------------------------------------------------------------------------

    static QPalette palette();

    // -------------------------------------------------------------------------
    // Icons
    // -------------------------------------------------------------------------

    static QIcon recolor_icon(
        const QIcon& source,
        const QColor& color
    );

    static QIcon icon(
        const QString& resource_path
    );

    // -------------------------------------------------------------------------
    // Application style
    // -------------------------------------------------------------------------

    static QString application_style();

private:
    static void apply_palette();

private:
    static ThemeMode current_mode_;
};

} // namespace oncrypto::ui