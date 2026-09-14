#pragma once

class QMainWindow;
class QToolBar;
class QAction;

namespace oncrypto::ui {

class Toolbar final
{
public:
    static QToolBar* createCompact(
        QMainWindow& window
    );

    static QAction* drawerAction(
        QToolBar& toolbar
    );

    static QAction* menuAction(
        QToolBar& toolbar
    );
};

} // namespace oncrypto::ui