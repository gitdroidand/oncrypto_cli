#include "gui/ui/toolbar.hpp"

#include "gui/ui/theme.hpp"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QSizePolicy>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

namespace oncrypto::ui {

namespace {

constexpr int kToolbarHeight = 56;
constexpr int kSideButtonSize = 48;
constexpr int kIconSize = 24;

QAction* find_action(
    QToolBar& toolbar,
    const char* object_name
)
{
    const auto actions = toolbar.actions();

    for (QAction* action : actions) {
        if (action &&
            action->objectName() ==
                QString::fromLatin1(object_name)) {
            return action;
        }
    }

    return nullptr;
}

} // namespace

QToolBar* Toolbar::createCompact(
    QMainWindow& window
)
{
    auto* toolbar =
        new QToolBar(
            QStringLiteral("Compact Toolbar"),
            &window
        );

    toolbar->setObjectName(
        QStringLiteral(
            "OnCryptoCompactToolbar"
        )
    );

    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setAllowedAreas(
        Qt::TopToolBarArea
    );

    toolbar->setIconSize(
        QSize(
            kIconSize,
            kIconSize
        )
    );

    toolbar->setFixedHeight(
        kToolbarHeight
    );

    /*
     * Drawer action
     */

    auto* drawer_action =
        new QAction(
            Theme::icon(
                QStringLiteral(
                    ":/icons/drawer.svg"
                )
            ),
            QString(),
            toolbar
        );

    drawer_action->setObjectName(
        QStringLiteral(
            "compactDrawerAction"
        )
    );

    drawer_action->setToolTip(
        QStringLiteral(
            "Toggle Navigation"
        )
    );

    drawer_action->setStatusTip(
        QStringLiteral(
            "Toggle Navigation"
        )
    );

    toolbar->addAction(
        drawer_action
    );

    /*
     * Central title
     *
     * A stretchable container guarantees that
     * the title is centered relative to the
     * toolbar rather than merely placed after
     * the drawer button.
     */

    auto* container =
        new QWidget(toolbar);

    container->setObjectName(
        QStringLiteral(
            "OnCryptoCompactToolbarCenter"
        )
    );

    auto* layout =
        new QHBoxLayout(container);

    layout->setContentsMargins(
        0,
        0,
        0,
        0
    );

    layout->setSpacing(0);

    auto* title =
        new QLabel(
            QStringLiteral("OnCrypto"),
            container
        );

    title->setObjectName(
        QStringLiteral(
            "OnCryptoCompactToolbarTitle"
        )
    );

    title->setAlignment(
        Qt::AlignCenter
    );

    title->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
    );

    layout->addWidget(
        title
    );

    container->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred
    );

    toolbar->addWidget(
        container
    );

    /*
     * Menu action
     */

    auto* menu_action =
        new QAction(
            Theme::icon(
                QStringLiteral(
                    ":/icons/menu.svg"
                )
            ),
            QString(),
            toolbar
        );

    menu_action->setObjectName(
        QStringLiteral(
            "compactMenuAction"
        )
    );

    menu_action->setToolTip(
        QStringLiteral(
            "More Options"
        )
    );

    menu_action->setStatusTip(
        QStringLiteral(
            "More Options"
        )
    );

    toolbar->addAction(
        menu_action
    );

    /*
     * Install toolbar.
     */

    window.addToolBar(
        Qt::TopToolBarArea,
        toolbar
    );

    return toolbar;
}

QAction* Toolbar::drawerAction(
    QToolBar& toolbar
)
{
    return find_action(
        toolbar,
        "compactDrawerAction"
    );
}

QAction* Toolbar::menuAction(
    QToolBar& toolbar
)
{
    return find_action(
        toolbar,
        "compactMenuAction"
    );
}

} // namespace oncrypto::ui