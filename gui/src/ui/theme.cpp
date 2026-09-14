#include "gui/ui/theme.hpp"

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QImage>
#include <QMetaObject>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyleHints>
#include <QString>
#include <QWidget>
#include <QSvgRenderer>

namespace oncrypto::ui
{

    ThemeMode Theme::current_mode_ = ThemeMode::System;

    namespace
    {

        constexpr const char *kAccent = "#B73974";
        constexpr const char *kAccentLight = "#A52F68";
        constexpr const char *kAccentDark = "#D75A91";

        constexpr const char *kLightBackground = "#F7F5F8";
        constexpr const char *kLightSurface = "#FFFFFF";
        constexpr const char *kLightSurfaceVariant = "#F0EDF2";
        constexpr const char *kLightForeground = "#211D22";
        constexpr const char *kLightForegroundSecondary = "#665F68";
        constexpr const char *kLightForegroundDisabled = "#A8A2AA";
        constexpr const char *kLightBorder = "#DED9E0";
        constexpr const char *kLightBorderStrong = "#CAC3CC";
        constexpr const char *kLightSelection = "#E9B9CE";

        constexpr const char *kDarkBackground = "#15152B";
        constexpr const char *kDarkSurface = "#1F2040";
        constexpr const char *kDarkSurfaceVariant = "#292A4D";
        constexpr const char *kDarkForeground = "#F6F3F7";
        constexpr const char *kDarkForegroundSecondary = "#BDB8C2";
        constexpr const char *kDarkForegroundDisabled = "#696572";
        constexpr const char *kDarkBorder = "#383955";
        constexpr const char *kDarkBorderStrong = "#4A4B69";
        constexpr const char *kDarkSelection = "#71304F";

        QColor color_from_hex(const char *value)
        {
            return QColor(QString::fromLatin1(value));
        }

        QString css_color(const QColor &color)
        {
            return color.name(QColor::HexArgb);
        }

        void schedule_theme_refresh()
        {
            if (!qApp)
                return;

            QMetaObject::invokeMethod(
                qApp,
                []
                {
                    if (!qApp)
                        return;

                    if (Theme::mode() != ThemeMode::System)
                        return;

                    Theme::refresh();
                },
                Qt::QueuedConnection);
        }

        // ---------------------------------------------------------------------
        // Product-grade tinting (GPU-accelerated, preserves anti-aliasing)
        // ---------------------------------------------------------------------
        QPixmap tint_pixmap(const QPixmap &source, const QColor &color)
        {
            if (source.isNull())
                return QPixmap();

            QPixmap result(source.size());
            result.setDevicePixelRatio(source.devicePixelRatio());
            result.fill(Qt::transparent);

            QPainter painter(&result);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPixmap(0, 0, source);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(result.rect(), color);
            painter.end();

            return result;
        }

        // ---------------------------------------------------------------------
        // Crisp SVG rendering at exact sizes (vector → tinted raster)
        // ---------------------------------------------------------------------
        QIcon load_tinted_svg(const QString &path, const QColor &color, const QColor &disabled)
        {
            QIcon result;
            QSvgRenderer renderer(path);
            if (!renderer.isValid())
                return result;

            const QList<QSize> sizes = {
                QSize(16, 16), QSize(20, 20), QSize(24, 24),
                QSize(26, 26), QSize(28, 28), QSize(32, 32),
                QSize(48, 48), QSize(52, 52), QSize(56, 56),
                QSize(64, 64)};

            for (const QSize &size : sizes)
            {
                // Normal
                QPixmap pm(size);
                pm.fill(Qt::transparent);
                QPainter p(&pm);
                p.setRenderHint(QPainter::Antialiasing);
                renderer.render(&p);
                p.setCompositionMode(QPainter::CompositionMode_SourceIn);
                p.fillRect(pm.rect(), color);
                p.end();
                result.addPixmap(pm, QIcon::Normal, QIcon::Off);

                // Disabled
                QPixmap pm_d(size);
                pm_d.fill(Qt::transparent);
                QPainter p_d(&pm_d);
                p_d.setRenderHint(QPainter::Antialiasing);
                renderer.render(&p_d);
                p_d.setCompositionMode(QPainter::CompositionMode_SourceIn);
                p_d.fillRect(pm_d.rect(), disabled);
                p_d.end();
                result.addPixmap(pm_d, QIcon::Disabled, QIcon::Off);
            }

            return result;
        }

    } // namespace

    void Theme::initialize()
    {
        if (!qApp)
            return;

        apply_palette();

        auto *hints = qApp->styleHints();

        if (!hints)
            return;

        QObject::connect(
            hints,
            &QStyleHints::colorSchemeChanged,
            qApp,
            [](Qt::ColorScheme)
            {
                if (Theme::mode() != ThemeMode::System)
                    return;

                schedule_theme_refresh();
            });
    }

    ThemeMode Theme::mode()
    {
        return current_mode_;
    }

    void Theme::set_mode(ThemeMode mode)
    {
        if (current_mode_ == mode)
        {
            refresh();
            return;
        }

        current_mode_ = mode;
        apply_palette();
    }

    void Theme::refresh()
    {
        apply_palette();
    }

    bool Theme::is_dark()
    {
        switch (current_mode_)
        {
        case ThemeMode::Dark:
            return true;

        case ThemeMode::Light:
            return false;

        case ThemeMode::System:
            break;
        }

        if (qApp)
        {
            if (auto *hints = qApp->styleHints())
            {
                const auto scheme = hints->colorScheme();

                if (scheme == Qt::ColorScheme::Dark)
                    return true;

                if (scheme == Qt::ColorScheme::Light)
                    return false;
            }
        }

        return false;
    }

    QColor Theme::background()
    {
        return color_from_hex(
            is_dark() ? kDarkBackground : kLightBackground);
    }

    QColor Theme::foreground()
    {
        return color_from_hex(
            is_dark() ? kDarkForeground : kLightForeground);
    }

    QColor Theme::surface()
    {
        return color_from_hex(
            is_dark() ? kDarkSurface : kLightSurface);
    }

    QColor Theme::surface_variant()
    {
        return color_from_hex(
            is_dark()
                ? kDarkSurfaceVariant
                : kLightSurfaceVariant);
    }

    QColor Theme::foreground_secondary()
    {
        return color_from_hex(
            is_dark()
                ? kDarkForegroundSecondary
                : kLightForegroundSecondary);
    }

    QColor Theme::foreground_disabled()
    {
        return color_from_hex(
            is_dark()
                ? kDarkForegroundDisabled
                : kLightForegroundDisabled);
    }

    QColor Theme::border()
    {
        return color_from_hex(
            is_dark() ? kDarkBorder : kLightBorder);
    }

    QColor Theme::border_strong()
    {
        return color_from_hex(
            is_dark()
                ? kDarkBorderStrong
                : kLightBorderStrong);
    }

    QColor Theme::accent()
    {
        return color_from_hex(kAccent);
    }

    QColor Theme::accent_hover()
    {
        return color_from_hex(
            is_dark() ? kAccentDark : kAccentLight);
    }

    QColor Theme::primary()
    {
        return accent();
    }

    QColor Theme::primary_hover()
    {
        return accent_hover();
    }

    QColor Theme::selection()
    {
        return color_from_hex(
            is_dark() ? kDarkSelection : kLightSelection);
    }

    QColor Theme::icon()
    {
        return foreground();
    }

    QPalette Theme::palette()
    {
        QPalette palette;

        const QColor background_color = Theme::background();
        const QColor surface_color = Theme::surface();
        const QColor surface_variant_color = Theme::surface_variant();
        const QColor foreground_color = Theme::foreground();
        const QColor secondary_color = Theme::foreground_secondary();
        const QColor disabled_color = Theme::foreground_disabled();
        const QColor border_color = Theme::border();
        const QColor accent_color = Theme::accent();
        const QColor selection_color = Theme::selection();

        palette.setColor(
            QPalette::Window,
            background_color);

        palette.setColor(
            QPalette::WindowText,
            foreground_color);

        palette.setColor(
            QPalette::Base,
            surface_color);

        palette.setColor(
            QPalette::AlternateBase,
            surface_variant_color);

        palette.setColor(
            QPalette::Text,
            foreground_color);

        palette.setColor(
            QPalette::Button,
            surface_color);

        palette.setColor(
            QPalette::ButtonText,
            foreground_color);

        palette.setColor(
            QPalette::Mid,
            border_color);

        palette.setColor(
            QPalette::Dark,
            border_color);

        palette.setColor(
            QPalette::Shadow,
            QColor(
                0,
                0,
                0,
                is_dark() ? 110 : 30));

        palette.setColor(
            QPalette::PlaceholderText,
            secondary_color);

        palette.setColor(
            QPalette::ToolTipBase,
            surface_color);

        palette.setColor(
            QPalette::ToolTipText,
            foreground_color);

        palette.setColor(
            QPalette::Link,
            accent_color);

        palette.setColor(
            QPalette::LinkVisited,
            accent_hover());

        palette.setColor(
            QPalette::Highlight,
            selection_color);

        palette.setColor(
            QPalette::HighlightedText,
            is_dark()
                ? foreground_color
                : color_from_hex("#FFFFFF"));

        palette.setColor(
            QPalette::Disabled,
            QPalette::WindowText,
            disabled_color);

        palette.setColor(
            QPalette::Disabled,
            QPalette::Text,
            disabled_color);

        palette.setColor(
            QPalette::Disabled,
            QPalette::ButtonText,
            disabled_color);

        palette.setColor(
            QPalette::Disabled,
            QPalette::PlaceholderText,
            disabled_color);

        return palette;
    }

    QString Theme::application_style()
    {
        const QString background_color =
            css_color(Theme::background());

        const QString surface_color =
            css_color(Theme::surface());

        const QString surface_variant_color =
            css_color(Theme::surface_variant());

        const QString foreground_color =
            css_color(Theme::foreground());

        const QString secondary_color =
            css_color(Theme::foreground_secondary());

        const QString disabled_color =
            css_color(Theme::foreground_disabled());

        const QString border_color =
            css_color(Theme::border());

        const QString border_strong_color =
            css_color(Theme::border_strong());

        const QString accent_color =
            css_color(Theme::accent());

        const QString selection_color =
            css_color(Theme::selection());

        return QStringLiteral(R"(
        QWidget {
            color: %1;
            background-color: %2;
            selection-background-color: %11;
            selection-color: %4;
        }

        QMainWindow {
            background-color: %2;
        }

        QFrame {
            color: %4;
        }

        QLabel {
            color: %4;
            background: transparent;
        }

        QToolTip {
            color: %4;
            background-color: %3;
            border: 1px solid %7;
            padding: 6px 9px;
        }

        QLineEdit,
        QTextEdit,
        QPlainTextEdit {
            color: %4;
            background-color: %3;
            border: 1px solid %7;
            border-radius: 8px;
            padding: 7px 9px;
            selection-background-color: %11;
            selection-color: %4;
        }

        QLineEdit:hover,
        QTextEdit:hover,
        QPlainTextEdit:hover {
            border-color: %8;
        }

        QLineEdit:focus,
        QTextEdit:focus,
        QPlainTextEdit:focus {
            border-color: %9;
        }

        QLineEdit:disabled,
        QTextEdit:disabled,
        QPlainTextEdit:disabled {
            color: %6;
            background-color: %5;
            border-color: %7;
        }

        QComboBox {
            color: %4;
            background-color: %3;
            border: 1px solid %7;
            border-radius: 8px;
            padding: 7px 10px;
            min-height: 18px;
        }

        QComboBox:hover {
            border-color: %8;
        }

        QComboBox:focus {
            border-color: %9;
        }

        QComboBox::drop-down {
            border: none;
            width: 28px;
        }

        QComboBox QAbstractItemView {
            color: %4;
            background-color: %3;
            border: 1px solid %7;
            selection-background-color: %11;
            selection-color: %4;
            padding: 4px;
        }

        QSpinBox,
        QDoubleSpinBox {
            color: %4;
            background-color: %3;
            border: 1px solid %7;
            border-radius: 8px;
            padding: 6px 8px;
        }

        QSpinBox:hover,
        QDoubleSpinBox:hover {
            border-color: %8;
        }

        QSpinBox:focus,
        QDoubleSpinBox:focus {
            border-color: %9;
        }

        QPushButton {
            color: %4;
            background-color: %3;
            border: 1px solid %7;
            border-radius: 8px;
            padding: 7px 14px;
            min-height: 18px;
        }

        QPushButton:hover {
            background-color: %5;
            border-color: %8;
        }

        QPushButton:pressed {
            background-color: %5;
            border-color: %9;
        }

        QPushButton:disabled {
            color: %6;
            background-color: %5;
            border-color: %7;
        }

        QCheckBox,
        QRadioButton {
            color: %4;
            background: transparent;
            spacing: 7px;
        }

        QCheckBox:hover,
        QRadioButton:hover {
            color: %9;
        }

        QGroupBox {
            color: %4;
            background-color: %3;
            border: 1px solid %7;
            border-radius: 10px;
            margin-top: 12px;
            padding: 12px;
        }

        QGroupBox::title {
            color: %4;
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            background-color: %3;
        }

        QListWidget,
        QTreeWidget,
        QTableWidget {
            color: %4;
            background-color: %3;
            alternate-background-color: %5;
            border: 1px solid %7;
            border-radius: 8px;
            outline: none;
        }

        QListWidget::item,
        QTreeWidget::item,
        QTableWidget::item {
            padding: 6px;
            border-radius: 5px;
        }

        QListWidget::item:hover,
        QTreeWidget::item:hover,
        QTableWidget::item:hover {
            background-color: %5;
        }

        QListWidget::item:selected,
        QTreeWidget::item:selected,
        QTableWidget::item:selected {
            color: %4;
            background-color: %11;
        }

        QMenu {
            color: %4;
            background-color: %3;
            border: 1px solid %7;
            padding: 5px;
        }

        QMenu::item {
            color: %4;
            padding: 7px 28px 7px 10px;
            border-radius: 6px;
        }

        QMenu::item:hover,
        QMenu::item:selected {
            color: %4;
            background-color: %11;
        }

        QMenu::item:disabled {
            color: %6;
        }

        QMenu::separator {
            height: 1px;
            background-color: %7;
            margin: 5px 8px;
        }

        QMenuBar {
            color: %4;
            background-color: %3;
            border: none;
            padding: 3px 5px;
        }

        QMenuBar::item {
            color: %4;
            background: transparent;
            padding: 6px 9px;
            border-radius: 6px;
        }

        QMenuBar::item:hover,
        QMenuBar::item:selected {
            color: %4;
            background-color: %5;
        }

        QMenuBar::item:pressed {
            background-color: %11;
        }

        QToolBar {
            color: %4;
            background-color: %3;
            border: none;
        }

        QToolBar::separator {
            background-color: %7;
            width: 1px;
            margin: 10px 5px;
        }

        QToolButton {
            color: %4;
            background: transparent;
            border: none;
            border-radius: 7px;
            padding: 5px;
        }

        QToolButton:hover {
            color: %4;
            background-color: %5;
        }

        QToolButton:pressed {
            color: %4;
            background-color: %11;
        }

        QToolButton:checked {
            color: %4;
            background-color: %11;
        }

        QToolButton:disabled {
            color: %6;
        }

        QStatusBar {
            color: %10;
            background-color: %5;
            border-top: 1px solid %7;
        }

        QStatusBar::item {
            border: none;
        }

        QTabWidget::pane {
            background-color: %3;
            border: 1px solid %7;
            border-radius: 8px;
            top: -1px;
        }

        QTabBar::tab {
            color: %10;
            background-color: %5;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 7px 12px;
            margin-right: 2px;
        }

        QTabBar::tab:hover {
            color: %4;
            background-color: %5;
        }

        QTabBar::tab:selected {
            color: %4;
            background-color: %3;
            border-color: %7;
        }

        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 2px;
        }

        QScrollBar::handle:vertical {
            background-color: %7;
            border-radius: 5px;
            min-height: 26px;
        }

        QScrollBar::handle:vertical:hover {
            background-color: %8;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
        }

        QScrollBar:horizontal {
            background: transparent;
            height: 10px;
            margin: 2px;
        }

        QScrollBar::handle:horizontal {
            background-color: %7;
            border-radius: 5px;
            min-width: 26px;
        }

        QScrollBar::handle:horizontal:hover {
            background-color: %8;
        }

        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal,
        QScrollBar::sub-page:horizontal {
            background: transparent;
            border: none;
        }

        QProgressBar {
            color: %4;
            background-color: %5;
            border: none;
            border-radius: 5px;
            text-align: center;
        }

        QProgressBar::chunk {
            background-color: %9;
            border-radius: 5px;
        }

        QSlider::groove:horizontal {
            height: 4px;
            background-color: %7;
            border-radius: 2px;
        }

        QSlider::handle:horizontal {
            width: 14px;
            height: 14px;
            margin: -5px 0;
            background-color: %9;
            border-radius: 7px;
        }

        QSlider::handle:horizontal:hover {
            background-color: %8;
        }

        QDockWidget {
            color: %4;
            background-color: %3;
        }

        QDockWidget::title {
            color: %4;
            background-color: %5;
            padding: 7px 9px;
            border-bottom: 1px solid %7;
        }

        QSplitter::handle {
            background-color: %7;
        }

        QSplitter::handle:hover {
            background-color: %8;
        }

        QWidget:focus {
            outline: none;
        }

        QAbstractItemView {
            color: %4;
            background-color: %3;
            selection-background-color: %11;
            selection-color: %4;
        }
    )")
            .arg(
                foreground_color,
                background_color,
                surface_color,
                foreground_color,
                surface_variant_color,
                disabled_color,
                border_color,
                border_strong_color,
                accent_color,
                secondary_color,
                selection_color);
    }

    void Theme::apply_palette()
    {
        if (!qApp)
            return;

        qApp->setPalette(
            Theme::palette());

        qApp->setStyleSheet(
            Theme::application_style());

        const auto widgets =
            QApplication::allWidgets();

        for (QWidget *widget : widgets)
        {
            if (!widget)
                continue;

            widget->update();
            widget->updateGeometry();
        }
    }

    // ============================================================================
    // Product-grade icon tinting
    // ============================================================================

    QIcon Theme::recolor_icon(
        const QIcon &source,
        const QColor &color)
    {
        QIcon result;

        if (source.isNull())
            return result;

        const QColor disabled_color = Theme::foreground_disabled();

        // Use source's native sizes if available (PNGs), otherwise use common UI sizes
        QList<QSize> sizes = source.availableSizes(QIcon::Normal, QIcon::Off);
        if (sizes.isEmpty())
        {
            sizes = {
                QSize(16, 16), QSize(20, 20), QSize(24, 24),
                QSize(26, 26), QSize(28, 28), QSize(32, 32),
                QSize(48, 48), QSize(52, 52), QSize(56, 56),
                QSize(64, 64)};
        }

        for (const QSize &size : sizes)
        {
            QPixmap normal = source.pixmap(size, QIcon::Normal, QIcon::Off);
            if (!normal.isNull())
                result.addPixmap(tint_pixmap(normal, color), QIcon::Normal, QIcon::Off);

            QPixmap normal_on = source.pixmap(size, QIcon::Normal, QIcon::On);
            if (!normal_on.isNull())
                result.addPixmap(tint_pixmap(normal_on, color), QIcon::Normal, QIcon::On);

            QPixmap active = source.pixmap(size, QIcon::Active, QIcon::Off);
            if (!active.isNull())
                result.addPixmap(tint_pixmap(active, color), QIcon::Active, QIcon::Off);

            QPixmap selected = source.pixmap(size, QIcon::Selected, QIcon::Off);
            if (!selected.isNull())
                result.addPixmap(tint_pixmap(selected, color), QIcon::Selected, QIcon::Off);

            QPixmap disabled = source.pixmap(size, QIcon::Disabled, QIcon::Off);
            if (!disabled.isNull())
                result.addPixmap(tint_pixmap(disabled, disabled_color), QIcon::Disabled, QIcon::Off);
        }

        return result;
    }

    QIcon Theme::icon(const QString &name)
    {
        if (name.isEmpty())
            return {};

        // SVG: render vector directly at exact sizes (crisp, no scaling artifacts)
        if (name.endsWith(".svg", Qt::CaseInsensitive))
        {
            return load_tinted_svg(name, Theme::icon(), Theme::foreground_disabled());
        }

        const QIcon source(name);
        if (source.isNull())
            return {};

        return recolor_icon(source, Theme::icon());
    }

} // namespace oncrypto::ui