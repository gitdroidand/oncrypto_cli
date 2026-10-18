#include "gui/ui/menu.hpp"
#include "gui/ui/navhost.hpp"
#include "gui/ui/content.hpp"
#include "gui/ui/theme.hpp"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QObject>
#include <functional>

namespace
{
    void show_quick_action_dialog(
        QWidget *parent,
        const QString &title,
        const QList<QPair<QString, std::function<void()>>> &actions)
    {
        auto *dialog = new QDialog(parent);
        dialog->setWindowTitle(title);
        dialog->setWindowModality(Qt::WindowModal);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->resize(420, 220);

        auto *layout = new QVBoxLayout(dialog);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(10);

        auto *label = new QLabel(QStringLiteral("Choose a quick action:"), dialog);
        label->setStyleSheet(QStringLiteral("font-weight: 600;"));
        layout->addWidget(label);

        for (const auto &action : actions)
        {
            auto *button = new QPushButton(action.first, dialog);
            button->setCursor(Qt::PointingHandCursor);
            QObject::connect(button, &QPushButton::clicked, dialog, [dialog, func = action.second]()
                             {
                                 if (func)
                                     func();
                                 dialog->accept();
                             });
            layout->addWidget(button);
        }

        layout->addStretch();
        dialog->exec();
    }
}

namespace oncrypto::ui
{

    void Menu::openNewQuickAccess(QWidget *parent, QWidget *content)
    {
        show_quick_action_dialog(
            parent,
            QStringLiteral("Quick Access"),
            {
                {QStringLiteral("Encrypt File"), [content]()
                 {
                     if (content)
                         Content::navigate(content, "encrypt");
                 }},
                {QStringLiteral("Decrypt File"), [content]()
                 {
                     if (content)
                         Content::navigate(content, "decrypt");
                 }},
                {QStringLiteral("Encrypt Text"), [content]()
                 {
                     if (content)
                         Content::navigate(content, "encrypt");
                 }},
                {QStringLiteral("Decrypt Text"), [content]()
                 {
                     if (content)
                         Content::navigate(content, "decrypt");
                 }},
            });
    }

    void Menu::openKeyManager(QWidget *parent)
    {
        QDialog dialog(parent);
        dialog.setWindowTitle(QStringLiteral("Key Manager"));
        dialog.setWindowModality(Qt::WindowModal);
        dialog.resize(440, 220);

        auto *layout = new QVBoxLayout(&dialog);
        auto *form = new QFormLayout();
        form->setContentsMargins(0, 0, 0, 0);

        QSettings settings(QStringLiteral("OnCrypto"), QStringLiteral("OnCryptoDesktop"));

        auto *alias = new QLineEdit(settings.value(QStringLiteral("key_manager/alias")).toString(), &dialog);
        alias->setPlaceholderText(QStringLiteral("e.g. personal-workstation"));
        auto *key_value = new QLineEdit(settings.value(QStringLiteral("key_manager/key")).toString(), &dialog);
        key_value->setPlaceholderText(QStringLiteral("Enter key material"));
        key_value->setEchoMode(QLineEdit::Password);

        form->addRow(QStringLiteral("Alias:"), alias);
        form->addRow(QStringLiteral("Key:"), key_value);
        layout->addLayout(form);

        auto *remember = new QCheckBox(QStringLiteral("Remember this key for this app"), &dialog);
        remember->setChecked(settings.value(QStringLiteral("key_manager/remember"), true).toBool());
        layout->addWidget(remember);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);

        QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]()
                         {
                             const QString alias_text = alias->text().trimmed();
                             const QString key_text = key_value->text();

                             if (alias_text.isEmpty() || key_text.isEmpty())
                             {
                                 QMessageBox::warning(&dialog, QStringLiteral("Key Manager"),
                                                      QStringLiteral("Both alias and key value are required."));
                                 return;
                             }

                             settings.setValue(QStringLiteral("key_manager/alias"), alias_text);
                             settings.setValue(QStringLiteral("key_manager/key"), key_text);
                             settings.setValue(QStringLiteral("key_manager/remember"), remember->isChecked());
                             QMessageBox::information(&dialog, QStringLiteral("Key Manager"),
                                                      QStringLiteral("Key saved successfully."));
                             dialog.accept();
                         });

        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        dialog.exec();
    }

    void Menu::openSettings(QWidget *parent)
    {
        QDialog dialog(parent);
        dialog.setWindowTitle(QStringLiteral("Settings"));
        dialog.setWindowModality(Qt::WindowModal);
        dialog.resize(420, 220);

        QSettings settings(QStringLiteral("OnCrypto"), QStringLiteral("OnCryptoDesktop"));

        auto *layout = new QVBoxLayout(&dialog);
        auto *form = new QFormLayout();
        form->setContentsMargins(0, 0, 0, 0);

        auto *theme = new QComboBox(&dialog);
        theme->addItem(QStringLiteral("System"), static_cast<int>(oncrypto::ui::ThemeMode::System));
        theme->addItem(QStringLiteral("Light"), static_cast<int>(oncrypto::ui::ThemeMode::Light));
        theme->addItem(QStringLiteral("Dark"), static_cast<int>(oncrypto::ui::ThemeMode::Dark));
        theme->setCurrentIndex(theme->findData(static_cast<int>(oncrypto::ui::Theme::mode())));

        auto *autosave = new QCheckBox(QStringLiteral("Auto-save recent activity"), &dialog);
        autosave->setChecked(settings.value(QStringLiteral("settings/auto_save_recent"), true).toBool());

        form->addRow(QStringLiteral("Theme:"), theme);
        layout->addLayout(form);
        layout->addWidget(autosave);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);

        QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]()
                         {
                             const auto selected = static_cast<oncrypto::ui::ThemeMode>(theme->currentData().toInt());
                             oncrypto::ui::Theme::set_mode(selected);
                             settings.setValue(QStringLiteral("settings/theme"), static_cast<int>(selected));
                             settings.setValue(QStringLiteral("settings/auto_save_recent"), autosave->isChecked());
                             dialog.accept();
                         });

        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        dialog.exec();
    }

    void Menu::openAbout(QWidget *parent)
    {
        QMessageBox::about(parent,
                           QStringLiteral("About OnCrypto"),
                           QStringLiteral("OnCrypto Desktop\n\nSecure. Simple. Local.\n\nQuick actions and key management are available from the menu."));
    }

    // ---------------------------------------------------------------------
    // Desktop Menu Bar
    // ---------------------------------------------------------------------

    void Menu::setupDesktop(QMainWindow &window)
    {
        auto *menu_bar = window.menuBar();
        menu_bar->setObjectName(QStringLiteral("OnCryptoMenuBar"));

        setupFileMenu(*menu_bar);
        setupEditMenu(*menu_bar);
        setupViewMenu(*menu_bar);
        setupToolsMenu(*menu_bar);
        setupHelpMenu(*menu_bar);
    }

    void Menu::setupFileMenu(QMenuBar &menu_bar)
    {
        auto *file = menu_bar.addMenu(QStringLiteral("&File"));
        file->setObjectName(QStringLiteral("OnCryptoFileMenu"));

        auto *new_action = file->addAction(QStringLiteral("&New"));
        new_action->setObjectName(QStringLiteral("fileNewAction"));
        new_action->setShortcut(QKeySequence::New);

        auto *open_action = file->addAction(QStringLiteral("&Open..."));
        open_action->setObjectName(QStringLiteral("fileOpenAction"));
        open_action->setShortcut(QKeySequence::Open);

        file->addSeparator();

        auto *encrypt_action = file->addAction(QStringLiteral("&Encrypt..."));
        encrypt_action->setObjectName(QStringLiteral("fileEncryptAction"));
        encrypt_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));

        auto *decrypt_action = file->addAction(QStringLiteral("&Decrypt..."));
        decrypt_action->setObjectName(QStringLiteral("fileDecryptAction"));
        decrypt_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

        file->addSeparator();

        auto *quit_action = file->addAction(QStringLiteral("&Quit"));
        quit_action->setObjectName(QStringLiteral("fileQuitAction"));
        quit_action->setShortcut(QKeySequence::Quit);
    }

    void Menu::setupEditMenu(QMenuBar &menu_bar)
    {
        auto *edit = menu_bar.addMenu(QStringLiteral("&Edit"));
        edit->setObjectName(QStringLiteral("OnCryptoEditMenu"));

        auto *undo_action = edit->addAction(QStringLiteral("&Undo"));
        undo_action->setObjectName(QStringLiteral("editUndoAction"));
        undo_action->setShortcut(QKeySequence::Undo);

        auto *redo_action = edit->addAction(QStringLiteral("&Redo"));
        redo_action->setObjectName(QStringLiteral("editRedoAction"));
        redo_action->setShortcut(QKeySequence::Redo);

        edit->addSeparator();

        auto *cut_action = edit->addAction(QStringLiteral("Cu&t"));
        cut_action->setObjectName(QStringLiteral("editCutAction"));
        cut_action->setShortcut(QKeySequence::Cut);

        auto *copy_action = edit->addAction(QStringLiteral("&Copy"));
        copy_action->setObjectName(QStringLiteral("editCopyAction"));
        copy_action->setShortcut(QKeySequence::Copy);

        auto *paste_action = edit->addAction(QStringLiteral("&Paste"));
        paste_action->setObjectName(QStringLiteral("editPasteAction"));
        paste_action->setShortcut(QKeySequence::Paste);

        edit->addSeparator();

        auto *select_all_action = edit->addAction(QStringLiteral("Select &All"));
        select_all_action->setObjectName(QStringLiteral("editSelectAllAction"));
        select_all_action->setShortcut(QKeySequence::SelectAll);
    }

    void Menu::setupViewMenu(QMenuBar &menu_bar)
    {
        auto *view = menu_bar.addMenu(QStringLiteral("&View"));
        view->setObjectName(QStringLiteral("OnCryptoViewMenu"));

        auto *toolbar_action = view->addAction(QStringLiteral("&Toolbar"));
        toolbar_action->setObjectName(QStringLiteral("viewToolbarAction"));
        toolbar_action->setCheckable(true);
        toolbar_action->setChecked(true);

        auto *statusbar_action = view->addAction(QStringLiteral("&Status Bar"));
        statusbar_action->setObjectName(QStringLiteral("viewStatusBarAction"));
        statusbar_action->setCheckable(true);
        statusbar_action->setChecked(true);

        view->addSeparator();

        auto *fullscreen_action = view->addAction(QStringLiteral("&Full Screen"));
        fullscreen_action->setObjectName(QStringLiteral("viewFullScreenAction"));
        fullscreen_action->setShortcut(QKeySequence(Qt::Key_F11));
    }

    void Menu::setupToolsMenu(QMenuBar &menu_bar)
    {
        auto *tools = menu_bar.addMenu(QStringLiteral("&Tools"));
        tools->setObjectName(QStringLiteral("OnCryptoToolsMenu"));

        auto *encrypt_action = tools->addAction(QStringLiteral("&Encrypt"));
        encrypt_action->setObjectName(QStringLiteral("toolsEncryptAction"));
        encrypt_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));

        auto *decrypt_action = tools->addAction(QStringLiteral("&Decrypt"));
        decrypt_action->setObjectName(QStringLiteral("toolsDecryptAction"));
        decrypt_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

        tools->addSeparator();

        auto *key_manager_action = tools->addAction(QStringLiteral("&Key Manager..."));
        key_manager_action->setObjectName(QStringLiteral("toolsKeyManagerAction"));
        key_manager_action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));

        auto *settings_action = tools->addAction(QStringLiteral("&Settings..."));
        settings_action->setObjectName(QStringLiteral("toolsSettingsAction"));
        settings_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    }

    void Menu::setupHelpMenu(QMenuBar &menu_bar)
    {
        auto *help = menu_bar.addMenu(QStringLiteral("&Help"));
        help->setObjectName(QStringLiteral("OnCryptoHelpMenu"));

        auto *documentation_action = help->addAction(QStringLiteral("&Documentation"));
        documentation_action->setObjectName(QStringLiteral("helpDocumentationAction"));

        auto *shortcuts_action = help->addAction(QStringLiteral("&Keyboard Shortcuts"));
        shortcuts_action->setObjectName(QStringLiteral("helpShortcutsAction"));

        help->addSeparator();

        auto *about_action = help->addAction(QStringLiteral("&About OnCrypto"));
        about_action->setObjectName(QStringLiteral("helpAboutAction"));
        about_action->setShortcut(QKeySequence(Qt::Key_F1));
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
                                 { Menu::openSettings(content); });
            }
            else if (name == QStringLiteral("compactAboutAction"))
            {
                QObject::connect(action, &QAction::triggered, content, [content]()
                                 { Menu::openAbout(content); });
            }
        }
    }

} // namespace oncrypto::ui