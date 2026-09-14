#ifndef GUI_UI_DRAWER_HPP
#define GUI_UI_DRAWER_HPP

#include <QWidget>
#include <functional>

namespace oncrypto::ui
{

    class Drawer
    {
    public:
        using NavigateCallback = std::function<void(const char *route)>;

        static QWidget *create(QWidget *parent = nullptr);
        static void setExpandedMode(QWidget *drawer, bool expanded);
        static void setOpen(QWidget *drawer, bool open, bool animated = true);
        static bool isOpen(QWidget *drawer);
        static int preferredWidth(QWidget *drawer);
        static void setNavigateCallback(QWidget *drawer, NavigateCallback callback);
        static void setCurrentRoute(QWidget *drawer, const char *route);
        static void setNavHost(QWidget *drawer, QWidget *navhost);
    };

} // namespace oncrypto::ui

#endif