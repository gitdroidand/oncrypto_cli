#pragma once

#include <QFrame>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QStyle>
#include <QShortcut>
#include <QApplication>
#include <QClipboard>
#include <functional>

#include "gui/ui/theme.hpp"
#include "gui/ui/screen/layout_utils.hpp"

namespace oncrypto::ui::screen
{

    /**
     * @brief کلاس پایه DropZone برای پذیرش فایل و متن
     */
    class BaseDropZone : public QFrame
    {
    public:
        using FileDropHandler = std::function<void(const QString &)>;
        using TextDropHandler = std::function<void(const QString &)>;

        explicit BaseDropZone(const QString &iconName,
                              const QString &labelText,
                              const QString &subText,
                              QWidget *parent = nullptr)
            : QFrame(parent)
        {
            setObjectName(QStringLiteral("DropZone"));
            setAcceptDrops(true);
            setMinimumHeight(120);
            setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

            auto *layout = oncrypto::ui::screen::Layout::v_box(this, 8);
            layout->setAlignment(Qt::AlignCenter);

            // =============================================================
            // آیکون SVG از Theme - دقیقاً مثل dashboard
            // =============================================================
            auto *icon = new QLabel(this);
            icon->setObjectName(QStringLiteral("DropZoneIcon"));
            icon->setAlignment(Qt::AlignCenter);

            // استفاده از متد icon مثل dashboard
            QIcon themeIcon = iconFromTheme(iconName);
            if (!themeIcon.isNull())
            {
                QPixmap pixmap = themeIcon.pixmap(48, 48);
                icon->setPixmap(pixmap);
            }

            layout->addWidget(icon);

            auto *label = new QLabel(labelText, this);
            label->setObjectName(QStringLiteral("DropZoneLabel"));
            label->setAlignment(Qt::AlignCenter);
            layout->addWidget(label);

            auto *sub = oncrypto::ui::screen::Layout::body_text(subText, this);
            sub->setAlignment(Qt::AlignCenter);
            layout->addWidget(sub);

            // Ctrl+V برای پیست کردن متن
            QShortcut *pasteShortcut = new QShortcut(QKeySequence::Paste, this);
            connect(pasteShortcut, &QShortcut::activated, this, [this]()
                    {
            if (text_handler_) {
                QString text = QApplication::clipboard()->text();
                if (!text.isEmpty()) {
                    text_handler_(text);
                }
            } });
        }

        void setFileHandler(FileDropHandler handler) { file_handler_ = std::move(handler); }
        void setTextHandler(TextDropHandler handler) { text_handler_ = std::move(handler); }

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override
        {
            if (event->mimeData()->hasUrls() || event->mimeData()->hasText())
            {
                event->acceptProposedAction();
                setProperty("dragOver", true);
                style()->unpolish(this);
                style()->polish(this);
            }
        }

        void dragLeaveEvent(QDragLeaveEvent *event) override
        {
            QFrame::dragLeaveEvent(event);
            setProperty("dragOver", false);
            style()->unpolish(this);
            style()->polish(this);
        }

        void dropEvent(QDropEvent *event) override
        {
            setProperty("dragOver", false);
            style()->unpolish(this);
            style()->polish(this);

            const QMimeData *mime = event->mimeData();

            if (mime->hasUrls())
            {
                for (const QUrl &url : mime->urls())
                {
                    QString path = url.toLocalFile();
                    if (!path.isEmpty() && file_handler_)
                    {
                        file_handler_(path);
                        break;
                    }
                }
            }
            else if (mime->hasText() && text_handler_)
            {
                text_handler_(mime->text());
            }

            event->acceptProposedAction();
        }

        void mousePressEvent(QMouseEvent *event) override
        {
            QFrame::mousePressEvent(event);

            QString filter;
            if (isEncryptMode())
            {
                filter = QObject::tr("All files (*)");
            }
            else
            {
                filter = QObject::tr("Encrypted files (*.onc);;All files (*)");
            }

            QString path = QFileDialog::getOpenFileName(this,
                                                        isEncryptMode() ? QObject::tr("Select File") : QObject::tr("Select Encrypted File"),
                                                        QString(),
                                                        filter);

            if (!path.isEmpty() && file_handler_)
            {
                file_handler_(path);
            }
        }

        virtual bool isEncryptMode() const = 0;

    private:
        // =============================================================
        // متد کمکی برای بارگذاری آیکون - دقیقاً مثل dashboard
        // =============================================================
        static QIcon iconFromTheme(const QString &name)
        {
            const QString path = QStringLiteral(":/icons/") + name + QStringLiteral(".svg");
            QIcon ico = oncrypto::ui::Theme::icon(path);
            return ico.isNull() ? oncrypto::ui::Theme::icon(name) : ico;
        }

        FileDropHandler file_handler_;
        TextDropHandler text_handler_;
    };

    // ---------------------------------------------------------------------
    // EncryptDropZone - آیکون folder
    // ---------------------------------------------------------------------
    class EncryptDropZone final : public BaseDropZone
    {
    public:
        explicit EncryptDropZone(QWidget *parent = nullptr)
            : BaseDropZone(
                  QStringLiteral("folder"), // آیکون folder
                  QObject::tr("Drop file here or click to browse"),
                  QObject::tr("Supports text and files"),
                  parent)
        {
            setObjectName(QStringLiteral("EncryptDropZone"));
        }

    protected:
        bool isEncryptMode() const override { return true; }
    };

    // ---------------------------------------------------------------------
    // DecryptDropZone - آیکون visible (چشم)
    // ---------------------------------------------------------------------
    class DecryptDropZone final : public BaseDropZone
    {
    public:
        explicit DecryptDropZone(QWidget *parent = nullptr)
            : BaseDropZone(
                  QStringLiteral("visible"), // آیکون visible (چشم)
                  QObject::tr("Drop encrypted file here"),
                  QObject::tr("Supports .onc files and encrypted text"),
                  parent)
        {
            setObjectName(QStringLiteral("DecryptDropZone"));
        }

    protected:
        bool isEncryptMode() const override { return false; }
    };

} // namespace oncrypto::ui::screen