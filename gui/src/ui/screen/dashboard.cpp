#include "gui/ui/core/recent_files.hpp"
#include "gui/ui/screen/dashboard.hpp"
#include "gui/ui/screen/layout_utils.hpp"
#include "gui/ui/theme.hpp"

#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <functional>

namespace
{
    // ---------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------
    QString format_bytes(qint64 bytes)
    {
        static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
        double value = static_cast<double>(bytes);
        int unit = 0;
        while (value >= 1024.0 && unit < 4)
        {
            value /= 1024.0;
            ++unit;
        }
        return QStringLiteral("%1 %2")
            .arg(value, 0, 'f', value >= 10.0 || unit == 0 ? 0 : 1)
            .arg(QString::fromLatin1(units[unit]));
    }

    // متن را بر اساس عرض فعلی لیبل با ... کوتاه می‌کند
    class ElideFilter final : public QObject
    {
    public:
        ElideFilter(QLabel *label, QString full)
            : QObject(label), label_(label), full_(std::move(full))
        {
            label_->setWordWrap(false);
            label_->setMinimumWidth(0);
            label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
            label_->setToolTip(full_);
            label_->installEventFilter(this);
            apply();
        }

    protected:
        bool eventFilter(QObject *o, QEvent *e) override
        {
            if (o == label_ && e->type() == QEvent::Resize)
                apply();
            return QObject::eventFilter(o, e);
        }

    private:
        void apply()
        {
            const int w = label_->width();
            label_->setText(w > 0
                                ? label_->fontMetrics().elidedText(full_, Qt::ElideMiddle, w)
                                : full_);
        }

        QLabel *label_;
        QString full_;
    };

    // ---------------------------------------------------------------------
    // ActionCard
    // ---------------------------------------------------------------------
    class ActionCard final : public oncrypto::ui::screen::TouchCard
    {
    public:
        ActionCard(const QString &title, const QString &desc,
                   const QIcon &icon, QWidget *parent)
            : TouchCard(parent)
        {
            setObjectName(QStringLiteral("DashboardActionCard"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

            auto *root = oncrypto::ui::screen::Layout::h_box(this, 14);
            root->setContentsMargins(18, 16, 18, 16);

            icon_frame_ = new QFrame(this);
            icon_frame_->setObjectName(QStringLiteral("DashboardActionIconFrame"));
            auto *icon_layout = oncrypto::ui::screen::Layout::v_box(icon_frame_);
            icon_layout->setAlignment(Qt::AlignCenter);

            icon_label_ = new QLabel(icon_frame_);
            icon_label_->setObjectName(QStringLiteral("DashboardActionIcon"));
            icon_label_->setAlignment(Qt::AlignCenter);
            icon_label_->setPixmap(icon.pixmap(28, 28, QIcon::Normal, QIcon::Off));
            icon_layout->addWidget(icon_label_);

            auto *text = oncrypto::ui::screen::Layout::v_box(nullptr, 3);
            auto *t = oncrypto::ui::screen::Layout::heading(title, this, 15);
            t->setObjectName(QStringLiteral("DashboardActionTitle"));
            text->addWidget(t);
            text->addWidget(oncrypto::ui::screen::Layout::body_text(desc, this));

            auto *arrow = new QLabel(QStringLiteral("›"), this);
            arrow->setObjectName(QStringLiteral("DashboardActionArrow"));
            arrow->setAlignment(Qt::AlignCenter);
            arrow->setFixedWidth(20);
            QFont af = arrow->font();
            af.setPixelSize(26);
            arrow->setFont(af);

            root->addWidget(icon_frame_, 0, Qt::AlignTop);
            root->addLayout(text, 1);
            root->addWidget(arrow, 0, Qt::AlignCenter);
        }

        void set_compact(bool compact)
        {
            const int h = compact ? 96 : 116;
            const int fs = compact ? 48 : 52;
            const int is = compact ? 26 : 28;
            setMinimumHeight(h);
            icon_frame_->setFixedSize(fs, fs);
            icon_label_->setFixedSize(is, is);
        }

    private:
        QFrame *icon_frame_ = nullptr;
        QLabel *icon_label_ = nullptr;
    };

    // ---------------------------------------------------------------------
    // StatCard
    // ---------------------------------------------------------------------
    class StatCard final : public QFrame
    {
    public:
        StatCard(const QString &title, const QString &value, QWidget *parent)
            : QFrame(parent)
        {
            setObjectName(QStringLiteral("DashboardCard"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            auto *l = oncrypto::ui::screen::Layout::v_box(this, 3);
            l->setContentsMargins(18, 16, 18, 16);

            value_ = oncrypto::ui::screen::Layout::heading(value, this, 24);
            value_->setObjectName(QStringLiteral("DashboardValue"));
            l->addWidget(value_);
            l->addWidget(oncrypto::ui::screen::Layout::body_text(title, this));
        }

        void set_value(const QString &v) { value_->setText(v); }

    private:
        QLabel *value_ = nullptr;
    };

    // ---------------------------------------------------------------------
    // RecentActivity
    // ---------------------------------------------------------------------
    class RecentActivityRow final : public QFrame
    {
    public:
        explicit RecentActivityRow(const gui::core::RecentFileEntry &entry, QWidget *parent = nullptr)
            : QFrame(parent)
        {
            setObjectName(QStringLiteral("DashboardActivityRow"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            setStyleSheet(QStringLiteral(
                "QFrame#DashboardActivityRow {"
                "   background: rgba(255,255,255,8);"
                "   border: 1px solid rgba(255,255,255,12);"
                "   border-radius: 10px;"
                "}"
                "QLabel#DashboardActivityTitle { color: palette(window-text); }"
                "QLabel#DashboardActivityMeta { color: palette(mid); }"
                "QLabel#DashboardActivityTime { color: palette(midlight); }"
            ));

            auto *root = new QHBoxLayout(this);
            root->setContentsMargins(14, 10, 14, 10);
            root->setSpacing(12);

            auto *meta = oncrypto::ui::screen::Layout::v_box(nullptr, 2);
            meta->setContentsMargins(0, 0, 0, 0);

            const QString action_text =
                entry.operation == QStringLiteral("encrypt_file") ? tr("Encrypted file") :
                entry.operation == QStringLiteral("decrypt_file") ? tr("Decrypted file") :
                entry.operation == QStringLiteral("encrypt_text") ? tr("Encrypted text") :
                                                                    tr("Decrypted text");

            auto *title = oncrypto::ui::screen::Layout::heading(action_text, this, 13);
            title->setObjectName(QStringLiteral("DashboardActivityTitle"));
            meta->addWidget(title);

            QString file_label = entry.file_path;
            if (file_label.isEmpty())
                file_label = entry.output_path;
            if (file_label.isEmpty())
                file_label = tr("No path recorded");

            const QString display_name = QFileInfo(file_label).fileName();
            const QString final_label = display_name.isEmpty() ? file_label : display_name;

            auto *detail = oncrypto::ui::screen::Layout::body_text(QString(), this);
            detail->setObjectName(QStringLiteral("DashboardActivityMeta"));
            new ElideFilter(detail,
                            QStringLiteral("%1 • %2").arg(final_label, format_bytes(entry.file_size)));
            if (!entry.file_path.isEmpty())
                setToolTip(entry.file_path); // مسیر کامل در tooltip
            meta->addWidget(detail);

            auto *when = oncrypto::ui::screen::Layout::body_text(
                entry.timestamp.toLocalTime().toString(QStringLiteral("d MMM yyyy • hh:mm")),
                this);
            when->setObjectName(QStringLiteral("DashboardActivityTime"));
            when->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            when->setWordWrap(false);

            root->addLayout(meta, 1);
            root->addWidget(when, 0, Qt::AlignRight | Qt::AlignVCenter);
        }
    };

    class RecentActivity final : public QFrame
    {
        Q_OBJECT
    public:
        explicit RecentActivity(QWidget *parent)
            : QFrame(parent)
        {
            setObjectName(QStringLiteral("DashboardActivity"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            root_ = oncrypto::ui::screen::Layout::v_box(this, 8);
            root_->setContentsMargins(20, 16, 20, 16);
            refresh();
        }

        // ارتفاع را خود layout تعیین می‌کند (max-height قبلی ردیف‌ها را له می‌کرد)
        void set_compact(bool compact)
        {
            root_->setSpacing(compact ? 6 : 8);
        }

    public slots:
        void refresh()
        {
            while (QLayoutItem *item = root_->takeAt(0))
            {
                if (QWidget *w = item->widget())
                {
                    w->hide(); // وگرنه تا اجرای deleteLater کنار ردیف جدید دیده می‌شود
                    w->deleteLater();
                }
                delete item;
            }

            const auto entries = gui::core::RecentFiles::instance().list(5);
            if (entries.isEmpty())
            {
                auto *empty = oncrypto::ui::screen::Layout::heading(
                    tr("No activity yet"), this, 14);
                empty->setAlignment(Qt::AlignLeft | Qt::AlignTop);
                root_->addWidget(empty);
                root_->addWidget(oncrypto::ui::screen::Layout::body_text(
                    tr("Your recent encryption activity will appear here."), this));
                return;
            }

            for (const auto &entry : entries)
                root_->addWidget(new RecentActivityRow(entry, this));
        }

    private:
        QVBoxLayout *root_ = nullptr;
    };

    // ---------------------------------------------------------------------
    // DashboardContent
    // ---------------------------------------------------------------------
    class DashboardContent final : public oncrypto::ui::screen::ResponsiveWidget
    {
    public:
        explicit DashboardContent(QWidget *parent = nullptr)
            : ResponsiveWidget(parent)
        {
            setObjectName(QStringLiteral("DashboardContent"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

            root_ = oncrypto::ui::screen::Layout::v_box(this, 0);
            root_->setSpacing(24);

            title_ = oncrypto::ui::screen::Layout::heading(
                tr("Welcome to OnCrypto"), this, 30);
            title_->setObjectName(QStringLiteral("DashboardTitle"));
            subtitle_ = oncrypto::ui::screen::Layout::body_text(
                tr("Secure. Simple. Local."), this);
            root_->addWidget(title_);
            root_->addWidget(subtitle_);

            root_->addWidget(oncrypto::ui::screen::Layout::heading(
                tr("Quick actions"), this, 16));

            actions_ = oncrypto::ui::screen::Layout::h_box(nullptr, 14);
            encrypt_ = new ActionCard(tr("Encrypt"),
                                      tr("Protect a file or data with encryption."),
                                      icon(QStringLiteral("encrypt")), this);
            decrypt_ = new ActionCard(tr("Decrypt"),
                                      tr("Recover your original data."),
                                      icon(QStringLiteral("decrypt")), this);
            actions_->addWidget(encrypt_, 1);
            actions_->addWidget(decrypt_, 1);
            root_->addLayout(actions_);

            root_->addWidget(oncrypto::ui::screen::Layout::heading(
                tr("Overview"), this, 16));

            overview_ = oncrypto::ui::screen::Layout::h_box(nullptr, 14);
            encrypted_ = new StatCard(tr("Encrypted files"),
                                      QString::number(gui::core::RecentFiles::instance().totalEncrypted()),
                                      this);
            decrypted_ = new StatCard(tr("Decrypted files"),
                                      QString::number(gui::core::RecentFiles::instance().totalDecrypted()),
                                      this);
            processed_ = new StatCard(tr("Data processed"),
                                      format_bytes(gui::core::RecentFiles::instance().totalBytesProcessed()),
                                      this);
            overview_->addWidget(encrypted_, 1);
            overview_->addWidget(decrypted_, 1);
            overview_->addWidget(processed_, 1);
            root_->addLayout(overview_);

            root_->addWidget(oncrypto::ui::screen::Layout::heading(
                tr("Recent activity"), this, 16));
            activity_ = new RecentActivity(this);
            root_->addWidget(activity_);
            root_->addSpacing(8);

            // =========================================================
            // NAVIGATION — via callback (set by Dashboard::create)
            // =========================================================
            connect(encrypt_, &ActionCard::tapped, this, [this]
                    {
                if (navigate_callback_) navigate_callback_("encrypt"); });
            connect(decrypt_, &ActionCard::tapped, this, [this]
                    {
                if (navigate_callback_) navigate_callback_("decrypt"); });

            apply_compact();
        }

        void setNavigateCallback(std::function<void(const char *)> callback)
        {
            navigate_callback_ = std::move(callback);
        }

    protected:
        // با هر بار نمایش داشبورد، آمار و لیست تازه می‌شوند
        void showEvent(QShowEvent *e) override
        {
            ResponsiveWidget::showEvent(e);
            auto &rf = gui::core::RecentFiles::instance();
            encrypted_->set_value(QString::number(rf.totalEncrypted()));
            decrypted_->set_value(QString::number(rf.totalDecrypted()));
            processed_->set_value(format_bytes(rf.totalBytesProcessed()));
            activity_->refresh();
        }

        void apply_compact() override
        {
            root_->setContentsMargins(20, 24, 20, 28);
            root_->setSpacing(20);

            QFont f = title_->font();
            f.setPixelSize(oncrypto::ui::screen::Dpi::sp(28));
            title_->setFont(f);

            oncrypto::ui::screen::Layout::clear(actions_);
            actions_->setDirection(QBoxLayout::TopToBottom);
            actions_->setSpacing(14);
            encrypt_->set_compact(true);
            decrypt_->set_compact(true);
            actions_->addWidget(encrypt_);
            actions_->addWidget(decrypt_);

            oncrypto::ui::screen::Layout::clear(overview_);
            overview_->setSpacing(12);
            auto *top = oncrypto::ui::screen::Layout::h_box(nullptr, 12);
            top->addWidget(encrypted_, 1);
            top->addWidget(decrypted_, 1);
            auto *bottom = oncrypto::ui::screen::Layout::h_box(nullptr, 12);
            bottom->addWidget(processed_, 1);
            overview_->setDirection(QBoxLayout::TopToBottom);
            overview_->addLayout(top);
            overview_->addLayout(bottom);

            activity_->set_compact(true);
        }

        void apply_medium() override
        {
            root_->setContentsMargins(24, 24, 24, 28);
            root_->setSpacing(22);

            QFont f = title_->font();
            f.setPixelSize(oncrypto::ui::screen::Dpi::sp(30));
            title_->setFont(f);

            rebuild_horizontal();
            encrypt_->set_compact(false);
            decrypt_->set_compact(false);
            activity_->set_compact(false);
        }

        void apply_expanded() override
        {
            root_->setContentsMargins(32, 28, 32, 32);
            root_->setSpacing(24);

            QFont f = title_->font();
            f.setPixelSize(oncrypto::ui::screen::Dpi::sp(32));
            title_->setFont(f);

            rebuild_horizontal();
            encrypt_->set_compact(false);
            decrypt_->set_compact(false);
            activity_->set_compact(false);
        }

    private:
        void rebuild_horizontal()
        {
            oncrypto::ui::screen::Layout::clear(actions_);
            actions_->setDirection(QBoxLayout::LeftToRight);
            actions_->setSpacing(16);
            actions_->addWidget(encrypt_, 1);
            actions_->addWidget(decrypt_, 1);

            oncrypto::ui::screen::Layout::clear(overview_);
            overview_->setDirection(QBoxLayout::LeftToRight);
            overview_->setSpacing(16);
            overview_->addWidget(encrypted_, 1);
            overview_->addWidget(decrypted_, 1);
            overview_->addWidget(processed_, 1);
        }

        static QIcon icon(const QString &name)
        {
            const QString path = QStringLiteral(":/icons/") + name + QStringLiteral(".svg");
            QIcon ico = oncrypto::ui::Theme::icon(path);
            return ico.isNull() ? oncrypto::ui::Theme::icon(name) : ico;
        }

        QVBoxLayout *root_ = nullptr;
        QLabel *title_ = nullptr;
        QLabel *subtitle_ = nullptr;

        QHBoxLayout *actions_ = nullptr;
        ActionCard *encrypt_ = nullptr;
        ActionCard *decrypt_ = nullptr;

        QHBoxLayout *overview_ = nullptr;
        StatCard *encrypted_ = nullptr;
        StatCard *decrypted_ = nullptr;
        StatCard *processed_ = nullptr;

        RecentActivity *activity_ = nullptr;

        std::function<void(const char *)> navigate_callback_;
    };

} // anonymous namespace

namespace oncrypto::ui::screen
{

    QWidget *Dashboard::create(QWidget *parent, std::function<void(const char *)> nav_callback)
    {
        auto *scroll = new ResponsiveScroll(parent);
        scroll->setObjectName(QStringLiteral("OnCryptoDashboard"));
        scroll->setAlignment(Qt::AlignTop);

        auto *content = new DashboardContent(scroll);
        if (nav_callback)
        {
            content->setNavigateCallback(std::move(nav_callback));
        }

        scroll->setWidget(content);
        scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        return scroll;
    }

} // namespace oncrypto::ui::screen

#include "dashboard.moc"