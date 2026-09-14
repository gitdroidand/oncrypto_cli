#pragma once

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScroller>
#include <QTimer>
#include <QTouchEvent>
#include <QVBoxLayout>
#include <QWidget>

namespace oncrypto::ui::screen
{

    enum class LayoutMode
    {
        Compact,
        Medium,
        Expanded
    };

    LayoutMode layout_mode_for_width(int width) noexcept;

    struct Dpi
    {
        static int px(int pt) noexcept;
        static int sp(int value) noexcept;
        static int dp(int value) noexcept;
    };

    class ResponsiveWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit ResponsiveWidget(QWidget *parent = nullptr);
        ~ResponsiveWidget() override = default;

        LayoutMode current_mode() const noexcept { return current_mode_; }

    signals:
        void mode_changed(LayoutMode mode);

    protected:
        virtual void apply_compact() = 0;
        virtual void apply_medium() = 0;
        virtual void apply_expanded() = 0;

        void resizeEvent(QResizeEvent *event) override;

    private:
        LayoutMode current_mode_ = LayoutMode::Compact;
        QTimer *resize_debounce_ = nullptr;
    };

    class TouchCard : public QFrame
    {
        Q_OBJECT

    public:
        explicit TouchCard(QWidget *parent = nullptr);
        ~TouchCard() override = default;

        void set_ripple_color(const QColor &color);
        void set_tap_slop(int px) noexcept { tap_slop_ = px; }

    signals:
        void tapped();

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void enterEvent(QEnterEvent *event) override;
        void leaveEvent(QEvent *event) override;

        void keyPressEvent(QKeyEvent *event) override;
        void keyReleaseEvent(QKeyEvent *event) override;

        bool event(QEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

    private:
        void start_press(const QPoint &pos);
        void end_press(bool activated);
        void cancel_press();
        void update_ripple(const QPoint &center);

        bool pressed_ = false;
        bool hovered_ = false;
        QPoint press_origin_;
        int tap_slop_ = 12;

        QWidget *ripple_ = nullptr;
        QColor ripple_color_ = QColor(255, 255, 255, 40);
        QPropertyAnimation *ripple_anim_ = nullptr;
    };

    class ResponsiveScroll : public QScrollArea
    {
    public:
        explicit ResponsiveScroll(QWidget *parent = nullptr);
    };

    struct Layout
    {
        static void clear(QBoxLayout *layout) noexcept;
        static QHBoxLayout *h_box(QWidget *parent, int spacing = 0);
        static QVBoxLayout *v_box(QWidget *parent, int spacing = 0);
        static QLabel *heading(const QString &text, QWidget *parent, int sp_size);
        static QLabel *body_text(const QString &text, QWidget *parent);
    };

} // namespace oncrypto::ui::screen