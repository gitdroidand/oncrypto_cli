#pragma once

class QWidget;

namespace oncrypto::ui {

class Content final
{
public:
    static QWidget* create(
        QWidget* parent = nullptr
    );

    static void navigate(
        QWidget* content,
        const char* route
    );

    static const char* currentRoute(
        QWidget* content
    );

    /*
     * Returns the NavHost owned by Content.
     *
     * The returned QWidget is owned by Content and must not
     * be deleted by the caller.
     */
    static QWidget* navHost(
        QWidget* content
    );

private:
    Content() = delete;
};

} // namespace oncrypto::ui