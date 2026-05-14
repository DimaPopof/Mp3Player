#include "CustomTitleBar.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMainWindow>
#include <QStyle>
#include <QWindow>

CustomTitleBar::CustomTitleBar(QWidget *parent) : QWidget(parent) {
    setFixedHeight(40);
    setObjectName("customTitleBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#customTitleBar { background-color: rgb(31, 41, 55); border-top-left-radius: 12px; border-top-right-radius: 12px; }");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(8);

    appIconLabel = new QLabel(this);
    appIconLabel->setPixmap(QPixmap(":/assets/app_icon.png").scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    appIconLabel->setFixedSize(24, 24);

    titleLabel = new QLabel("Mp3Player", this);
    titleLabel->setObjectName("titleBarLabel");

    minimizeBtn = new QPushButton("—", this);
    minimizeBtn->setFixedSize(30, 30);
    minimizeBtn->setObjectName("titleBarBtn");

    maximizeBtn = new QPushButton("□", this);
    maximizeBtn->setFixedSize(30, 30);
    maximizeBtn->setObjectName("titleBarBtn");

    closeBtn = new QPushButton("✕", this);
    closeBtn->setFixedSize(30, 30);
    closeBtn->setObjectName("titleBarCloseBtn");

    layout->addWidget(appIconLabel);
    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(minimizeBtn);
    layout->addWidget(maximizeBtn);
    layout->addWidget(closeBtn);

    connect(minimizeBtn, &QPushButton::clicked, this, &CustomTitleBar::onMinimizeClicked);
    connect(maximizeBtn, &QPushButton::clicked, this, &CustomTitleBar::onMaximizeClicked);
    connect(closeBtn, &QPushButton::clicked, this, &CustomTitleBar::onCloseClicked);
}

void CustomTitleBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (window() && window()->windowHandle()) {
            window()->windowHandle()->startSystemMove();
        }
        event->accept();
    }
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent *event) {
    Q_UNUSED(event);
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit toggleViewRequested();
        event->accept();
    }
}

void CustomTitleBar::onMinimizeClicked() {
    QMainWindow *mainWindow = qobject_cast<QMainWindow*>(window());
    if (mainWindow) {
        mainWindow->showMinimized();
    }
}

void CustomTitleBar::onMaximizeClicked() {
    emit toggleViewRequested();
}

void CustomTitleBar::onCloseClicked() {
    QMainWindow *mainWindow = qobject_cast<QMainWindow*>(window());
    if (mainWindow) {
        mainWindow->close();
    }
}
