#include "gui/ui/navhost.hpp"

#include "gui/ui/screen/dashboard.hpp"
#include "gui/ui/screen/encrypt.hpp"
#include "gui/ui/screen/decrypt.hpp"

#include <QSizePolicy>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace
{

    class NavHostWidget final : public QWidget
    {
    public:
        explicit NavHostWidget(QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setObjectName(QStringLiteral("OnCryptoNavHost"));

            setSizePolicy(
                QSizePolicy::Expanding,
                QSizePolicy::Expanding);

            layout_ = new QVBoxLayout(this);
            layout_->setContentsMargins(0, 0, 0, 0);
            layout_->setSpacing(0);

            stack_ = new QStackedWidget(this);
            stack_->setObjectName(QStringLiteral("OnCryptoNavigationStack"));
            stack_->setSizePolicy(
                QSizePolicy::Expanding,
                QSizePolicy::Expanding);

            layout_->addWidget(stack_);

            /*
             * Navigation destinations are registered here.
             *
             * NavHost is the single source of truth.
             *
             * Drawer must never duplicate this list.
             */
            register_destination("dashboard", "Dashboard");
            register_destination("encrypt", "Encrypt");
            register_destination("decrypt", "Decrypt");

            /*
             * Dashboard is the default route.
             */
            navigate("dashboard");
        }

        void navigate(const char *route)
        {
            if (!route)
                return;

            for (const auto &destination : destinations_)
            {

                if (std::strcmp(
                        destination.route.c_str(),
                        route) != 0)
                {

                    continue;
                }

                show_destination(
                    destination.route.c_str());

                return;
            }

            /*
             * Unknown routes are intentionally ignored.
             */
        }

        const char *current_route() const
        {
            return current_route_.c_str();
        }

        const oncrypto::ui::NavHost::Destination *
        destinations(
            std::size_t *count) const
        {
            if (count)
                *count = destinations_.size();

            return destination_cache_.data();
        }

        void setDestinationCallback(
            oncrypto::ui::NavHost::DestinationCallback callback)
        {
            destination_callback_ =
                std::move(callback);

            notify_destinations_changed();
        }

        void setRouteChangeCallback(
            oncrypto::ui::NavHost::RouteChangeCallback callback)
        {
            route_change_callback_ =
                std::move(callback);
        }

    private:
        struct DestinationEntry
        {
            std::string route;
            std::string title;
        };

        void register_destination(
            const char *route,
            const char *title)
        {
            if (!route || !title)
                return;

            if (*route == '\0')
                return;

            for (const auto &destination : destinations_)
            {

                if (destination.route == route)
                    return;
            }

            destinations_.push_back(
                DestinationEntry{
                    route,
                    title});

            rebuild_destination_cache();

            notify_destinations_changed();
        }

        void rebuild_destination_cache()
        {
            destination_cache_.clear();

            destination_cache_.reserve(
                destinations_.size());

            for (const auto &destination : destinations_)
            {

                destination_cache_.push_back(
                    oncrypto::ui::NavHost::Destination{
                        destination.route.c_str(),
                        destination.title.c_str()});
            }
        }

        void notify_destinations_changed()
        {
            if (destination_callback_)
                destination_callback_();
        }

        void notify_route_changed()
        {
            if (route_change_callback_)
                route_change_callback_(current_route_.c_str());
        }

        void show_destination(
            const char *route)
        {
            if (!route)
                return;

            if (std::strcmp(
                    route,
                    "dashboard") == 0)
            {

                show_dashboard();

                return;
            }

            if (std::strcmp(
                    route,
                    "encrypt") == 0)
            {

                show_encrypt();

                return;
            }

            if (std::strcmp(
                    route,
                    "decrypt") == 0)
            {

                show_decrypt();

                return;
            }
        }

        void show_dashboard()
        {
            if (dashboard_)
            {

                stack_->setCurrentWidget(
                    dashboard_);

                current_route_ =
                    "dashboard";

                notify_route_changed();

                return;
            }

            QWidget *navhost_ptr = this;

            dashboard_ =
                oncrypto::ui::screen::Dashboard::create(
                    stack_,
                    [navhost_ptr](const char *route)
                    {
                        oncrypto::ui::NavHost::navigate(
                            navhost_ptr,
                            route);
                    });

            if (!dashboard_)
                return;

            stack_->addWidget(
                dashboard_);

            stack_->setCurrentWidget(
                dashboard_);

            current_route_ =
                "dashboard";

            notify_route_changed();
        }

        void show_encrypt()
        {
            if (encrypt_)
            {

                stack_->setCurrentWidget(
                    encrypt_);

                current_route_ =
                    "encrypt";

                notify_route_changed();

                return;
            }

            encrypt_ =
                oncrypto::ui::screen::Encrypt::create(
                    stack_);

            if (!encrypt_)
                return;

            stack_->addWidget(
                encrypt_);

            stack_->setCurrentWidget(
                encrypt_);

            current_route_ =
                "encrypt";

            notify_route_changed();
        }

        void show_decrypt()
        {
            if (decrypt_)
            {

                stack_->setCurrentWidget(
                    decrypt_);

                current_route_ =
                    "decrypt";

                notify_route_changed();

                return;
            }

            decrypt_ =
                oncrypto::ui::screen::Decrypt::create(
                    stack_);

            if (!decrypt_)
                return;

            stack_->addWidget(
                decrypt_);

            stack_->setCurrentWidget(
                decrypt_);

            current_route_ =
                "decrypt";

            notify_route_changed();
        }

    private:
        QVBoxLayout *
            layout_ = nullptr;

        QStackedWidget *
            stack_ = nullptr;

        QWidget *
            dashboard_ = nullptr;

        QWidget *
            encrypt_ = nullptr;

        QWidget *
            decrypt_ = nullptr;

        std::string
            current_route_;

        std::vector<DestinationEntry>
            destinations_;

        std::vector<
            oncrypto::ui::NavHost::Destination>
            destination_cache_;

        oncrypto::ui::NavHost::DestinationCallback
            destination_callback_;

        oncrypto::ui::NavHost::RouteChangeCallback
            route_change_callback_;
    };

} // namespace

namespace oncrypto::ui
{

    QWidget *NavHost::create(
        QWidget *parent)
    {
        return new NavHostWidget(
            parent);
    }

    void NavHost::navigate(
        QWidget *navhost,
        const char *route)
    {
        if (!navhost)
            return;

        auto *widget =
            dynamic_cast<NavHostWidget *>(
                navhost);

        if (!widget)
            return;

        widget->navigate(
            route);
    }

    const char *NavHost::currentRoute(
        QWidget *navhost)
    {
        if (!navhost)
            return "";

        auto *widget =
            dynamic_cast<NavHostWidget *>(
                navhost);

        if (!widget)
            return "";

        return widget->current_route();
    }

    const NavHost::Destination *
    NavHost::destinations(
        QWidget *navhost,
        std::size_t *count)
    {
        if (count)
            *count = 0;

        if (!navhost)
            return nullptr;

        auto *widget =
            dynamic_cast<NavHostWidget *>(
                navhost);

        if (!widget)
            return nullptr;

        return widget->destinations(
            count);
    }

    void NavHost::setDestinationCallback(
        QWidget *navhost,
        DestinationCallback callback)
    {
        if (!navhost)
            return;

        auto *widget =
            dynamic_cast<NavHostWidget *>(
                navhost);

        if (!widget)
            return;

        widget->setDestinationCallback(
            std::move(callback));
    }

    void NavHost::setRouteChangeCallback(
        QWidget *navhost,
        RouteChangeCallback callback)
    {
        if (!navhost)
            return;

        auto *widget =
            dynamic_cast<NavHostWidget *>(
                navhost);

        if (!widget)
            return;

        widget->setRouteChangeCallback(
            std::move(callback));
    }

} // namespace oncrypto::ui