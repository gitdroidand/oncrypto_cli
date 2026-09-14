#include "gui/ui/screen/encrypt.hpp"
#include "gui/ui/screen/layout_utils.hpp"
#include "gui/ui/screen/drop_zone.hpp"
#include "gui/ui/theme.hpp"
#include "gui/ui/core/onc_service.hpp"
#include "gui/ui/core/file_utils.hpp"
#include "gui/ui/core/password_validator.hpp"
#include "gui/ui/core/recent_files.hpp"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
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
    // PasswordStrengthMeter
    // ---------------------------------------------------------------------
    class PasswordStrengthMeter final : public QFrame
    {
    public:
        explicit PasswordStrengthMeter(QWidget *parent = nullptr)
            : QFrame(parent)
        {
            setObjectName(QStringLiteral("PasswordStrengthMeter"));
            setFixedHeight(4);
            setStyleSheet(QStringLiteral("background-color: #E5E7EB; border-radius: 2px;"));

            fill_ = new QWidget(this);
            fill_->setObjectName(QStringLiteral("PasswordStrengthFill"));
            fill_->setGeometry(0, 0, 0, height());
            fill_->setStyleSheet(QStringLiteral("background-color: #9CA3AF; border-radius: 2px;"));

            anim_ = new QPropertyAnimation(fill_, "geometry", this);
            anim_->setDuration(200);
            anim_->setEasingCurve(QEasingCurve::OutCubic);
        }

        void setStrength(gui::core::PasswordStrength strength)
        {
            QString color = gui::core::PasswordValidator::strengthColor(strength.level);
            fill_->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 2px;").arg(color));

            int target_width = qMax(0, qMin(this->width(), (this->width() * strength.score) / 100));
            anim_->setEndValue(QRect(0, 0, target_width, height()));
            anim_->start();
        }

    protected:
        void resizeEvent(QResizeEvent *event) override
        {
            QFrame::resizeEvent(event);
            fill_->setFixedHeight(height());
        }

    private:
        QWidget *fill_ = nullptr;
        QPropertyAnimation *anim_ = nullptr;
    };

    // ---------------------------------------------------------------------
    // EncryptContent
    // ---------------------------------------------------------------------
    class EncryptContent final : public oncrypto::ui::screen::ResponsiveWidget
    {
    public:
        explicit EncryptContent(QWidget *parent = nullptr)
            : ResponsiveWidget(parent)
        {
            setObjectName(QStringLiteral("EncryptContent"));
            setupUi();
            setupConnections();
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
        void setupUi()
        {
            root_ = oncrypto::ui::screen::Layout::v_box(this, 0);
            root_->setSpacing(20);

            title_ = oncrypto::ui::screen::Layout::heading(QObject::tr("Encrypt"), this, 28);
            title_->setObjectName(QStringLiteral("EncryptTitle"));
            root_->addWidget(title_);

            subtitle_ = oncrypto::ui::screen::Layout::body_text(
                QObject::tr("Protect your files and data with strong encryption."), this);
            root_->addWidget(subtitle_);

            root_->addSpacing(8);

            // Mode tabs
            auto *tab_bar = oncrypto::ui::screen::Layout::h_box(nullptr, 0);
            tab_file_ = new QPushButton(QObject::tr("File"), this);
            tab_file_->setObjectName(QStringLiteral("EncryptTabFile"));
            tab_file_->setCheckable(true);
            tab_file_->setChecked(true);
            tab_file_->setCursor(Qt::PointingHandCursor);

            tab_text_ = new QPushButton(QObject::tr("Text"), this);
            tab_text_->setObjectName(QStringLiteral("EncryptTabText"));
            tab_text_->setCheckable(true);
            tab_text_->setCursor(Qt::PointingHandCursor);

            tab_bar->addWidget(tab_file_);
            tab_bar->addWidget(tab_text_);
            root_->addLayout(tab_bar);

            // Stack
            mode_stack_ = new QStackedWidget(this);

            // ---- FILE MODE ----
            auto *file_page = new QWidget(this);
            auto *file_layout = oncrypto::ui::screen::Layout::v_box(file_page, 12);

            drop_zone_ = new oncrypto::ui::screen::EncryptDropZone(this);
            file_layout->addWidget(drop_zone_);

            file_info_card_ = new QFrame(this);
            file_info_card_->setObjectName(QStringLiteral("EncryptCard"));
            file_info_card_->hide();
            auto *file_info_layout = oncrypto::ui::screen::Layout::h_box(file_info_card_, 12);
            file_info_layout->setContentsMargins(16, 12, 16, 12);

            file_name_label_ = new QLabel(this);
            file_name_label_->setObjectName(QStringLiteral("EncryptFileName"));
            file_info_layout->addWidget(file_name_label_, 1);

            file_size_label_ = oncrypto::ui::screen::Layout::body_text("", this);
            file_info_layout->addWidget(file_size_label_);

            auto *clear_file_btn = new QToolButton(this);
            clear_file_btn->setText(QStringLiteral("✕"));
            clear_file_btn->setCursor(Qt::PointingHandCursor);
            file_info_layout->addWidget(clear_file_btn);

            file_layout->addWidget(file_info_card_);

            // Output path
            auto *out_group = new QFrame(this);
            out_group->setObjectName(QStringLiteral("EncryptCard"));
            auto *out_layout = oncrypto::ui::screen::Layout::v_box(out_group, 8);
            out_layout->setContentsMargins(16, 12, 16, 12);

            out_layout->addWidget(oncrypto::ui::screen::Layout::heading(QObject::tr("Output"), this, 14));

            auto *out_row = oncrypto::ui::screen::Layout::h_box(nullptr, 8);
            output_path_edit_ = new QLineEdit(this);
            output_path_edit_->setObjectName(QStringLiteral("EncryptOutputPath"));
            output_path_edit_->setPlaceholderText(QObject::tr("Auto-generated..."));
            output_path_edit_->setReadOnly(true);
            out_row->addWidget(output_path_edit_, 1);

            auto *out_browse = new QPushButton(QObject::tr("Change"), this);
            out_browse->setObjectName(QStringLiteral("EncryptChangeOutput"));
            out_browse->setCursor(Qt::PointingHandCursor);
            out_row->addWidget(out_browse);

            out_layout->addLayout(out_row);
            file_layout->addWidget(out_group);

            mode_stack_->addWidget(file_page);

            // ---- TEXT MODE ----
            auto *text_page = new QWidget(this);
            auto *text_layout = oncrypto::ui::screen::Layout::v_box(text_page, 12);

            auto *text_card = new QFrame(this);
            text_card->setObjectName(QStringLiteral("EncryptCard"));
            auto *text_card_layout = oncrypto::ui::screen::Layout::v_box(text_card, 8);
            text_card_layout->setContentsMargins(16, 12, 16, 12);

            text_card_layout->addWidget(oncrypto::ui::screen::Layout::heading(QObject::tr("Text to Encrypt"), this, 14));

            text_input_ = new QTextEdit(this);
            text_input_->setObjectName(QStringLiteral("EncryptTextInput"));
            text_input_->setPlaceholderText(QObject::tr("Enter or paste text here..."));
            text_input_->setMinimumHeight(120);
            text_card_layout->addWidget(text_input_);

            text_layout->addWidget(text_card);

            auto *text_out_card = new QFrame(this);
            text_out_card->setObjectName(QStringLiteral("EncryptCard"));
            auto *text_out_layout = oncrypto::ui::screen::Layout::v_box(text_out_card, 8);
            text_out_layout->setContentsMargins(16, 12, 16, 12);

            text_out_layout->addWidget(oncrypto::ui::screen::Layout::heading(QObject::tr("Encrypted Result"), this, 14));

            text_output_ = new QTextEdit(this);
            text_output_->setObjectName(QStringLiteral("EncryptTextOutput"));
            text_output_->setPlaceholderText(QObject::tr("Result will appear here..."));
            text_output_->setReadOnly(true);
            text_output_->setMinimumHeight(80);
            text_out_layout->addWidget(text_output_);

            auto *copy_btn = new QPushButton(QObject::tr("Copy to Clipboard"), this);
            copy_btn->setObjectName(QStringLiteral("EncryptCopyButton"));
            copy_btn->setCursor(Qt::PointingHandCursor);
            text_out_layout->addWidget(copy_btn);

            text_layout->addWidget(text_out_card);

            mode_stack_->addWidget(text_page);
            root_->addWidget(mode_stack_);

            // ---- PASSWORD ----
            auto *pass_card = new QFrame(this);
            pass_card->setObjectName(QStringLiteral("EncryptCard"));
            auto *pass_layout = oncrypto::ui::screen::Layout::v_box(pass_card, 10);
            pass_layout->setContentsMargins(16, 16, 16, 16);

            pass_layout->addWidget(oncrypto::ui::screen::Layout::heading(QObject::tr("Password"), this, 14));

            auto *pass_row = oncrypto::ui::screen::Layout::h_box(nullptr, 8);
            password_edit_ = new QLineEdit(this);
            password_edit_->setObjectName(QStringLiteral("EncryptPassword"));
            password_edit_->setPlaceholderText(QObject::tr("Enter a strong password..."));
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
            pass_row->addWidget(toggle_vis);

            pass_layout->addLayout(pass_row);

            strength_meter_ = new PasswordStrengthMeter(this);
            pass_layout->addWidget(strength_meter_);

            strength_label_ = oncrypto::ui::screen::Layout::body_text("", this);
            strength_label_->setObjectName(QStringLiteral("PasswordStrengthLabel"));
            pass_layout->addWidget(strength_label_);

            confirm_edit_ = new QLineEdit(this);
            confirm_edit_->setObjectName(QStringLiteral("EncryptConfirmPassword"));
            confirm_edit_->setPlaceholderText(QObject::tr("Confirm password..."));
            confirm_edit_->setEchoMode(QLineEdit::Password);
            pass_layout->addWidget(confirm_edit_);

            auto *algo_row = oncrypto::ui::screen::Layout::h_box(nullptr, 8);
            auto *algo_label = new QLabel(QObject::tr("Algorithm:"), this);
            algo_row->addWidget(algo_label);

            algo_combo_ = new QComboBox(this);
            algo_combo_->addItem(QObject::tr("Auto (Recommended)"));
            algo_combo_->addItem(QStringLiteral("AES-256-GCM"));
            algo_combo_->addItem(QStringLiteral("ChaCha20"));
            algo_combo_->addItem(QStringLiteral("XChaCha20"));
            algo_row->addWidget(algo_combo_, 1);
            pass_layout->addLayout(algo_row);

            root_->addWidget(pass_card);

            // ---- ACTION BUTTON ----
            encrypt_btn_ = new QPushButton(QObject::tr("Encrypt Now"), this);
            encrypt_btn_->setObjectName(QStringLiteral("EncryptActionButton"));
            encrypt_btn_->setCursor(Qt::PointingHandCursor);
            encrypt_btn_->setMinimumHeight(52);
            encrypt_btn_->setEnabled(false);
            root_->addWidget(encrypt_btn_);

            root_->addStretch(1);

            connect(clear_file_btn, &QToolButton::clicked, this, [this]
                    {
                selected_file_path_.clear();
                file_info_card_->hide();
                updateEncryptButton(); });

            connect(toggle_vis, &QToolButton::toggled, this, [this](bool checked)
                    { password_edit_->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password); });

            connect(out_browse, &QPushButton::clicked, this, &EncryptContent::onChangeOutputPath);

            connect(copy_btn, &QPushButton::clicked, this, [this]
                    { QApplication::clipboard()->setText(text_output_->toPlainText()); });
        }

        void setupConnections()
        {
            connect(tab_file_, &QPushButton::clicked, this, [this]
                    {
                tab_file_->setChecked(true);
                tab_text_->setChecked(false);
                mode_stack_->setCurrentIndex(0);
                confirm_edit_->setVisible(true); });
            connect(tab_text_, &QPushButton::clicked, this, [this]
                    {
                tab_file_->setChecked(false);
                tab_text_->setChecked(true);
                mode_stack_->setCurrentIndex(1);
                confirm_edit_->setVisible(false); });

            drop_zone_->setFileHandler([this](const QString &path)
                                       { onFileDropped(path); });
            drop_zone_->setTextHandler([this](const QString &text)
                                       {
                tab_text_->click();
                text_input_->setPlainText(text); });

            connect(password_edit_, &QLineEdit::textChanged, this, &EncryptContent::onPasswordChanged);
            connect(confirm_edit_, &QLineEdit::textChanged, this, &EncryptContent::updateEncryptButton);
            connect(text_input_, &QTextEdit::textChanged, this, &EncryptContent::updateEncryptButton);
            connect(encrypt_btn_, &QPushButton::clicked, this, &EncryptContent::onEncrypt);
        }

        void onFileDropped(const QString &path)
        {
            selected_file_path_ = path;
            QFileInfo info(path);

            file_name_label_->setText(info.fileName());
            file_size_label_->setText(gui::core::FileUtils::formatBytes(info.size()));

            QString auto_out = gui::core::FileUtils::autoEncryptPath(path);
            auto result = gui::core::FileUtils::resolveOutputPath(auto_out, false);
            output_path_ = result.path;
            output_path_edit_->setText(output_path_);

            file_info_card_->setVisible(true);
            QPropertyAnimation *anim = new QPropertyAnimation(file_info_card_, "maximumHeight", this);
            anim->setStartValue(0);
            anim->setEndValue(file_info_card_->sizeHint().height());
            anim->setDuration(200);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);

            updateEncryptButton();
        }

        void onChangeOutputPath()
        {
            QString path = QFileDialog::getSaveFileName(this, QObject::tr("Save Encrypted File"),
                                                        output_path_,
                                                        QObject::tr("Encrypted files (*.onc);;All files (*)"));
            if (!path.isEmpty())
            {
                output_path_ = path;
                output_path_edit_->setText(path);
            }
        }

        void onPasswordChanged(const QString &password)
        {
            auto strength = gui::core::PasswordValidator::analyze(password);
            strength_meter_->setStrength(strength);
            strength_label_->setText(
                QStringLiteral("%1: %2")
                    .arg(QObject::tr("Strength"))
                    .arg(gui::core::PasswordValidator::strengthLabel(strength.level)));

            QString color = gui::core::PasswordValidator::strengthColor(strength.level);
            strength_label_->setStyleSheet(QStringLiteral("color: %1;").arg(color));

            updateEncryptButton();
        }

        void updateEncryptButton()
        {
            bool can_encrypt = false;

            if (mode_stack_->currentIndex() == 0)
            {
                can_encrypt = !selected_file_path_.isEmpty() &&
                              !password_edit_->text().isEmpty() &&
                              password_edit_->text() == confirm_edit_->text();
            }
            else
            {
                can_encrypt = !text_input_->toPlainText().isEmpty() &&
                              !password_edit_->text().isEmpty();
            }

            encrypt_btn_->setEnabled(can_encrypt);
        }

        void onEncrypt()
        {
            if (mode_stack_->currentIndex() == 0)
            {
                encryptFile();
            }
            else
            {
                encryptText();
            }
        }

        void encryptFile()
        {
            QString password = password_edit_->text();

            QString error;
            if (!gui::core::PasswordValidator::isValid(password, error))
            {
                QMessageBox::critical(this, QObject::tr("Error"), error);
                return;
            }

            auto result = gui::core::FileUtils::resolveOutputPath(output_path_, false);
            if (!result.success)
            {
                QMessageBox::critical(this, QObject::tr("Error"), result.error);
                return;
            }

            if (result.was_renamed)
            {
                int ret = QMessageBox::question(this, QObject::tr("File Exists"),
                                                QObject::tr("File already exists. Save as:\n%1").arg(result.path));
                if (ret != QMessageBox::Yes)
                    return;
                output_path_ = result.path;
            }

            QProgressDialog progress(QObject::tr("Encrypting..."), QObject::tr("Cancel"), 0, 100, this);
            progress.setWindowModality(Qt::WindowModal);
            progress.setMinimumDuration(0);

            gui::core::OncService::ProgressCallback prog_cb = [&progress](const gui::core::OncService::Progress &p)
            {
                progress.setValue(static_cast<int>(p.percentage()));
                QApplication::processEvents();
                return !progress.wasCanceled();
            };

            auto svc = gui::core::OncService();
            auto result_op = svc.stream()
                                 .input(selected_file_path_.toStdString())
                                 .output(output_path_.toStdString())
                                 .password(password.toStdString())
                                 // =============================================================
                                 // حذف شد: متد algorithm در StreamBuilder وجود ندارد
                                 // =============================================================
                                 // .algorithm(algo_combo_->currentText().toStdString())
                                 .progress(prog_cb)
                                 .encrypt();

            progress.setValue(100);

            if (result_op)
            {
                gui::core::RecentFileEntry entry;
                entry.file_path = selected_file_path_;
                entry.output_path = output_path_;
                entry.operation = "encrypt_file";
                entry.algorithm = algo_combo_->currentText();
                entry.file_size = gui::core::FileUtils::fileSize(selected_file_path_);
                entry.timestamp = QDateTime::currentDateTime();
                entry.success = true;
                gui::core::RecentFiles::instance().add(entry);

                // =============================================================
                // حذف شد: SessionStats متدهای مورد نیاز را ندارد
                // =============================================================
                // gui::core::SessionStats::instance().addEncryptedFile(selected_file_path_);

                QMessageBox::information(this, QObject::tr("Success"),
                                         QObject::tr("File encrypted successfully."));
            }
            else
            {
                QMessageBox::critical(this, QObject::tr("Error"),
                                      QString::fromStdString(result_op.message));
            }
        }

        void encryptText()
        {
            QString text = text_input_->toPlainText();
            QString password = password_edit_->text();

            if (text.isEmpty())
            {
                QMessageBox::critical(this, QObject::tr("Error"), QObject::tr("Please enter text to encrypt."));
                return;
            }

            QString error;
            if (!gui::core::PasswordValidator::isValid(password, error))
            {
                QMessageBox::critical(this, QObject::tr("Error"), error);
                return;
            }

            auto svc = gui::core::OncService();

            QByteArray utf8Bytes = text.toUtf8();
            auto data = std::vector<gui::core::OncService::Byte>(
                utf8Bytes.begin(),
                utf8Bytes.end());

            auto result = svc.data()
                              .input(std::move(data))
                              .password(password.toStdString())
                              // =============================================================
                              // حذف شد: متد algorithm در DataBuilder وجود ندارد
                              // =============================================================
                              // .algorithm(algo_combo_->currentText().toStdString())
                              .encrypt();

            if (result)
            {
                QByteArray bytes(reinterpret_cast<const char *>(result.data.data()),
                                 static_cast<int>(result.data.size()));
                QString base64 = bytes.toBase64();
                text_output_->setPlainText(base64);

                gui::core::RecentFileEntry entry;
                entry.operation = "encrypt_text";
                entry.algorithm = algo_combo_->currentText();
                entry.file_size = text.length();
                entry.timestamp = QDateTime::currentDateTime();
                entry.success = true;
                gui::core::RecentFiles::instance().add(entry);

                // =============================================================
                // حذف شد: SessionStats متدهای مورد نیاز را ندارد
                // =============================================================
                // gui::core::SessionStats::instance().addEncryptedText(text.length());

                QMessageBox::information(this, QObject::tr("Success"),
                                         QObject::tr("Text encrypted successfully."));
            }
            else
            {
                QMessageBox::critical(this, QObject::tr("Error"),
                                      QString::fromStdString(result.message));
            }
        }

    private:
        QVBoxLayout *root_ = nullptr;
        QLabel *title_ = nullptr;
        QLabel *subtitle_ = nullptr;

        QPushButton *tab_file_ = nullptr;
        QPushButton *tab_text_ = nullptr;
        QStackedWidget *mode_stack_ = nullptr;

        oncrypto::ui::screen::EncryptDropZone *drop_zone_ = nullptr;
        QFrame *file_info_card_ = nullptr;
        QLabel *file_name_label_ = nullptr;
        QLabel *file_size_label_ = nullptr;
        QLineEdit *output_path_edit_ = nullptr;
        QString selected_file_path_;
        QString output_path_;

        QTextEdit *text_input_ = nullptr;
        QTextEdit *text_output_ = nullptr;

        QLineEdit *password_edit_ = nullptr;
        QLineEdit *confirm_edit_ = nullptr;
        PasswordStrengthMeter *strength_meter_ = nullptr;
        QLabel *strength_label_ = nullptr;
        QComboBox *algo_combo_ = nullptr;
        QPushButton *encrypt_btn_ = nullptr;
    };

} // anonymous namespace

namespace oncrypto::ui::screen
{

    QWidget *Encrypt::create(QWidget *parent)
    {
        auto *scroll = new ResponsiveScroll(parent);
        scroll->setObjectName(QStringLiteral("OnCryptoEncrypt"));
        scroll->setAlignment(Qt::AlignTop);
        scroll->setWidget(new EncryptContent(scroll));
        scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        return scroll;
    }

} // namespace oncrypto::ui::screen