#ifndef GUI_UI_SCREEN_DASHBOARD_HPP
#define GUI_UI_SCREEN_DASHBOARD_HPP

#include <QWidget>
#include <functional>

namespace oncrypto::ui::screen
{

    class Dashboard
    {
    public:
        static QWidget *create(QWidget *parent = nullptr,
                               std::function<void(const char *)> nav_callback = nullptr);
    };

} // namespace oncrypto::ui::screen

#endif