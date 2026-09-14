#include "gui/gui.hpp"

#include "gui/ui/content.hpp"
#include "gui/ui/drawer.hpp"
#include "gui/ui/menu.hpp"
#include "gui/ui/toolbar.hpp"

#include <functional>
#include <utility>

#include <QAction>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPoint>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QToolBar>
#include <QWidget>

namespace
{

    enum class WindowMode
    {
        Compact,
        Medium,
        Expanded
    };

    constexpr int kCompactBreakpoint = 600;
    constexpr int kExpandedBreakpoint = 960;
    constexpr int kOverlayAnimationDuration = 220;

    WindowMode mode_for_width(
        int width)
    {
        if (width < kCompactBreakpoint)
            return WindowMode::Compact;

        if (width < kExpandedBreakpoint)
            return WindowMode::Medium;

        return WindowMode::Expanded;
    }

    /*
     * ========================================================================
     * Overlay
     * ========================================================================
     */

    class OverlayWidget final : public QWidget
    {
    public:
        using CloseCallback =
            std::function<void()>;

        explicit OverlayWidget(
            QWidget *parent,
            CloseCallback callback)
            : QWidget(parent), close_callback_(std::move(callback))
        {
            setObjectName(
                QStringLiteral(
                    "OnCryptoDrawerOverlay"));

            setFocusPolicy(
                Qt::NoFocus);

            setStyleSheet(
                QStringLiteral(
                    "QWidget#OnCryptoDrawerOverlay {"
                    "background-color: rgba(0, 0, 0, 90);"
                    "}"));

            auto *effect =
                new QGraphicsOpacityEffect(
                    this);

            effect->setOpacity(
                0.0);

            setGraphicsEffect(
                effect);

            hide();
        }

        QGraphicsOpacityEffect *
        opacity_effect() const
        {
            return qobject_cast<
                QGraphicsOpacityEffect *>(
                graphicsEffect());
        }

    protected:
        void mousePressEvent(
            QMouseEvent *event) override
        {
            Q_UNUSED(event);

            if (close_callback_)
                close_callback_();
        }

    private:
        CloseCallback close_callback_;
    };

    /*
     * ========================================================================
     * Main Window
     * ========================================================================
     */

    class MainWindow final : public QMainWindow
    {
    public:
        explicit MainWindow(
            QWidget *parent = nullptr)
            : QMainWindow(parent)
        {
            setup_window();
            setup_menu();
            setup_toolbar();
            setup_shell();
            setup_connections();

            apply_mode(
                mode_for_width(
                    width()),
                false);
        }

        ~MainWindow() override
        {
            stop_overlay_animation();
        }

    protected:
        void resizeEvent(
            QResizeEvent *event) override
        {
            QMainWindow::resizeEvent(
                event);

            const WindowMode new_mode =
                mode_for_width(
                    event->size().width());

            if (new_mode != mode_)
            {

                apply_mode(
                    new_mode,
                    true);
            }

            update_shell_geometry();
        }

    private:
        /*
         * ====================================================================
         * Window
         * ====================================================================
         */

        void setup_window()
        {
            setWindowTitle(
                QStringLiteral(
                    "OnCrypto"));

            resize(
                1100,
                700);

            setMinimumSize(
                360,
                480);
        }

        /*
         * ====================================================================
         * Menu
         * ====================================================================
         */

        void setup_menu()
        {
            oncrypto::ui::Menu::setupDesktop(
                *this);
        }

        /*
         * ====================================================================
         * Toolbar
         * ====================================================================
         */

        void setup_toolbar()
        {
            mobile_toolbar_ =
                oncrypto::ui::Toolbar::createCompact(
                    *this);

            if (mobile_toolbar_)
                mobile_toolbar_->hide();

            compact_menu_ =
                oncrypto::ui::Menu::createCompact(
                    *this);
        }

        /*
         * ====================================================================
         * Shell
         * ====================================================================
         */

        void setup_shell()
        {
            auto *shell =
                new QWidget(
                    this);

            shell->setObjectName(
                QStringLiteral(
                    "OnCryptoShell"));

            setCentralWidget(
                shell);

            /*
             * Content owns NavHost.
             */
            content_ =
                oncrypto::ui::Content::create(
                    shell);

            /*
             * Drawer lives in the same shell as Content.
             */
            drawer_ =
                oncrypto::ui::Drawer::create(
                    shell);

            /*
             * IMPORTANT:
             *
             * Content owns the real NavHost.
             * Drawer must receive that exact NavHost.
             */
            if (drawer_ && content_)
            {

                QWidget *navhost =
                    oncrypto::ui::Content::navHost(
                        content_);

                if (navhost)
                {

                    oncrypto::ui::Drawer::setNavHost(
                        drawer_,
                        navhost);
                }
            }

            /*
             * Overlay must be created after Drawer.
             */
            overlay_ =
                new OverlayWidget(
                    shell,
                    [this]()
                    {
                        close_drawer();
                    });

            update_shell_geometry();
        }

        /*
         * ====================================================================
         * Connections
         * ====================================================================
         */

        void setup_connections()
        {
            /*
             * ----------------------------------------------------------------
             * Drawer navigation
             * ----------------------------------------------------------------
             */

            if (drawer_)
            {

                oncrypto::ui::Drawer::setNavigateCallback(
                    drawer_,
                    [this](const char *route)
                    {
                        if (!route ||
                            !content_)
                        {
                            return;
                        }

                        /*
                         * Navigate through Content.
                         */
                        oncrypto::ui::Content::navigate(
                            content_,
                            route);

                        /*
                         * Overlay drawer closes automatically
                         * on compact / medium layouts.
                         */
                        if (mode_ != WindowMode::Expanded)
                        {

                            set_drawer_open(
                                false,
                                true);
                        }
                    });

                /*
                 * Initial route synchronization.
                 * NavHost will keep Drawer in sync via RouteChangeCallback.
                 */
                oncrypto::ui::Drawer::setCurrentRoute(
                    drawer_,
                    oncrypto::ui::Content::currentRoute(
                        content_));
            }

            /*
             * ----------------------------------------------------------------
             * Mobile toolbar
             * ----------------------------------------------------------------
             */

            if (!mobile_toolbar_)
                return;

            QAction *drawer_action =
                oncrypto::ui::Toolbar::drawerAction(
                    *mobile_toolbar_);

            if (drawer_action)
            {

                connect(
                    drawer_action,
                    &QAction::triggered,
                    this,
                    [this]()
                    {
                        toggle_drawer();
                    });
            }

            QAction *menu_action =
                oncrypto::ui::Toolbar::menuAction(
                    *mobile_toolbar_);

            if (menu_action)
            {

                connect(
                    menu_action,
                    &QAction::triggered,
                    this,
                    [this]()
                    {
                        show_compact_menu();
                    });
            }
        }

        /*
         * ====================================================================
         * Compact menu
         * ====================================================================
         */

        void show_compact_menu()
        {
            if (!compact_menu_ ||
                !mobile_toolbar_)
            {
                return;
            }

            QAction *action =
                oncrypto::ui::Toolbar::menuAction(
                    *mobile_toolbar_);

            if (!action)
                return;

            QWidget *button =
                mobile_toolbar_->widgetForAction(
                    action);

            if (!button)
            {

                compact_menu_->popup(
                    mobile_toolbar_->mapToGlobal(
                        QPoint(
                            mobile_toolbar_->width(),
                            mobile_toolbar_->height())));

                return;
            }

            const QPoint global_pos =
                button->mapToGlobal(
                    QPoint(
                        button->width() -
                            compact_menu_->sizeHint().width(),
                        button->height()));

            compact_menu_->popup(
                global_pos);
        }

        /*
         * ====================================================================
         * Drawer control
         * ====================================================================
         */

        void toggle_drawer()
        {
            if (mode_ == WindowMode::Expanded ||
                !drawer_)
            {
                return;
            }

            const bool open =
                !oncrypto::ui::Drawer::isOpen(
                    drawer_);

            set_drawer_open(
                open,
                true);
        }

        void close_drawer()
        {
            if (mode_ == WindowMode::Expanded)
                return;

            set_drawer_open(
                false,
                true);
        }

        void set_drawer_open(
            bool open,
            bool animated)
        {
            if (!drawer_)
                return;

            oncrypto::ui::Drawer::setOpen(
                drawer_,
                open,
                animated);

            show_overlay(
                open,
                animated);

            update_shell_geometry();
        }

        /*
         * ====================================================================
         * Responsive mode
         * ====================================================================
         */

        void apply_mode(
            WindowMode mode,
            bool animated)
        {
            mode_ = mode;

            switch (mode_)
            {

            case WindowMode::Compact:

                if (menuBar())
                    menuBar()->hide();

                if (mobile_toolbar_)
                    mobile_toolbar_->show();

                apply_compact(
                    animated);

                break;

            case WindowMode::Medium:

                if (menuBar())
                    menuBar()->hide();

                if (mobile_toolbar_)
                    mobile_toolbar_->show();

                apply_medium(
                    animated);

                break;

            case WindowMode::Expanded:

                if (mobile_toolbar_)
                    mobile_toolbar_->hide();

                if (menuBar())
                    menuBar()->show();

                apply_expanded();

                break;
            }

            update_shell_geometry();
        }

        /*
         * --------------------------------------------------------------------
         * Compact
         * --------------------------------------------------------------------
         */

        void apply_compact(
            bool animated)
        {
            if (!drawer_)
                return;

            oncrypto::ui::Drawer::setExpandedMode(
                drawer_,
                false);

            oncrypto::ui::Drawer::setOpen(
                drawer_,
                false,
                animated);

            show_overlay(
                false,
                animated);
        }

        /*
         * --------------------------------------------------------------------
         * Medium
         * --------------------------------------------------------------------
         */

        void apply_medium(
            bool animated)
        {
            if (!drawer_)
                return;

            oncrypto::ui::Drawer::setExpandedMode(
                drawer_,
                false);

            oncrypto::ui::Drawer::setOpen(
                drawer_,
                true,
                animated);

            show_overlay(
                true,
                animated);
        }

        /*
         * --------------------------------------------------------------------
         * Expanded
         * --------------------------------------------------------------------
         */

        void apply_expanded()
        {
            if (!drawer_)
                return;

            oncrypto::ui::Drawer::setExpandedMode(
                drawer_,
                true);

            oncrypto::ui::Drawer::setOpen(
                drawer_,
                true,
                false);

            show_overlay(
                false,
                false);
        }

        /*
         * ====================================================================
         * Overlay animation
         * ====================================================================
         */

        void stop_overlay_animation()
        {
            if (!overlay_animation_)
                return;

            auto *animation =
                overlay_animation_;

            overlay_animation_ =
                nullptr;

            animation->stop();
            animation->deleteLater();
        }

        void show_overlay(
            bool visible,
            bool animated)
        {
            if (!overlay_)
                return;

            auto *effect =
                overlay_->opacity_effect();

            if (!effect)
                return;

            stop_overlay_animation();

            if (visible)
            {

                if (centralWidget())
                {

                    overlay_->setGeometry(
                        centralWidget()->rect());
                }

                overlay_->show();
                overlay_->raise();

                if (drawer_)
                    drawer_->raise();

                if (!animated)
                {

                    effect->setOpacity(
                        1.0);

                    return;
                }

                animate_overlay(
                    effect->opacity(),
                    1.0,
                    false);

                return;
            }

            if (!animated)
            {

                effect->setOpacity(
                    0.0);

                overlay_->hide();

                if (drawer_)
                    drawer_->raise();

                return;
            }

            if (!overlay_->isVisible())
            {

                effect->setOpacity(
                    0.0);

                return;
            }

            animate_overlay(
                effect->opacity(),
                0.0,
                true);
        }

        void animate_overlay(
            qreal start,
            qreal end,
            bool hide_after)
        {
            if (!overlay_)
                return;

            auto *effect =
                overlay_->opacity_effect();

            if (!effect)
                return;

            stop_overlay_animation();

            effect->setOpacity(
                start);

            auto *animation =
                new QPropertyAnimation(
                    effect,
                    "opacity",
                    this);

            overlay_animation_ =
                animation;

            animation->setDuration(
                kOverlayAnimationDuration);

            animation->setStartValue(
                start);

            animation->setEndValue(
                end);

            animation->setEasingCurve(
                QEasingCurve::OutCubic);

            connect(
                animation,
                &QPropertyAnimation::finished,
                this,
                [this, animation, hide_after]()
                {
                    if (overlay_animation_ !=
                        animation)
                    {
                        return;
                    }

                    overlay_animation_ =
                        nullptr;

                    if (hide_after &&
                        overlay_)
                    {

                        if (auto *effect =
                                overlay_->opacity_effect())
                        {

                            effect->setOpacity(
                                0.0);
                        }

                        overlay_->hide();
                    }

                    if (drawer_)
                        drawer_->raise();

                    animation->deleteLater();
                });

            animation->start();
        }

        /*
         * ====================================================================
         * Shell geometry
         * ====================================================================
         */

        void update_shell_geometry()
        {
            if (!centralWidget() ||
                !content_ ||
                !drawer_)
            {
                return;
            }

            auto *shell =
                centralWidget();

            const QRect area =
                shell->rect();

            /*
             * ================================================================
             * Expanded
             * ================================================================
             */

            if (mode_ ==
                WindowMode::Expanded)
            {

                const int drawer_width =
                    oncrypto::ui::Drawer::preferredWidth(
                        drawer_);

                drawer_->setGeometry(
                    area.left(),
                    area.top(),
                    drawer_width,
                    area.height());

                content_->setGeometry(
                    area.left() + drawer_width,
                    area.top(),
                    area.width() - drawer_width,
                    area.height());

                if (overlay_)
                {

                    if (auto *effect =
                            overlay_->opacity_effect())
                    {

                        effect->setOpacity(
                            0.0);
                    }

                    overlay_->hide();
                }

                drawer_->raise();

                return;
            }

            /*
             * ================================================================
             * Compact / Medium
             * ================================================================
             */

            content_->setGeometry(
                area);

            drawer_->setGeometry(
                area.left(),
                area.top(),
                drawer_->width(),
                area.height());

            if (overlay_)
            {

                if (overlay_->isVisible())
                {

                    overlay_->setGeometry(
                        area);

                    overlay_->raise();
                }

                if (oncrypto::ui::Drawer::isOpen(
                        drawer_))
                {

                    drawer_->raise();
                }
            }
        }

    private:
        QWidget *content_ = nullptr;

        QWidget *drawer_ = nullptr;

        QToolBar *mobile_toolbar_ = nullptr;

        QMenu *compact_menu_ = nullptr;

        OverlayWidget *overlay_ = nullptr;

        QPropertyAnimation *
            overlay_animation_ = nullptr;

        WindowMode mode_ =
            WindowMode::Expanded;
    };

} // namespace

int create_oncrypto_gui(
    QApplication &app)
{
    MainWindow window;

    window.show();

    return app.exec();
}