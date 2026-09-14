#include "gui/ui/content.hpp"

#include "gui/ui/navhost.hpp"

#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

namespace {

class ContentWidget final : public QWidget
{
public:
    explicit ContentWidget(
        QWidget* parent = nullptr
    )
        : QWidget(parent)
    {
        setObjectName(
            QStringLiteral(
                "OnCryptoContent"
            )
        );

        setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Expanding
        );

        layout_ =
            new QVBoxLayout(
                this
            );

        layout_->setContentsMargins(
            0,
            0,
            0,
            0
        );

        layout_->setSpacing(
            0
        );

        navhost_ =
            oncrypto::ui::NavHost::create(
                this
            );

        layout_->addWidget(
            navhost_
        );
    }

    void navigate(
        const char* route
    )
    {
        if (!navhost_)
            return;

        oncrypto::ui::NavHost::navigate(
            navhost_,
            route
        );
    }

    const char* current_route() const
    {
        if (!navhost_)
            return "";

        return oncrypto::ui::NavHost::currentRoute(
            navhost_
        );
    }

    QWidget* nav_host() const
    {
        return navhost_;
    }

private:
    QVBoxLayout* layout_ = nullptr;

    QWidget* navhost_ = nullptr;
};

} // namespace

namespace oncrypto::ui {

QWidget* Content::create(
    QWidget* parent
)
{
    return new ContentWidget(
        parent
    );
}

void Content::navigate(
    QWidget* content,
    const char* route
)
{
    if (!content)
        return;

    auto* widget =
        dynamic_cast<ContentWidget*>(
            content
        );

    if (!widget)
        return;

    widget->navigate(
        route
    );
}

const char* Content::currentRoute(
    QWidget* content
)
{
    if (!content)
        return "";

    auto* widget =
        dynamic_cast<ContentWidget*>(
            content
        );

    if (!widget)
        return "";

    return widget->current_route();
}

QWidget* Content::navHost(
    QWidget* content
)
{
    if (!content)
        return nullptr;

    auto* widget =
        dynamic_cast<ContentWidget*>(
            content
        );

    if (!widget)
        return nullptr;

    return widget->nav_host();
}

} // namespace oncrypto::ui