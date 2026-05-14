#include "MarqueeLabel.h"
#include "ThemeManager.h"
#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QStyleOption>
#include <QStyle>

MarqueeLabel::MarqueeLabel(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
    , m_scrollOffset(0)
    , m_textWidth(0)
{
    setAttribute(Qt::WA_StyledBackground, true);
    m_animation = new QPropertyAnimation(this, "scrollOffset", this);
    m_pauseTimer = new QTimer(this);
    m_pauseTimer->setSingleShot(true);

    connect(m_animation, &QPropertyAnimation::finished, this, &MarqueeLabel::onAnimationFinished);
    connect(m_pauseTimer, &QTimer::timeout, this, &MarqueeLabel::updateAnimation);
    connect(m_animation, &QPropertyAnimation::stateChanged, this, [this](QAbstractAnimation::State, QAbstractAnimation::State) {
        update(); // Force repaint when animation starts or stops to update fade effect
    });

    calculateTextWidth();
}

MarqueeLabel::~MarqueeLabel() = default;

QString MarqueeLabel::text() const {
    return m_text;
}

void MarqueeLabel::setText(const QString &text) {
    if (m_text == text)
        return;

    m_text = text;
    m_animation->stop();
    m_pauseTimer->stop();
    m_scrollOffset = 0;
    calculateTextWidth();
    updateAnimation();
    update();
}

int MarqueeLabel::scrollOffset() const {
    return m_scrollOffset;
}

void MarqueeLabel::setScrollOffset(int offset) {
    if (m_scrollOffset != offset) {
        m_scrollOffset = offset;
        update();
    }
}

void MarqueeLabel::calculateTextWidth() {
    QFontMetrics fm(font());
    m_textWidth = fm.horizontalAdvance(m_text);
}

void MarqueeLabel::updateAnimation() {
    m_animation->stop();
    
    int drawWidth = width() - 20; // 10px indent on both sides
    if (m_textWidth > drawWidth && drawWidth > 0) {
        m_animation->setDuration((m_textWidth + 50) * 40); // Adjust speed
        m_animation->setStartValue(0);
        m_animation->setEndValue(m_textWidth + 50); // Provide some padding at the end
        m_animation->start();
    } else {
        m_scrollOffset = 0;
        update();
    }
}

void MarqueeLabel::onAnimationFinished() {
    m_pauseTimer->start(2000); // 2 second pause
}

void MarqueeLabel::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);

    // Draw stylesheet background
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    painter.setFont(font());
    
    int indent = 10;
    int drawWidth = width() - 2 * indent;
    if (drawWidth <= 0) return;

    QColor textColor = ThemeManager::instance().getTextColor();

    if (m_textWidth > drawWidth) {
        // We have scrolling text
        
        // Setup fade gradient if running
        if (m_animation->state() == QAbstractAnimation::Running) {
            QLinearGradient gradient(indent, 0, width() - indent, 0);
            QColor transparentColor = textColor;
            transparentColor.setAlpha(0);
            
            qreal fadeRatio = 20.0 / (qreal)drawWidth; // 20px fade width
            if (fadeRatio > 0.4) fadeRatio = 0.4;
            
            gradient.setColorAt(0.0, transparentColor);
            gradient.setColorAt(fadeRatio, textColor);
            gradient.setColorAt(1.0 - fadeRatio, textColor);
            gradient.setColorAt(1.0, transparentColor);
            
            painter.setPen(QPen(QBrush(gradient), 1));
        } else {
            painter.setPen(textColor);
        }
        
        // Clip to drawing area so it doesn't draw into the 10px indents
        painter.setClipRect(indent, 0, drawWidth, height());
        
        // Scrolling text
        painter.drawText(indent - m_scrollOffset, 0, m_textWidth, height(), Qt::AlignVCenter | Qt::AlignLeft, m_text);
        
        // Draw the second string if we are scrolling far enough to need looping
        if (m_scrollOffset > m_textWidth - drawWidth + 50) {
           painter.drawText(indent - m_scrollOffset + m_textWidth + 50, 0, m_textWidth, height(), Qt::AlignVCenter | Qt::AlignLeft, m_text);
        }
    } else {
        // Centered static text
        painter.setPen(textColor);
        painter.drawText(indent, 0, drawWidth, height(), Qt::AlignCenter | Qt::TextWordWrap, m_text);
    }
}

void MarqueeLabel::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateAnimation();
}

void MarqueeLabel::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    updateAnimation();
}
