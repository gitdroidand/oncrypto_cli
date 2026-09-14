#include "gui/ui/screen/decrypt.hpp"
#include "gui/ui/screen/layout_utils.hpp"
#include "gui/ui/screen/drop_zone.hpp"
#include "gui/ui/theme.hpp"
#include "gui/ui/core/onc_service.hpp"
#include "gui/ui/core/file_utils.hpp"
#include "gui/ui/core/recent_files.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
    // ---------------------------------------------------------------------
    // DecryptContent
    // ---------------------------------------------------------------------
    class DecryptContent final : public oncrypto::ui::screen::ResponsiveWidget
    {
    public:
        explicit DecryptContent(QWidget *parent = nullptr)
            : ResponsiveWidget(parent)
        {
            setObjectName(QStringLiteral("DecryptContent"));
            setupUi();
            setupConnections();
            apply_compact();
        }

    private:
        void setupUi()
        {
            root_ = oncrypto::ui::screen::Layout::v_box(this, 0);
            root_->setSpacing(20);

            title_ = oncrypto::ui::screen::Layout::heading(QObject::tr("Decrypt"), this, 28);
            title_->setObjectName(QStringLiteral("DecryptTitle"));
            root_->addWidget(title_);

            subtitle_ = oncrypto::ui::screen::Layout::body_text(
                QObject::tr("Recover your original data securely."), this);
            root_->addWidget(subtitle_);

            root_->addSpacing(8);

            // Tabs
            auto *tab_bar = oncrypto::ui::screen::Layout::h_box(nullptr, 0);
            tab_file_ = new QPushButton(QObject::tr("File"), this);
            tab_file_->setObjectName(QStringLiteral("DecryptTabFile"));
            tab_file_->setCheckable(true);
            tab_file_->setChecked(true);
            tab_file_->setCursor(Qt::PointingHandCursor);

            tab_text_ = new QPushButton(QObject::tr("Text"), this);
            tab_text_->setObjectName(QStringLiteral("DecryptTabText"));
            tab_text_->setCheckable(true);
            tab_text_->setCursor(Qt::PointingHandCursor);

            tab_bar->addWidget(tab_file_);
            tab_bar->addWidget(tab_text_);
            root_->addLayout(tab_bar);

            mode_stack_ = new QStackedWidget(this);

            // ---- FILE MODE ----
            auto *file_page = new QWidget(this);
            auto *file_layout = oncrypto::ui::screen::Layout::v_box(file_page, 12);

            drop_zone_ = new oncrypto::ui::screen::DecryptDropZone(this);
            file_layout->addWidget(drop_zone_);

            file_info_card_ = new QFrame(this);
            file_info_card_->setObjectName(QStringLiteral("DecryptCard"));
            file_info_card_->hide();
            auto *file_info_layout = oncrypto::ui::screen::Layout::h_box(file_info_card_, 12);
            file_info_layout->setContentsMargins(16, 12, 16, 12);

            file_name_label_ = new QLabel(this);
            file_name_label_->setObjectName(QStringLiteral("DecryptFileName"));
            file_info_layout->addWidget(file_name_label_, 1);

            file_size_label_ = oncrypto::ui::screen::Layout::body_text("", this);
            file_info_layout->addWidget(file_size_label_);

            auto *clear_btn = new QToolButton(this);
            clear_btn->setText(QStringLiteral("✕"));
            clear_btn->setCursor(Qt::PointingHandCursor);
            connect(clear_btn, &QToolButton::clicked, this, [this]
                    {
                selected_file_.clear();
                file_info_card_->hide();
                updateButton(); });
            file_info_layout->addWidget(clear_btn);

            file_layout->addWidget(file_info_card_);

            // Output
            auto *out_group = new QFrame(this);
            out_group->setObjectName(QStringLiteral("DecryptCard"));
            auto *out_layout = oncrypto::ui::screen::Layout::v_box(out_group, 8);
            out_layout->setContentsMargins(16, 12, 16, 12);

            out_layout->addWidget(oncrypto::ui::screen::Layout::heading(QObject::tr("Output"), this, 14));

            auto *out_row = oncrypto::ui::screen::Layout::h_box(nullptr, 8);
            output_edit_ = new QLineEdit(this);
            output_edit_->setObjectName(QStringLiteral("DecryptOutputPath"));
            output_edit_->setReadOnly(true);
            output_edit_->setPlaceholderText(QObject::tr("Auto-generated..."));
            out_row->addWidget(output_edit_, 1);

            auto *out_change = new QPushButton(QObject::tr("Change"), this);
            out_change->setObjectName(QStringLiteral("DecryptChangeOutput"));
            out_change->setCursor(Qt::PointingHandCursor);
            connect(out_change, &QPushButton::clicked, this, &DecryptContent::onChangeOutput);
            out_row->addWidget(out_change);

            out_layout->addLayout(out_row);
            file_layout->addWidget(out_group);

            mode_stack_->addWidget(file_page);

            // ---- TEXT MODE ----
            auto *text_page = new QWidget(this);
            auto *text_layout = oncrypto::ui::screen::Layout::v_box(text_page, 12);

            auto *in_card = new QFrame(this);
            in_card->setObjectName(QStringLiteral("DecryptCard"));
            auto *in_layout = oncrypto::ui::screen::Layout::v_box(in_card, 8);
            in_layout->setContentsMargins(16, 12, 16, 12);
            in_layout->addWidget(oncrypto::ui::screen::Layout::heading(QObject::tr("Encrypted Text"), this, 14));

            text_input_ = new QTextEdit(this);
            text_input_->setObjectName(QStringLiteral("DecryptTextInput"));
            text_input_->setPlaceholderText(QObject::tr("Paste base64 encrypted text..."));
            text_input_->setMinimumHeight(100);
            in_layout->addWidget(text_input_);
            text_layout->addWidget(in_card);

            auto *out_card = new QFrame(this);
            out_card->setObjectName(QStringLiteral("DecryptCard"));
            auto *out_card_layout = oncrypto::ui::screen::Layout::v_box(out_card, 8);
            out_card_layout->setContentsMargins(16, 12, 16, 12);
            out_card_layout->addWidget(oncrypto::ui::screen::Layout::heading(QObject::tr("Decrypted Result"), this, 14));

            text_output_ = new QTextEdit(this);
            text_output_->setObjectName(QStringLiteral("DecryptTextOutput"));
            text_output_->setReadOnly(true);
            text_output_->setPlaceholderText(QObject::tr("Result will appear here..."));
            text_output_->setMinimumHeight(100);
            out_card_layout->addWidget(text_output_);

            auto *copy_btn = new QPushButton(QObject::tr("Copy to Clipboard"), this);
            copy_btn->setObjectName(QStringLiteral("DecryptCopyButton"));
            copy_btn->setCursor(Qt::PointingHandCursor);
            connect(copy_btn, &QPushButton::clicked, this, [this]
                    { QApplication::clipboard()->setText(text_output_->toPlainText()); });
            out_card_layout->addWidget(copy_btn);

            text_layout->addWidget(out_card);
            mode_stack_->addWidget(text_page);
            root_->addWidget(mode_stack_);

            // ---- PASSWORD ----
            auto *pass_card = new QFrame(this);
            pass_card->setObjectName(QStringLiteral("DecryptCard"));
            auto *pass_layout = oncrypto::ui::screen::Layout::v_box(pass_card, 10);
            pass_layout->setContentsMargins(16, 16, 16, 16);

            pass_layout->addWidget(oncrypto::ui::screen::Layout::heading(QObject::tr("Password"), this, 14));

            auto *pass_row = oncrypto::ui::screen::Layout::h_box(nullptr, 8);
            password_edit_ = new QLineEdit(this);
            password_edit_->setObjectName(QStringLiteral("DecryptPassword"));
            password_edit_->setPlaceholderText(QObject::tr("Enter decryption password..."));
            password_edit_->setEchoMode(QLineEdit::Password);
            pass_row->addWidget(password_edit_, 1);

            auto *toggle_vis = new QToolButton(this);
            QIcon eyeIcon = oncrypto::ui::Theme::icon(QStringLiteral("visible"));
            if (!eyeIcon.isNull())
            {
                toggle_vis->setIcon(eyeIcon);
                toggle_vis->setIconSize(QSize(20, 20));
            }
            else
            {
                toggle_vis->setText(QStringLiteral("👁")); // fallback
            }
            toggle_vis->setCheckable(true);
            connect(toggle_vis, &QToolButton::toggled, this, [this](bool checked)
                    { password_edit_->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password); });
            pass_row->addWidget(toggle_vis);

            pass_layout->addLayout(pass_row);
            root_->addWidget(pass_card);

            // ---- BUTTON ----
            decrypt_btn_ = new QPushButton(QObject::tr("Decrypt Now"), this);
            decrypt_btn_->setObjectName(QStringLiteral("DecryptActionButton"));
            decrypt_btn_->setCursor(Qt::PointingHandCursor);
            decrypt_btn_->setMinimumHeight(52);
            decrypt_btn_->setEnabled(false);
            root_->addWidget(decrypt_btn_);

            root_->addStretch(1);
        }

        void setupConnections()
        {
            connect(tab_file_, &QPushButton::clicked, this, [this]
                    {
                tab_file_->setChecked(true);
                tab_text_->setChecked(false);
                mode_stack_->setCurrentIndex(0); });
            connect(tab_text_, &QPushButton::clicked, this, [this]
                    {
                tab_file_->setChecked(false);
                tab_text_->setChecked(true);
                mode_stack_->setCurrentIndex(1); });

            drop_zone_->setFileHandler([this](const QString &path)
                                       { onFileDropped(path); });
            drop_zone_->setTextHandler([this](const QString &text)
                                       {
                tab_text_->click();
                text_input_->setPlainText(text); });

            connect(password_edit_, &QLineEdit::textChanged, this, &DecryptContent::updateButton);
            connect(text_input_, &QTextEdit::textChanged, this, &DecryptContent::updateButton);
            connect(decrypt_btn_, &QPushButton::clicked, this, &DecryptContent::onDecrypt);
        }

        void onFileDropped(const QString &path)
        {
            selected_file_ = path;
            QFileInfo info(path);

            file_name_label_->setText(info.fileName());
            file_size_label_->setText(gui::core::FileUtils::formatBytes(info.size()));

            QString auto_out = gui::core::FileUtils::autoDecryptPath(path);
            auto result = gui::core::FileUtils::resolveOutputPath(auto_out, false);
            output_path_ = result.path;
            output_edit_->setText(output_path_);

            file_info_card_->setVisible(true);
            QPropertyAnimation *anim = new QPropertyAnimation(file_info_card_, "maximumHeight", this);
            anim->setStartValue(0);
            anim->setEndValue(file_info_card_->sizeHint().height());
            anim->setDuration(200);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);

            updateButton();
        }

        void onChangeOutput()
        {
            QString path = QFileDialog::getSaveFileName(this, QObject::tr("Save Decrypted File"),
                                                        output_path_);
            if (!path.isEmpty())
            {
                output_path_ = path;
                output_edit_->setText(path);
            }
        }

        void updateButton()
        {
            bool ok = !password_edit_->text().isEmpty();
            if (mode_stack_->currentIndex() == 0)
                ok = ok && !selected_file_.isEmpty();
            else
                ok = ok && !text_input_->toPlainText().isEmpty();
            decrypt_btn_->setEnabled(ok);
        }

        void onDecrypt()
        {
            if (mode_stack_->currentIndex() == 0)
                decryptFile();
            else
                decryptText();
        }

        void decryptFile()
        {
            QString error;
            if (!gui::core::FileUtils::validateInputFile(selected_file_, error))
            {
                QMessageBox::critical(this, QObject::tr("Error"), error);
                return;
            }

            auto out_result = gui::core::FileUtils::resolveOutputPath(output_path_, false);
            if (!out_result.success)
            {
                QMessageBox::critical(this, QObject::tr("Error"), out_result.error);
                return;
            }
            if (out_result.was_renamed)
            {
                int ret = QMessageBox::question(this, QObject::tr("File Exists"),
                                                QObject::tr("File already exists. Save as:\n%1").arg(out_result.path));
                if (ret != QMessageBox::Yes)
                    return;
                output_path_ = out_result.path;
            }

            QProgressDialog progress(QObject::tr("Decrypting..."), QObject::tr("Cancel"), 0, 100, this);
            progress.setWindowModality(Qt::WindowModal);
            progress.setMinimumDuration(0);

            auto svc = gui::core::OncService();
            auto result = svc.stream()
                              .input(selected_file_.toStdString())
                              .output(output_path_.toStdString())
                              .password(password_edit_->text().toStdString())
                              .progress([&progress](const gui::core::OncService::Progress &p)
                                        {
                    progress.setValue(static_cast<int>(p.percentage()));
                    QApplication::processEvents();
                    return !progress.wasCanceled(); })
                              .decrypt();

            progress.setValue(100);

            if (result)
            {
                gui::core::RecentFileEntry entry;
                entry.file_path = selected_file_;
                entry.output_path = output_path_;
                entry.operation = "decrypt_file";
                entry.file_size = gui::core::FileUtils::fileSize(selected_file_);
                entry.timestamp = QDateTime::currentDateTime();
                entry.success = true;
                gui::core::RecentFiles::instance().add(entry);

                // =============================================================
                // حذف شد: SessionStats متدهای مورد نیاز را ندارد
                // =============================================================
                // gui::core::SessionStats::instance().addDecryptedFile(selected_file_);

                QMessageBox::information(this, QObject::tr("Success"),
                                         QObject::tr("File decrypted to:\n%1").arg(output_path_));
            }
            else
            {
                QMessageBox::critical(this, QObject::tr("Error"),
                                      QString::fromStdString(result.message));
            }
        }

        void decryptText()
        {
            QString base64_text = text_input_->toPlainText().trimmed();
            if (base64_text.isEmpty())
            {
                QMessageBox::critical(this, QObject::tr("Error"),
                                      QObject::tr("Please enter encrypted text."));
                return;
            }

            QByteArray bytes = QByteArray::fromBase64(base64_text.toUtf8());
            if (bytes.isEmpty())
            {
                QMessageBox::critical(this, QObject::tr("Error"),
                                      QObject::tr("Invalid encrypted text format."));
                return;
            }

            auto data = std::vector<gui::core::OncService::Byte>(bytes.begin(), bytes.end());
            auto svc = gui::core::OncService();
            auto result = svc.data()
                              .input(std::move(data))
                              .password(password_edit_->text().toStdString())
                              .decrypt();

            if (result)
            {
                QString text = QString::fromUtf8(
                    reinterpret_cast<const char *>(result.data.data()),
                    static_cast<int>(result.data.size()));
                text_output_->setPlainText(text);

                gui::core::RecentFileEntry entry;
                entry.operation = "decrypt_text";
                entry.file_size = base64_text.length();
                entry.timestamp = QDateTime::currentDateTime();
                entry.success = true;
                gui::core::RecentFiles::instance().add(entry);

                // =============================================================
                // حذف شد: SessionStats متدهای مورد نیاز را ندارد
                // =============================================================
                // gui::core::SessionStats::instance().addDecryptedText(base64_text.length());

                QMessageBox::information(this, QObject::tr("Success"),
                                         QObject::tr("Text decrypted successfully."));
            }
            else
            {
                QMessageBox::critical(this, QObject::tr("Error"),
                                      QString::fromStdString(result.message));
            }
        }

    protected:
        void apply_compact() override
        {
            root_->setContentsMargins(16, 16, 16, 20);
            root_->setSpacing(16);
            QFont f = title_->font();
            f.setPixelSize(oncrypto::ui::screen::Dpi::sp(24));
            title_->setFont(f);
        }

        void apply_medium() override
        {
            root_->setContentsMargins(24, 24, 24, 28);
            root_->setSpacing(20);
            QFont f = title_->font();
            f.setPixelSize(oncrypto::ui::screen::Dpi::sp(28));
            title_->setFont(f);
        }

        void apply_expanded() override
        {
            root_->setContentsMargins(32, 28, 32, 32);
            root_->setSpacing(24);
            QFont f = title_->font();
            f.setPixelSize(oncrypto::ui::screen::Dpi::sp(32));
            title_->setFont(f);
        }

    private:
        QVBoxLayout *root_ = nullptr;
        QLabel *title_ = nullptr;
        QLabel *subtitle_ = nullptr;

        QPushButton *tab_file_ = nullptr;
        QPushButton *tab_text_ = nullptr;
        QStackedWidget *mode_stack_ = nullptr;

        oncrypto::ui::screen::DecryptDropZone *drop_zone_ = nullptr;
        QFrame *file_info_card_ = nullptr;
        QLabel *file_name_label_ = nullptr;
        QLabel *file_size_label_ = nullptr;
        QLineEdit *output_edit_ = nullptr;
        QString selected_file_;
        QString output_path_;

        QTextEdit *text_input_ = nullptr;
        QTextEdit *text_output_ = nullptr;

        QLineEdit *password_edit_ = nullptr;
        QPushButton *decrypt_btn_ = nullptr;
    };

} // anonymous namespace

namespace oncrypto::ui::screen
{

    QWidget *Decrypt::create(QWidget *parent)
    {
        auto *scroll = new ResponsiveScroll(parent);
        scroll->setObjectName(QStringLiteral("OnCryptoDecrypt"));
        scroll->setAlignment(Qt::AlignTop);
        scroll->setWidget(new DecryptContent(scroll));
        scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        return scroll;
    }

} // namespace oncrypto::ui::screen