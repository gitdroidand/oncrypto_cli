#include <QApplication>
#include <QStyleFactory>

#include "gui/gui.hpp"
#include "gui/ui/theme.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    /*
     * Fusion is intentionally used here.
     *
     * Qt's native Windows style does not consistently render widgets
     * from QPalette because it relies on Windows theme assets.
     * Fusion gives our application-controlled palette predictable
     * rendering behavior.
     */
    app.setStyle(
        QStyleFactory::create(
            QStringLiteral("Fusion")
        )
    );

    /*
     * Initialize the OnCrypto theme system.
     *
     * Default:
     *
     *     ThemeMode::System
     *
     * Therefore the application starts using Windows' current
     * Light/Dark preference and keeps listening for changes.
     */
    oncrypto::ui::Theme::initialize();

    return create_oncrypto_gui(app);
}