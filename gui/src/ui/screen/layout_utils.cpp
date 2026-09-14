#include "gui/ui/screen/layout_utils.hpp"

#include <QApplication>
#include <QFont>
#include <QKeyEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScreen>

namespace oncrypto::ui::screen
{

    LayoutMode layout_mode_for_width(int width) noexcept
    {
        if (width < 600)
            return LayoutMode::Compact;
        if (width < 900)
            return LayoutMode::Medium;
        return LayoutMode::Expanded;
    }

    int Dpi::px(int pt) noexcept
    {
        static const qreal ratio = qApp->primaryScreen()->logicalDotsPerInch() / 96.0;
        return qRound(pt * ratio);
    }

    int Dpi::sp(int value) noexcept
    {
        static const qreal font_scale = qApp->font().pointSizeF() / 10.0;
        return qRound(value * font_scale);
    }

    int Dpi::dp(int value) noexcept
    {
        return px(value);
    }

    ResponsiveWidget::ResponsiveWidget(QWidget *parent)
        : QWidget(parent)
    {
        resize_debounce_ = new QTimer(this);
        resize_debounce_->setSingleShot(true);
        resize_debounce_->setInterval(150);
        connect(resize_debounce_, &QTimer::timeout, this, [this]
                {
            const auto m = layout_mode_for_width(width());
            if (m == current_mode_)
                return;
            current_mode_ = m;
            switch (m) {
            case LayoutMode::Compact:  apply_compact();  break;
            case LayoutMode::Medium:   apply_medium();   break;
            case LayoutMode::Expanded: apply_expanded(); break;
            }
            emit mode_changed(m); });
    }

    void ResponsiveWidget::resizeEvent(QResizeEvent *event)
    {
        QWidget::resizeEvent(event);
        resize_debounce_->start();
    }

    TouchCard::TouchCard(QWidget *parent)
        : QFrame(parent)
    {
        setAttribute(Qt::WA_Hover, true);
        setAttribute(Qt::WA_AcceptTouchEvents, true);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);

        ripple_ = new QWidget(this);
        ripple_->setAttribute(Qt::WA_TransparentForMouseEvents);
        ripple_->hide();

        ripple_anim_ = new QPropertyAnimation(ripple_, "geometry", this);
        ripple_anim_->setDuration(300);
        ripple_anim_->setEasingCurve(QEasingCurve::OutCubic);
    }

    void TouchCard::set_ripple_color(const QColor &color)
    {
        ripple_color_ = color;
    }

    void TouchCard::start_press(const QPoint &pos)
    {
        pressed_ = true;
        press_origin_ = pos;
        update_ripple(pos);
        update();
    }

    void TouchCard::end_press(bool activated)
    {
        pressed_ = false;
        ripple_->hide();
        update();
        if (activated)
            emit tapped();
    }

    void TouchCard::cancel_press()
    {
        pressed_ = false;
        ripple_->hide();
        update();
    }

    void TouchCard::update_ripple(const QPoint &center)
    {
        const int d = qMax(width(), height()) * 2;
        ripple_->setStyleSheet(QStringLiteral(
                                   "background-color: %1; border-radius: %2px;")
                                   .arg(ripple_color_.name(QColor::HexArgb))
                                   .arg(d / 2));
        ripple_->setGeometry(center.x() - d / 4, center.y() - d / 4, d / 2, d / 2);
        ripple_->show();
        ripple_->setGeometry(center.x() - d / 2, center.y() - d / 2, d, d);
    }

    void TouchCard::mousePressEvent(QMouseEvent *e)
    {
        if (e->button() == Qt::LeftButton)
            start_press(e->pos());
        QFrame::mousePressEvent(e);
    }

    void TouchCard::mouseReleaseEvent(QMouseEvent *e)
    {
        if (e->button() != Qt::LeftButton)
            return QFrame::mouseReleaseEvent(e);

        const bool inside = (e->pos() - press_origin_).manhattanLength() <= tap_slop_;
        end_press(inside);
        QFrame::mouseReleaseEvent(e);
    }

    void TouchCard::mouseMoveEvent(QMouseEvent *e)
    {
        if (pressed_ && (e->pos() - press_origin_).manhattanLength() > tap_slop_)
            cancel_press();
        QFrame::mouseMoveEvent(e);
    }

    void TouchCard::enterEvent(QEnterEvent *e)
    {
        hovered_ = true;
        update();
        QFrame::enterEvent(e);
    }

    void TouchCard::leaveEvent(QEvent *e)
    {
        hovered_ = false;
        cancel_press();
        QFrame::leaveEvent(e);
    }

    void TouchCard::keyPressEvent(QKeyEvent *e)
    {
        if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Return)
        {
            pressed_ = true;
            update();
            emit tapped();
        }
        else
        {
            QFrame::keyPressEvent(e);
        }
    }

    void TouchCard::keyReleaseEvent(QKeyEvent *e)
    {
        if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Return)
        {
            pressed_ = false;
            update();
        }
        else
        {
            QFrame::keyReleaseEvent(e);
        }
    }

    bool TouchCard::event(QEvent *event)
    {
        switch (event->type())
        {
        case QEvent::TouchBegin:
        {
            auto *t = static_cast<QTouchEvent *>(event);
            if (!t->points().isEmpty())
                start_press(t->points().first().position().toPoint());
            return true;
        }
        case QEvent::TouchUpdate:
        {
            auto *t = static_cast<QTouchEvent *>(event);
            if (!t->points().isEmpty())
            {
                const QPoint pos = t->points().first().position().toPoint();
                if ((pos - press_origin_).manhattanLength() > tap_slop_)
                    cancel_press();
            }
            return true;
        }
        case QEvent::TouchEnd:
        {
            auto *t = static_cast<QTouchEvent *>(event);
            if (!t->points().isEmpty())
            {
                const QPoint pos = t->points().first().position().toPoint();
                const bool inside = (pos - press_origin_).manhattanLength() <= tap_slop_;
                end_press(inside);
            }
            else
            {
                cancel_press();
            }
            return true;
        }
        case QEvent::TouchCancel:
            cancel_press();
            return true;
        default:
            break;
        }
        return QFrame::event(event);
    }

    void TouchCard::paintEvent(QPaintEvent *e)
    {
        QFrame::paintEvent(e);
        if (pressed_)
        {
            QPainter p(this);
            p.fillRect(rect(), QColor(0, 0, 0, 25));
        }
        else if (hovered_)
        {
            QPainter p(this);
            p.fillRect(rect(), QColor(0, 0, 0, 8));
        }
    }

    ResponsiveScroll::ResponsiveScroll(QWidget *parent)
        : QScrollArea(parent)
    {
        setWidgetResizable(true);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        QScroller::grabGesture(this, QScroller::TouchGesture);
        auto *scroller = QScroller::scroller(this);
        QScrollerProperties p = scroller->scrollerProperties();
        p.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.6);
        p.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
        p.setScrollMetric(QScrollerProperties::FrameRate, QScrollerProperties::Fps60);
        scroller->setScrollerProperties(p);
    }

    void Layout::clear(QBoxLayout *layout) noexcept
    {
        if (!layout)
            return;
        while (QLayoutItem *item = layout->takeAt(0))
            delete item;
    }

    QHBoxLayout *Layout::h_box(QWidget *parent, int spacing)
    {
        auto *l = new QHBoxLayout(parent);
        l->setSpacing(spacing);
        l->setContentsMargins(0, 0, 0, 0);
        return l;
    }

    QVBoxLayout *Layout::v_box(QWidget *parent, int spacing)
    {
        auto *l = new QVBoxLayout(parent);
        l->setSpacing(spacing);
        l->setContentsMargins(0, 0, 0, 0);
        return l;
    }

    QLabel *Layout::heading(const QString &text, QWidget *parent, int sp_size)
    {
        auto *label = new QLabel(text, parent);
        QFont f = label->font();
        f.setPixelSize(Dpi::sp(sp_size));
        f.setWeight(QFont::DemiBold);
        label->setFont(f);
        label->setWordWrap(true);
        return label;
    }

    QLabel *Layout::body_text(const QString &text, QWidget *parent)
    {
        auto *label = new QLabel(text, parent);
        label->setWordWrap(true);
        label->setObjectName(QStringLiteral("DashboardSecondaryText"));
        return label;
    }

} // namespace oncrypto::ui::screen