#ifndef GUI_UI_SCREEN_DECRYPT_HPP
#define GUI_UI_SCREEN_DECRYPT_HPP

#include <QWidget>

namespace oncrypto::ui::screen
{

    class Decrypt
    {
    public:
        static QWidget *create(QWidget *parent = nullptr);
    };

} // namespace oncrypto::ui::screen

#endif