#include "gui/ui/menu.hpp"
#include "gui/ui/navhost.hpp"
#include "gui/ui/content.hpp"

#include <QAction>
#include <QKeySequence>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QObject>

namespace oncrypto::ui
{

    // ---------------------------------------------------------------------
    // Desktop Menu Bar
    // ---------------------------------------------------------------------

    void Menu::setupDesktop(QMainWindow &window)
    {
        auto *menu_bar = window.menuBar();

        setupFileMenu(*menu_bar);
        setupEditMenu(*menu_bar);
        setupViewMenu(*menu_bar);
        setupToolsMenu(*menu_bar);
        setupHelpMenu(*menu_bar);
    }

    void Menu::setupFileMenu(QMenuBar &menu_bar)
    {
        auto *file = menu_bar.addMenu(QStringLiteral("&File"));

        auto *new_action = file->addAction(QStringLiteral("&New"));
        new_action->setShortcut(QKeySequence::New);

        auto *open_action = file->addAction(QStringLiteral("&Open..."));
        open_action->setShortcut(QKeySequence::Open);

        file->addSeparator();

        auto *encrypt_action = file->addAction(QStringLiteral("&Encrypt..."));
        encrypt_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));

        auto *decrypt_action = file->addAction(QStringLiteral("&Decrypt..."));
        decrypt_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

        file->addSeparator();

        auto *quit_action = file->addAction(QStringLiteral("&Quit"));
        quit_action->setShortcut(QKeySequence::Quit);
        QObject::connect(quit_action, &QAction::triggered, &menu_bar, []()
                         {
                             // Signals main execution loop termination
                         });
    }

    void Menu::setupEditMenu(QMenuBar &menu_bar)
    {
        auto *edit = menu_bar.addMenu(QStringLiteral("&Edit"));

        auto *undo_action = edit->addAction(QStringLiteral("&Undo"));
        undo_action->setShortcut(QKeySequence::Undo);

        auto *redo_action = edit->addAction(QStringLiteral("&Redo"));
        redo_action->setShortcut(QKeySequence::Redo);

        edit->addSeparator();

        auto *cut_action = edit->addAction(QStringLiteral("Cu&t"));
        cut_action->setShortcut(QKeySequence::Cut);

        auto *copy_action = edit->addAction(QStringLiteral("&Copy"));
        copy_action->setShortcut(QKeySequence::Copy);

        auto *paste_action = edit->addAction(QStringLiteral("&Paste"));
        paste_action->setShortcut(QKeySequence::Paste);

        edit->addSeparator();

        auto *select_all_action = edit->addAction(QStringLiteral("Select &All"));
        select_all_action->setShortcut(QKeySequence::SelectAll);
    }

    void Menu::setupViewMenu(QMenuBar &menu_bar)
    {
        auto *view = menu_bar.addMenu(QStringLiteral("&View"));

        auto *toolbar_action = view->addAction(QStringLiteral("&Toolbar"));
        toolbar_action->setCheckable(true);
        toolbar_action->setChecked(true);

        auto *statusbar_action = view->addAction(QStringLiteral("&Status Bar"));
        statusbar_action->setCheckable(true);
        statusbar_action->setChecked(true);

        view->addSeparator();

        auto *fullscreen_action = view->addAction(QStringLiteral("&Full Screen"));
        fullscreen_action->setShortcut(QKeySequence(Qt::Key_F11));
    }

    void Menu::setupToolsMenu(QMenuBar &menu_bar)
    {
        auto *tools = menu_bar.addMenu(QStringLiteral("&Tools"));

        auto *encrypt_action = tools->addAction(QStringLiteral("&Encrypt"));
        encrypt_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));

        auto *decrypt_action = tools->addAction(QStringLiteral("&Decrypt"));
        decrypt_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

        tools->addSeparator();

        tools->addAction(QStringLiteral("&Key Manager..."));
        tools->addAction(QStringLiteral("&Settings..."));
    }

    void Menu::setupHelpMenu(QMenuBar &menu_bar)
    {
        auto *help = menu_bar.addMenu(QStringLiteral("&Help"));

        help->addAction(QStringLiteral("&Documentation"));
        help->addAction(QStringLiteral("&Keyboard Shortcuts"));

        help->addSeparator();

        help->addAction(QStringLiteral("&About OnCrypto"));
    }

    // ---------------------------------------------------------------------
    // Compact Menu (Mobile/Toolbar)
    // ---------------------------------------------------------------------

    QMenu *Menu::createCompact(QMainWindow &window)
    {
        auto *menu = new QMenu(&window);
        menu->setObjectName(QStringLiteral("OnCryptoCompactMenu"));

        // Encrypt
        auto *encrypt = menu->addAction(QStringLiteral("Encrypt"));
        encrypt->setObjectName(QStringLiteral("compactEncryptAction"));
        encrypt->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));

        // Decrypt
        auto *decrypt = menu->addAction(QStringLiteral("Decrypt"));
        decrypt->setObjectName(QStringLiteral("compactDecryptAction"));
        decrypt->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

        menu->addSeparator();

        // File operations
        auto *new_action = menu->addAction(QStringLiteral("New"));
        new_action->setObjectName(QStringLiteral("compactNewAction"));
        new_action->setShortcut(QKeySequence::New);

        auto *open_action = menu->addAction(QStringLiteral("Open..."));
        open_action->setObjectName(QStringLiteral("compactOpenAction"));
        open_action->setShortcut(QKeySequence::Open);

        auto *save_action = menu->addAction(QStringLiteral("Save"));
        save_action->setObjectName(QStringLiteral("compactSaveAction"));
        save_action->setShortcut(QKeySequence::Save);

        menu->addSeparator();

        // Settings
        auto *settings = menu->addAction(QStringLiteral("Settings..."));
        settings->setObjectName(QStringLiteral("compactSettingsAction"));

        // About
        auto *about = menu->addAction(QStringLiteral("About OnCrypto"));
        about->setObjectName(QStringLiteral("compactAboutAction"));

        return menu;
    }

    void Menu::connectNavigation(QMenu *menu, QWidget *content)
    {
        if (!menu || !content)
            return;

        for (QAction *action : menu->actions())
        {
            if (!action)
                continue;

            const QString name = action->objectName();

            if (name == QStringLiteral("compactEncryptAction"))
            {
                QObject::connect(action, &QAction::triggered, content, [content]()
                                 { Content::navigate(content, "encrypt"); });
            }
            else if (name == QStringLiteral("compactDecryptAction"))
            {
                QObject::connect(action, &QAction::triggered, content, [content]()
                                 { Content::navigate(content, "decrypt"); });
            }
            else if (name == QStringLiteral("compactSettingsAction"))
            {
                QObject::connect(action, &QAction::triggered, content, [content]()
                                 { Content::navigate(content, "settings"); });
            }
            else if (name == QStringLiteral("compactAboutAction"))
            {
                QObject::connect(action, &QAction::triggered, content, [content]()
                                 { Content::navigate(content, "about"); });
            }
        }
    }

} // namespace oncrypto::ui