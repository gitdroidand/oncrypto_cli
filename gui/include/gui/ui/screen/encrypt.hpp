#ifndef GUI_UI_SCREEN_ENCRYPT_HPP
#define GUI_UI_SCREEN_ENCRYPT_HPP

#include <QWidget>

namespace oncrypto::ui::screen
{

    class Encrypt
    {
    public:
        static QWidget *create(QWidget *parent = nullptr);
    };

} // namespace oncrypto::ui::screen

#endif