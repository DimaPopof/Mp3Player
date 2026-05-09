#include "BottomWidget.h"
#include "ClickableSlider.h"
#include <QStyle>
#include <QPainter>
// #include "ClickableSlider.h" // Include your custom slider

BottomWidget::BottomWidget(QWidget *parent) : QWidget(parent) {
    setObjectName("bottomWidget");
    
    // Включаем отрисовку фона через стили
    setAttribute(Qt::WA_StyledBackground, true);

    // Применяем стиль строго к самому виджету, используя его objectName
    setStyleSheet(
        "#bottomWidget {"
        "   background-color: rgba(30, 30, 30, 200);"
        "   border-radius: 0px;"
        "   padding: 0px;"
        "}"
    );
    setMinimumHeight(50);
    setupUi();
    setupConnections();
}

void BottomWidget::setupUi() {
    QHBoxLayout *bottomLayout = new QHBoxLayout(this);
    bottomLayout->setContentsMargins(10, 0, 10, 0);
    const QSize mediaIconSize(14, 14);

    auto makeBlackIcon = [this](QStyle::StandardPixmap sp, const QSize &size) {
        QPixmap src = style()->standardIcon(sp).pixmap(size);
        QPixmap tinted(src.size());
        tinted.fill(Qt::transparent);
        QPainter p(&tinted);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawPixmap(0, 0, src);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(tinted.rect(), QColor(0, 0, 0, 190));
        p.end();
        return QIcon(tinted);
    };

    QString blueStyle = "QPushButton { background-color: #47b6fb; color: black; border: none; border-radius: 4px; padding: 6px; }"
                        "QPushButton:hover { background-color: #3d91ff; }";
    QString redStyle = "QPushButton { background-color: #ff6666; border: none; border-radius: 4px; padding: 6px; }"
                       "QPushButton:hover { background-color: #c14444; }";

    prevButton = new QPushButton(this);
    prevButton->setIcon(makeBlackIcon(QStyle::SP_MediaSkipBackward, mediaIconSize));
    prevButton->setStyleSheet(blueStyle);

    playPauseButton = new QPushButton(this);
    playPauseButton->setIcon(makeBlackIcon(QStyle::SP_MediaPlay, mediaIconSize));
    playPauseButton->setStyleSheet(redStyle);

    nextButton = new QPushButton(this);
    nextButton->setIcon(makeBlackIcon(QStyle::SP_MediaSkipForward, mediaIconSize));
    nextButton->setStyleSheet(blueStyle);

    // Volume button rendering logic (copied from your code)
    volumeButton = new QPushButton(this);
    QPixmap volumePixmap = style()->standardIcon(QStyle::SP_MediaVolume).pixmap(24, 24);
    QPixmap volumeColoredPixmap(volumePixmap.size());
    volumeColoredPixmap.fill(Qt::transparent);
    QPainter iconPainter(&volumeColoredPixmap);
    iconPainter.setCompositionMode(QPainter::CompositionMode_Source);
    iconPainter.drawPixmap(0, 0, volumePixmap);
    iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    iconPainter.fillRect(volumeColoredPixmap.rect(), QColor("#ffffff"));
    iconPainter.end();
    
    volumeButton->setIcon(QIcon(volumeColoredPixmap));
    volumeButton->setIconSize(QSize(20, 20));
    volumeButton->setFixedSize(25, 25);
    volumeButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(71, 182, 251, 0.25); }"
    );

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);
    volumeSlider->setMaximumWidth(100);
    volumeSlider->hide();

    timeLabel = new QLabel("00:00 / 00:00", this);
    timeLabel->setStyleSheet("color: #FFFFFF;");
    
    
    bufferProgressBar = new QProgressBar(this);
    bufferProgressBar->setTextVisible(false);
    bufferProgressBar->setFixedHeight(24); // Match slider height
    bufferProgressBar->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    bufferProgressBar->setStyleSheet(
        "QProgressBar {"
        "   background: rgba(255, 255, 255, 0.05);"
        "   border: none;"
        "   border-radius: 3px;"
        "   margin-top: 9px;"
        "   margin-bottom: 9px;"
        "}"
        "QProgressBar::chunk {"
        "   background: rgba(71, 182, 251, 0.3);" // Light blue, semi-transparent
        "   border-radius: 3px;"
        "}" 
    );

    timelineSlider = new ClickableSlider(Qt::Horizontal, this);
    timelineSlider->setFixedHeight(24); // Give it some height for easier clicking
    timelineSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "   background: transparent;"
        "   height: 6px;"
        "   border-radius: 3px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "   background: #47b6fb;" // Match the blue theme
        "   height: 6px;"
        "   border-radius: 3px;"
        "}"
        "QSlider::add-page:horizontal {"
        "   background: transparent;" // Let the buffer bar show through
        "   height: 6px;"
        "   border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: white;"
        "   width: 16px;"
        "   height: 16px;"
        "   margin: -5px 0;"
        "   border-radius: 8px;"
        "   border: none;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "   background: #ff6666;"
        "}"
    );

    bottomLayout->addWidget(prevButton);
    bottomLayout->addWidget(playPauseButton);
    bottomLayout->addWidget(nextButton);
    bottomLayout->addWidget(volumeButton);
    bottomLayout->addWidget(volumeSlider);
    bottomLayout->addWidget(timeLabel);
    
    QWidget* sliderContainer = new QWidget(this);
    sliderContainer->setFixedHeight(24);
    sliderContainer->setAttribute(Qt::WA_TranslucentBackground);
    sliderContainer->setStyleSheet("background: transparent;");
    QStackedLayout* stackedLayout = new QStackedLayout(sliderContainer);
    stackedLayout->setStackingMode(QStackedLayout::StackAll);
    stackedLayout->setContentsMargins(0, 0, 0, 0);
    
    // IMPORTANT: Widget added first is at the bottom.
    stackedLayout->addWidget(bufferProgressBar);
    stackedLayout->addWidget(timelineSlider);
    bufferProgressBar->lower();
    timelineSlider->raise();
    
    bottomLayout->addWidget(sliderContainer, 1);
}

void BottomWidget::setupConnections() {
    // Map internal button clicks to public signals
    connect(prevButton, &QPushButton::clicked, this, &BottomWidget::previousClicked);
    connect(nextButton, &QPushButton::clicked, this, &BottomWidget::nextClicked);
    connect(playPauseButton, &QPushButton::clicked, this, &BottomWidget::playPauseClicked);
    volumeButton->installEventFilter(this);
    connect(volumeButton, &QPushButton::clicked, this, &BottomWidget::volumeMuteClicked);
    connect(volumeSlider, &QSlider::valueChanged, this, &BottomWidget::volumeChanged);
    connect(timelineSlider, &QSlider::sliderMoved, this, [this](int value) {
        timeLabel->setText(QString("%1 / %2").arg(formatTime(value / 1000), formatTime(timelineSlider->maximum() / 1000)));
    });
    connect(timelineSlider, &QSlider::sliderReleased, this, [this]() {
        emit seekRequested(timelineSlider->value() / 1000);
    });
    
}
void BottomWidget::updatePlayButtonState(bool isPlaying) {
    const QSize mediaIconSize(14, 14);
    auto makeBlackIcon = [this](QStyle::StandardPixmap sp, const QSize &size) {
        QPixmap src = style()->standardIcon(sp).pixmap(size);
        QPixmap tinted(src.size());
        tinted.fill(Qt::transparent);
        QPainter p(&tinted);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawPixmap(0, 0, src);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(tinted.rect(), QColor(0, 0, 0, 190));
        p.end();
        return QIcon(tinted);
    };

    if (isPlaying) {
        playPauseButton->setIcon(makeBlackIcon(QStyle::SP_MediaPause, mediaIconSize));
    } else {
        playPauseButton->setIcon(makeBlackIcon(QStyle::SP_MediaPlay, mediaIconSize));
    }
}
void BottomWidget::updateVolumeIcon(bool isMuted) {
    if (isMuted) {
        QPixmap mutedPixmap = style()->standardIcon(QStyle::SP_MediaVolumeMuted).pixmap(24, 24);
        QPixmap mutedColoredPixmap(mutedPixmap.size());
        mutedColoredPixmap.fill(Qt::transparent);
        QPainter iconPainter(&mutedColoredPixmap);
        iconPainter.setCompositionMode(QPainter::CompositionMode_Source);
        iconPainter.drawPixmap(0, 0, mutedPixmap);
        iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        iconPainter.fillRect(mutedColoredPixmap.rect(), QColor("#ff6666"));
        iconPainter.end();
        
        volumeButton->setIcon(QIcon(mutedColoredPixmap));
    } else {
        QPixmap volumePixmap = style()->standardIcon(QStyle::SP_MediaVolume).pixmap(24, 24);
        QPixmap volumeColoredPixmap(volumePixmap.size());
        volumeColoredPixmap.fill(Qt::transparent);
        QPainter iconPainter(&volumeColoredPixmap);
        iconPainter.setCompositionMode(QPainter::CompositionMode_Source);
        iconPainter.drawPixmap(0, 0, volumePixmap);
        iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        iconPainter.fillRect(volumeColoredPixmap.rect(), QColor("#ffffff"));
        iconPainter.end();
        
        volumeButton->setIcon(QIcon(volumeColoredPixmap));
    }
}
QString BottomWidget::formatTime(int totalSeconds) {
    int seconds = totalSeconds % 60;
    int minutes = (totalSeconds / 60) % 60;
    
    // Форматируем строку как ММ:СС
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

void BottomWidget::setDuration(qint64 durationMs) {
    timelineSlider->setRange(0, durationMs);
    bufferProgressBar->setRange(0, durationMs);
    timeLabel->setText(QString("00:00 / %1").arg(formatTime(durationMs / 1000)));
}

void BottomWidget::updatePosition(qint64 positionMs, qint64 durationMs) {
    // Only update slider if user is not actively dragging it
    if (!timelineSlider->isSliderDown()) {
        timelineSlider->setValue(positionMs);
    }
    
    timeLabel->setText(QString("%1 / %2").arg(formatTime(positionMs / 1000), formatTime(durationMs / 1000)));
}
int BottomWidget::getVolumeValue() const {
    return volumeSlider->value();
}
void BottomWidget::setVolumeValue(int value) {
    bool wasBlocked = volumeSlider->blockSignals(true);
    
    volumeSlider->setValue(value);
    
    volumeSlider->blockSignals(wasBlocked);
}
bool BottomWidget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == volumeButton && event->type() == QEvent::Enter) {
        volumeSlider->show();
    }
    return QWidget::eventFilter(obj, event);
}

void BottomWidget::leaveEvent(QEvent *event) {
    // Когда мышь покидает пределы нижней панели, прячем слайдер
    volumeSlider->hide();
    QWidget::leaveEvent(event);
}

void BottomWidget::resetSlider() {
    timelineSlider->setValue(0);
}

void BottomWidget::updateBufferedAmount(qint64 bufferedMs) {
    int val = static_cast<int>(bufferedMs);
    if (val > bufferProgressBar->maximum()) {
        val = bufferProgressBar->maximum();
    }
    bufferProgressBar->setValue(val);
}