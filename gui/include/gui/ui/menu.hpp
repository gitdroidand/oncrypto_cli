#ifndef GUI_UI_MENU_HPP
#define GUI_UI_MENU_HPP

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QWidget>

namespace oncrypto::ui
{

    class Menu
    {
    public:
        static void setupDesktop(QMainWindow &window);
        static QMenu *createCompact(QMainWindow &window);
        static void connectNavigation(QMenu *menu, QWidget *content);

    private:
        static void setupFileMenu(QMenuBar &menu_bar);
        static void setupEditMenu(QMenuBar &menu_bar);
        static void setupViewMenu(QMenuBar &menu_bar);
        static void setupToolsMenu(QMenuBar &menu_bar);
        static void setupHelpMenu(QMenuBar &menu_bar);
    };

} // namespace oncrypto::ui

#endif // GUI_UI_MENU_HPP