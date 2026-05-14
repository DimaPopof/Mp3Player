#pragma once

#include <QWidget>
#include <QString>
#include <QPropertyAnimation>
#include <QFont>
#include <QTimer>

class MarqueeLabel : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int scrollOffset READ scrollOffset WRITE setScrollOffset)

public:
    explicit MarqueeLabel(const QString &text = "", QWidget *parent = nullptr);
    ~MarqueeLabel() override;

    QString text() const;
    void setText(const QString &text);

    int scrollOffset() const;
    void setScrollOffset(int offset);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void updateAnimation();
    void onAnimationFinished();

private:
    QString m_text;
    int m_scrollOffset;
    int m_textWidth;
    QPropertyAnimation *m_animation;
    QTimer *m_pauseTimer;

    void calculateTextWidth();
};
