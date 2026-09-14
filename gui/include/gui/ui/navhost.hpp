#ifndef GUI_UI_NAVHOST_HPP
#define GUI_UI_NAVHOST_HPP

#include <QWidget>
#include <cstddef>
#include <functional>

namespace oncrypto::ui
{

    class NavHost
    {
    public:
        struct Destination
        {
            const char *route;
            const char *title;
        };

        using DestinationCallback = std::function<void()>;
        using RouteChangeCallback = std::function<void(const char *route)>;

        static QWidget *create(QWidget *parent = nullptr);

        static void navigate(QWidget *navhost, const char *route);
        static const char *currentRoute(QWidget *navhost);
        static const Destination *destinations(QWidget *navhost, std::size_t *count);
        static void setDestinationCallback(QWidget *navhost, DestinationCallback callback);

        // Called whenever route changes (for Drawer sync)
        static void setRouteChangeCallback(QWidget *navhost, RouteChangeCallback callback);
    };

} // namespace oncrypto::ui

#endif