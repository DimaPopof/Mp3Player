#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class CustomTitleBar : public QWidget {
    Q_OBJECT
public:
    explicit CustomTitleBar(QWidget *parent = nullptr);

signals:
    void toggleViewRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *appIconLabel;
    QLabel *titleLabel;
    QPushButton *minimizeBtn;
    QPushButton *maximizeBtn;
    QPushButton *closeBtn;

    QPoint dragPosition;

private slots:
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();
};

#endif // CUSTOMTITLEBAR_H
