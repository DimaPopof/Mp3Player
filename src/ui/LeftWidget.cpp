#include "LeftWidget.h"
#include <QEvent>

LeftWidget::LeftWidget(QWidget *parent) : QWidget(parent) {
    setMinimumWidth(120);
    setupUi();
    setupConnections();
}

void LeftWidget::setupUi() {
    QVBoxLayout *leftLayout = new QVBoxLayout(this);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    QHBoxLayout *folderButtonsLayout = new QHBoxLayout();

    openFolderButton = new QPushButton(this);
    openFolderButton->setIcon(QIcon(":/assets/folder.png"));
    openFolderButton->setFixedSize(50, 50);
    openFolderButton->setIconSize(QSize(45, 45));
    openFolderButton->setFlat(true);
    openFolderButton->setStyleSheet(
        "QPushButton {"
        "background: transparent;"
        "border: none;"
        "border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(140, 140, 140, 120);"
        "}"
    );

    constantFolderButton = new QPushButton(this);
    constantFolderButton->setIcon(QIcon(":/assets/cnst_folder.png"));
    constantFolderButton->setFixedSize(50, 50);
    constantFolderButton->setIconSize(QSize(45, 45));
    constantFolderButton->setFlat(true);
    constantFolderButton->setStyleSheet(
        "QPushButton {"
        "background: transparent;"
        "border: none;"
        "border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(140, 140, 140, 120);"
        "}"
    );

    folderButtonsLayout->addWidget(openFolderButton);
    folderButtonsLayout->addWidget(constantFolderButton);
    folderButtonsLayout->addStretch();

    coverLabel = new QLabel(this);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    coverLabel->setMinimumSize(50, 50);
    coverLabel->setMaximumHeight(300);

    metadataLabel = new QLabel("Artist - Title", this);
    metadataLabel->setStyleSheet(
        "background-color: rgba(30, 30, 30, 175);"
        "color: white;"
        "border-radius: 10px;"
        "padding: 5px 15px;"
        "font-weight: bold;"
    );
    metadataLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    metadataLabel->setAlignment(Qt::AlignCenter);
    metadataLabel->setWordWrap(true);

    leftLayout->addLayout(folderButtonsLayout, 0);
    leftLayout->setAlignment(folderButtonsLayout, Qt::AlignTop);
    leftLayout->addWidget(coverLabel, 1);
    leftLayout->addWidget(metadataLabel, 0, Qt::AlignBottom);
}

void LeftWidget::setupConnections() {
    connect(openFolderButton, &QPushButton::clicked, this, &LeftWidget::openFolderRequested);
    connect(constantFolderButton, &QPushButton::clicked, this, &LeftWidget::constantFolderRequested);
}
void LeftWidget::updateTrackDisplay(const TrackInfo &trackInfo) {
    // 1. Обновляем текст
    metadataLabel->setText(trackInfo.artist + " - " + trackInfo.title);
    
    // 2. Обновляем обложку
    currentCoverPixmap = trackInfo.cover;
    updateCoverImage();
}
void LeftWidget::updateCoverImage() {
    if (!currentCoverPixmap.isNull()) {
        // Get the current dimensions of the label
        QSize currentSize = coverLabel->size();
        
        // Scale the original pixmap to fit the new size while keeping aspect ratio
        QPixmap scaledPixmap = currentCoverPixmap.scaled(
            currentSize, 
            Qt::KeepAspectRatio, 
            Qt::SmoothTransformation
        );
        
        coverLabel->setPixmap(scaledPixmap);
    } else {
        coverLabel->clear();
    }
}
void LeftWidget::resetTrackDisplay() {
    metadataLabel->setText("Artist - Title");
    currentCoverPixmap = QPixmap();
    updateCoverImage(); 
}
void LeftWidget::applyFontSize(int pointSize) {
    QFont font = this->font(); // Берем текущий шрифт виджета
    font.setPointSize(pointSize);

    metadataLabel->setFont(font);
    coverLabel->setFont(font);
}
bool LeftWidget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == coverLabel && event->type() == QEvent::Resize) {
        updateCoverImage();
    }
    // Передаем остальные события базовому классу
    return QWidget::eventFilter(obj, event); 
}
