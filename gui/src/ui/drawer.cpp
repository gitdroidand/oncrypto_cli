#include "gui/ui/drawer.hpp"
#include "gui/ui/navhost.hpp"
#include "gui/ui/theme.hpp"

#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

    constexpr int kDrawerWidth = 280;
    constexpr int kAnimationDuration = 220;

    constexpr int kHorizontalMargin = 20;
    constexpr int kTopMargin = 24;
    constexpr int kBottomMargin = 20;

    constexpr int kHeaderHorizontalPadding = 4;

    constexpr int kHeaderTitleSize = 20;
    constexpr int kSubtitleSize = 12;

    constexpr int kNavigationTopSpacing = 22;
    constexpr int kNavigationItemSpacing = 6;

    constexpr int kNavigationItemHeight = 48;

    constexpr int kNavigationItemRadius = 12;
    constexpr int kNavigationItemHorizontalPadding = 16;

    constexpr int kFooterSpacing = 12;

    constexpr const char *kDrawerObjectName =
        "OnCryptoDrawer";

    constexpr const char *kNavigationButtonObjectName =
        "OnCryptoNavigationButton";

    /*
     * ========================================================================
     * Navigation button
     * ========================================================================
     */

    class NavigationButton final : public QPushButton
    {
    public:
        explicit NavigationButton(
            const QString &title,
            QWidget *parent = nullptr)
            : QPushButton(
                  title,
                  parent)
        {
            setObjectName(
                QString::fromLatin1(
                    kNavigationButtonObjectName));

            setCursor(
                Qt::PointingHandCursor);

            setSizePolicy(
                QSizePolicy::Expanding,
                QSizePolicy::Fixed);

            setMinimumHeight(
                kNavigationItemHeight);

            setMaximumHeight(
                kNavigationItemHeight);

            setMinimumWidth(
                0);

            QFont button_font =
                font();

            button_font.setPointSize(
                14);

            button_font.setWeight(
                QFont::Medium);

            setFont(
                button_font);

            setFlat(
                true);

            setFocusPolicy(
                Qt::StrongFocus);

            setStyleSheet(
                QStringLiteral(
                    "QPushButton#OnCryptoNavigationButton {"
                    "    text-align: left;"
                    "    padding-left: 16px;"
                    "    padding-right: 16px;"
                    "    border: none;"
                    "    border-radius: 12px;"
                    "    background: transparent;"
                    "}"
                    "QPushButton#OnCryptoNavigationButton:hover {"
                    "    background: rgba(127, 127, 127, 20);"
                    "}"
                    "QPushButton#OnCryptoNavigationButton:focus-visible {"
                    "    outline: none;"
                    "    border: 1px solid rgba(127, 127, 127, 100);"
                    "}"
                    "QPushButton#OnCryptoNavigationButton:pressed {"
                    "    background: rgba(127, 127, 127, 40);"
                    "    padding-left: 17px;"
                    "    padding-right: 15px;"
                    "}"
                    "QPushButton#OnCryptoNavigationButton[active=\"true\"] {"
                    "    background: rgba(127, 127, 127, 42);"
                    "}"
                    "QPushButton#OnCryptoNavigationButton[active=\"true\"]:hover {"
                    "    background: rgba(127, 127, 127, 52);"
                    "}"
                    "QPushButton#OnCryptoNavigationButton[active=\"true\"]:pressed {"
                    "    background: rgba(127, 127, 127, 62);"
                    "}"));
        }

        void setActive(
            bool active)
        {
            if (
                property("active").toBool() ==
                active)
            {
                return;
            }

            setProperty(
                "active",
                active);

            style()->unpolish(
                this);

            style()->polish(
                this);

            update();
        }
    };

    /*
     * ========================================================================
     * Drawer widget
     * ========================================================================
     */

    class DrawerWidget final : public QWidget
    {
    public:
        explicit DrawerWidget(
            QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setObjectName(
                QString::fromLatin1(
                    kDrawerObjectName));

            setFixedWidth(
                kDrawerWidth);

            setSizePolicy(
                QSizePolicy::Fixed,
                QSizePolicy::Expanding);

            setProperty(
                "drawerOpen",
                true);

            setProperty(
                "drawerExpandedMode",
                false);

            setAutoFillBackground(
                true);

            setBackgroundRole(
                QPalette::Window);

            setForegroundRole(
                QPalette::WindowText);

            setup_ui();
            createShadow();
        }

        ~DrawerWidget() override
        {
            stopAnimation();

            shadow_ = nullptr;
        }

        /*
         * ====================================================================
         * State
         * ====================================================================
         */

        bool isOpen() const
        {
            return property(
                       "drawerOpen")
                .toBool();
        }

        /*
         * ====================================================================
         * NavHost connection
         * ====================================================================
         */

        void setNavHost(
            QWidget *navhost)
        {
            navhost_ =
                navhost;

            refreshDestinations();

            if (!navhost_)
                return;

            oncrypto::ui::NavHost::
                setDestinationCallback(
                    navhost_,
                    [this]()
                    {
                        refreshDestinations();
                    });

            /*
             * Listen for route changes to update active button.
             */
            oncrypto::ui::NavHost::
                setRouteChangeCallback(
                    navhost_,
                    [this](const char *route)
                    {
                        setCurrentRoute(route);
                    });
        }

        /*
         * ====================================================================
         * Navigation callback
         * ====================================================================
         */

        void setNavigateCallback(
            std::function<void(const char *)> callback)
        {
            navigate_callback_ =
                std::move(callback);
        }

        /*
         * ====================================================================
         * Current route
         * ====================================================================
         */

        void setCurrentRoute(
            const char *route)
        {
            current_route_ =
                route
                    ? route
                    : "";

            updateActiveState();
        }

        /*
         * ====================================================================
         * Expanded mode
         * ====================================================================
         */

        void setExpandedMode(
            bool expanded)
        {
            stopAnimation();

            setProperty(
                "drawerExpandedMode",
                expanded);

            if (expanded)
            {

                setProperty(
                    "drawerOpen",
                    true);

                setVisible(
                    true);

                setMinimumWidth(
                    kDrawerWidth);

                setMaximumWidth(
                    kDrawerWidth);

                setFixedWidth(
                    kDrawerWidth);

                disableShadow();

                updateGeometry();
                update();

                return;
            }

            setVisible(
                true);

            setMinimumWidth(
                kDrawerWidth);

            setMaximumWidth(
                kDrawerWidth);

            setFixedWidth(
                kDrawerWidth);

            enableShadow();

            updateGeometry();
            update();
        }

        /*
         * ====================================================================
         * Open / close
         * ====================================================================
         */

        void setOpen(
            bool open,
            bool animated)
        {
            if (
                property(
                    "drawerExpandedMode")
                    .toBool())
            {

                stopAnimation();

                setProperty(
                    "drawerOpen",
                    true);

                setVisible(
                    true);

                setMinimumWidth(
                    kDrawerWidth);

                setMaximumWidth(
                    kDrawerWidth);

                setFixedWidth(
                    kDrawerWidth);

                disableShadow();

                updateGeometry();
                update();

                return;
            }

            if (
                isOpen() == open &&
                !animation_)
            {

                if (open)
                {

                    setVisible(
                        true);

                    enableShadow();
                }
                else
                {

                    disableShadow();

                    setVisible(
                        false);
                }

                return;
            }

            setProperty(
                "drawerOpen",
                open);

            if (!animated)
            {

                if (open)
                {

                    openImmediately();
                }
                else
                {

                    closeImmediately();
                }

                return;
            }

            animate(
                open);
        }

    private:
        void setup_ui()
        {
            layout_ =
                new QVBoxLayout(
                    this);

            layout_->setContentsMargins(
                kHorizontalMargin,
                kTopMargin,
                kHorizontalMargin,
                kBottomMargin);

            layout_->setSpacing(
                0);

            /*
             * ---------------------------------------------------------------
             * Header
             * ---------------------------------------------------------------
             */

            auto *header =
                new QVBoxLayout();

            header->setContentsMargins(
                kHeaderHorizontalPadding,
                0,
                kHeaderHorizontalPadding,
                0);

            header->setSpacing(
                3);

            auto *title =
                new QLabel(
                    QStringLiteral(
                        "OnCrypto"),
                    this);

            title->setObjectName(
                QStringLiteral(
                    "OnCryptoDrawerTitle"));

            QFont title_font =
                title->font();

            title_font.setPointSize(
                kHeaderTitleSize);

            title_font.setWeight(
                QFont::DemiBold);

            title->setFont(
                title_font);

            title->setSizePolicy(
                QSizePolicy::Preferred,
                QSizePolicy::Fixed);

            header->addWidget(
                title);

            auto *subtitle =
                new QLabel(
                    QStringLiteral(
                        "Navigation to OnCrypto parts"),
                    this);

            subtitle->setObjectName(
                QStringLiteral(
                    "OnCryptoDrawerSubtitle"));

            QFont subtitle_font =
                subtitle->font();

            subtitle_font.setPointSize(
                kSubtitleSize);

            subtitle_font.setWeight(
                QFont::Normal);

            subtitle->setFont(
                subtitle_font);

            subtitle->setWordWrap(
                true);

            subtitle->setSizePolicy(
                QSizePolicy::Expanding,
                QSizePolicy::Preferred);

            header->addWidget(
                subtitle);

            layout_->addLayout(
                header);

            layout_->addSpacing(
                kNavigationTopSpacing);

            /*
             * ---------------------------------------------------------------
             * Navigation
             * ---------------------------------------------------------------
             */

            navigation_layout_ =
                new QVBoxLayout();

            navigation_layout_->setContentsMargins(
                0,
                0,
                0,
                0);

            navigation_layout_->setSpacing(
                kNavigationItemSpacing);

            layout_->addLayout(
                navigation_layout_);

            /*
             * ---------------------------------------------------------------
             * Expand remaining vertical space
             * ---------------------------------------------------------------
             */

            layout_->addStretch(
                1);

            /*
             * ---------------------------------------------------------------
             * Footer
             * ---------------------------------------------------------------
             */

            auto *footer =
                new QHBoxLayout();

            footer->setContentsMargins(
                kHeaderHorizontalPadding,
                0,
                kHeaderHorizontalPadding,
                0);

            footer->setSpacing(
                kFooterSpacing);

            auto *footer_label =
                new QLabel(
                    QStringLiteral(
                        "Next-gen Cryptography tool"),
                    this);

            footer_label->setObjectName(
                QStringLiteral(
                    "OnCryptoDrawerFooter"));

            QFont footer_font =
                footer_label->font();

            footer_font.setPointSize(
                11);

            footer_font.setWeight(
                QFont::Normal);

            footer_label->setFont(
                footer_font);

            footer_label->setWordWrap(
                true);

            footer_label->setSizePolicy(
                QSizePolicy::Expanding,
                QSizePolicy::Preferred);

            footer->addWidget(
                footer_label);

            layout_->addLayout(
                footer);
        }

        void refreshDestinations()
        {
            if (!navigation_layout_)
                return;

            while (
                navigation_layout_->count() > 0)
            {

                auto *item =
                    navigation_layout_->takeAt(
                        0);

                if (!item)
                    continue;

                if (auto *widget =
                        item->widget())
                {

                    widget->deleteLater();
                }

                delete item;
            }

            buttons_.clear();

            if (!navhost_)
            {

                updateActiveState();

                return;
            }

            std::size_t count =
                0;

            const auto *destinations =
                oncrypto::ui::NavHost::destinations(
                    navhost_,
                    &count);

            if (
                !destinations ||
                count == 0)
            {

                updateActiveState();

                return;
            }

            buttons_.reserve(
                count);

            for (
                std::size_t i = 0;
                i < count;
                ++i)
            {

                const auto &destination =
                    destinations[i];

                if (
                    !destination.route ||
                    !destination.title)
                {
                    continue;
                }

                auto button =
                    std::make_unique<
                        NavigationButton>(
                        QString::fromUtf8(
                            destination.title),
                        this);

                button->setProperty(
                    "navigationRoute",
                    QString::fromUtf8(
                        destination.route));

                const std::string route =
                    destination.route;

                connect(
                    button.get(),
                    &QPushButton::clicked,
                    this,
                    [this, route]()
                    {
                        if (navigate_callback_)
                        {

                            navigate_callback_(
                                route.c_str());
                        }
                    });

                auto *button_ptr =
                    button.get();

                navigation_layout_->addWidget(
                    button_ptr);

                buttons_.push_back(
                    std::move(button));
            }

            updateActiveState();
        }

        void updateActiveState()
        {
            if (!navhost_)
                return;

            /*
             * NavHost is the authoritative source for the current route.
             */
            const char *route =
                oncrypto::ui::NavHost::currentRoute(
                    navhost_);

            if (route)
            {

                current_route_ =
                    route;
            }

            for (
                const auto &button :
                buttons_)
            {

                if (!button)
                    continue;

                const QVariant route_property =
                    button->property(
                        "navigationRoute");

                if (!route_property.isValid())
                    continue;

                const QString button_route =
                    route_property.toString();

                const QString current_route =
                    QString::fromUtf8(
                        current_route_.c_str());

                button->setActive(
                    button_route ==
                    current_route);
            }
        }

        void createShadow()
        {
            if (shadow_)
                return;

            shadow_ =
                new QGraphicsDropShadowEffect(
                    this);

            shadow_->setBlurRadius(
                28.0);

            shadow_->setOffset(
                6.0,
                0.0);

            updateShadowColor();

            setGraphicsEffect(
                shadow_);

            shadow_->setEnabled(
                true);
        }

        void enableShadow()
        {
            if (!shadow_)
                return;

            updateShadowColor();

            shadow_->setEnabled(
                true);

            update();
        }

        void disableShadow()
        {
            if (!shadow_)
                return;

            shadow_->setEnabled(
                false);

            update();
        }

        void updateShadowColor()
        {
            if (!shadow_)
                return;

            shadow_->setColor(
                QColor(
                    0,
                    0,
                    0,
                    oncrypto::ui::Theme::is_dark()
                        ? 110
                        : 70));
        }

        void openImmediately()
        {
            stopAnimation();

            setProperty(
                "drawerOpen",
                true);

            setVisible(
                true);

            setMinimumWidth(
                kDrawerWidth);

            setMaximumWidth(
                kDrawerWidth);

            setFixedWidth(
                kDrawerWidth);

            enableShadow();

            updateGeometry();
            updateParent();
        }

        void closeImmediately()
        {
            stopAnimation();

            setProperty(
                "drawerOpen",
                false);

            disableShadow();

            setVisible(
                false);

            setMinimumWidth(
                0);

            setMaximumWidth(
                0);

            setFixedWidth(
                0);

            updateGeometry();
            updateParent();
        }

        void animate(
            bool open)
        {
            stopAnimation();

            const int start_width =
                width();

            const int target_width =
                open
                    ? kDrawerWidth
                    : 0;

            if (open)
            {

                setProperty(
                    "drawerOpen",
                    true);

                setVisible(
                    true);

                enableShadow();
            }
            else
            {

                setProperty(
                    "drawerOpen",
                    false);

                disableShadow();
            }

            setMinimumWidth(
                start_width);

            setMaximumWidth(
                start_width);

            animation_ =
                new QPropertyAnimation(
                    this,
                    "maximumWidth",
                    this);

            animation_->setDuration(
                kAnimationDuration);

            animation_->setStartValue(
                start_width);

            animation_->setEndValue(
                target_width);

            animation_->setEasingCurve(
                QEasingCurve::OutCubic);

            connect(
                animation_,
                &QPropertyAnimation::valueChanged,
                this,
                [this](
                    const QVariant &value)
                {
                    const int current_width =
                        value.toInt();

                    setMinimumWidth(
                        current_width);

                    setMaximumWidth(
                        current_width);

                    if (parentWidget())
                        parentWidget()->update();
                });

            connect(
                animation_,
                &QPropertyAnimation::finished,
                this,
                [this, open]()
                {
                    auto *finished =
                        animation_;

                    animation_ =
                        nullptr;

                    if (open)
                    {

                        setProperty(
                            "drawerOpen",
                            true);

                        setVisible(
                            true);

                        setMinimumWidth(
                            kDrawerWidth);

                        setMaximumWidth(
                            kDrawerWidth);

                        setFixedWidth(
                            kDrawerWidth);

                        enableShadow();
                    }
                    else
                    {

                        setProperty(
                            "drawerOpen",
                            false);

                        disableShadow();

                        setVisible(
                            false);

                        setMinimumWidth(
                            0);

                        setMaximumWidth(
                            0);

                        setFixedWidth(
                            0);
                    }

                    updateGeometry();
                    updateParent();

                    if (finished)
                        finished->deleteLater();
                });

            animation_->start();
        }

        void stopAnimation()
        {
            if (!animation_)
                return;

            auto *animation =
                animation_;

            animation_ =
                nullptr;

            animation->stop();
            animation->deleteLater();
        }

        void updateParent()
        {
            if (!parentWidget())
                return;

            parentWidget()->updateGeometry();
            parentWidget()->update();
        }

    private:
        QVBoxLayout *
            layout_ = nullptr;

        QVBoxLayout *
            navigation_layout_ = nullptr;

        QWidget *
            navhost_ = nullptr;

        std::vector<
            std::unique_ptr<
                NavigationButton>>
            buttons_;

        QGraphicsDropShadowEffect *
            shadow_ = nullptr;

        QPropertyAnimation *
            animation_ = nullptr;

        std::function<void(const char *)>
            navigate_callback_;

        std::string
            current_route_;
    };

} // namespace

/*
 * =========================================================================
 * Public Drawer API
 * =========================================================================
 */

namespace oncrypto::ui
{

    QWidget *Drawer::create(
        QWidget *parent)
    {
        return new DrawerWidget(
            parent);
    }

    void Drawer::setExpandedMode(
        QWidget *drawer,
        bool expanded)
    {
        if (
            auto *widget =
                dynamic_cast<DrawerWidget *>(
                    drawer))
        {

            widget->setExpandedMode(
                expanded);
        }
    }

    void Drawer::setOpen(
        QWidget *drawer,
        bool open,
        bool animated)
    {
        if (
            auto *widget =
                dynamic_cast<DrawerWidget *>(
                    drawer))
        {

            widget->setOpen(
                open,
                animated);
        }
    }

    bool Drawer::isOpen(
        QWidget *drawer)
    {
        if (
            auto *widget =
                dynamic_cast<DrawerWidget *>(
                    drawer))
        {

            return widget->isOpen();
        }

        return false;
    }

    int Drawer::preferredWidth(
        QWidget *)
    {
        return kDrawerWidth;
    }

    void Drawer::setNavigateCallback(
        QWidget *drawer,
        NavigateCallback callback)
    {
        if (
            auto *widget =
                dynamic_cast<DrawerWidget *>(
                    drawer))
        {

            widget->setNavigateCallback(
                std::move(callback));
        }
    }

    void Drawer::setCurrentRoute(
        QWidget *drawer,
        const char *route)
    {
        if (
            auto *widget =
                dynamic_cast<DrawerWidget *>(
                    drawer))
        {

            widget->setCurrentRoute(
                route);
        }
    }

    void Drawer::setNavHost(
        QWidget *drawer,
        QWidget *navhost)
    {
        if (
            auto *widget =
                dynamic_cast<DrawerWidget *>(
                    drawer))
        {

            widget->setNavHost(
                navhost);
        }
    }

} // namespace oncrypto::ui