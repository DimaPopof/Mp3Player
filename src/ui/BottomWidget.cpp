#include "BottomWidget.h"
#include "ClickableSlider.h"
#include "ThemeManager.h"
#include <QPainter>
#include <QStyle>
// #include "ClickableSlider.h" // Include your custom slider

BottomWidget::BottomWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("bottomWidget");
  setAttribute(Qt::WA_StyledBackground, true);

  // Включаем отрисовку фона через стили
  setMinimumHeight(50);
  setupUi();
  setupConnections();
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          &BottomWidget::refreshTheme);
  refreshTheme();
}

void BottomWidget::setupUi() {
  QHBoxLayout *bottomLayout = new QHBoxLayout(this);
  bottomLayout->setContentsMargins(10, 0, 10, 0);
  const QSize mediaIconSize(14, 14);

  prevButton = new QPushButton(this);
  prevButton->setObjectName("prevButton");
  prevButton->setIcon(tintedIcon(QStyle::SP_MediaSkipBackward, mediaIconSize,
                                 ThemeManager::instance().getMediaIconColor()));

  playPauseButton = new QPushButton(this);
  playPauseButton->setObjectName("playPauseButton");
  playPauseButton->setIcon(
      tintedIcon(QStyle::SP_MediaPlay, mediaIconSize,
                 ThemeManager::instance().getMediaIconColor()));

  nextButton = new QPushButton(this);
  nextButton->setObjectName("nextButton");
  nextButton->setIcon(tintedIcon(QStyle::SP_MediaSkipForward, mediaIconSize,
                                 ThemeManager::instance().getMediaIconColor()));

  repeatButton = new QPushButton(this);
  repeatButton->setObjectName("repeatButton");
  repeatButton->setCheckable(true);

  shuffleButton = new QPushButton(this);
  shuffleButton->setObjectName("shuffleButton");
  shuffleButton->setCheckable(true);

  // Volume button rendering logic (copied from your code)
  volumeButton = new QPushButton(this);
  volumeButton->setObjectName("volumeButton");
  volumeButton->setIcon(
      tintedIcon(QStyle::SP_MediaVolume, QSize(24, 24),
                 ThemeManager::instance().getIconTintColor()));
  volumeButton->setIconSize(QSize(20, 20));
  volumeButton->setFixedSize(25, 25);

  volumeSlider = new QSlider(Qt::Horizontal, this);
  volumeSlider->setObjectName("volumeSlider");
  volumeSlider->setRange(0, 100);
  volumeSlider->setValue(50);
  volumeSlider->setMaximumWidth(100);
  volumeSlider->hide();

  timeLabel = new QLabel("00:00 / 00:00", this);
  timeLabel->setObjectName("timeLabel");

  bufferProgressBar = new QProgressBar(this);
  bufferProgressBar->setObjectName("bufferProgressBar");
  bufferProgressBar->setTextVisible(false);
  bufferProgressBar->setFixedHeight(24); // Match slider height
  bufferProgressBar->setAttribute(Qt::WA_TransparentForMouseEvents, true);

  timelineSlider = new ClickableSlider(Qt::Horizontal, this);
  timelineSlider->setObjectName("timelineSlider");
  timelineSlider->setFixedHeight(24); // Give it some height for easier clicking

  bottomLayout->addWidget(prevButton);
  bottomLayout->addWidget(playPauseButton);
  bottomLayout->addWidget(nextButton);
  bottomLayout->addWidget(volumeButton);
  bottomLayout->addWidget(volumeSlider);
  bottomLayout->addWidget(timeLabel);

  QWidget *sliderContainer = new QWidget(this);
  sliderContainer->setObjectName("sliderContainer");
  sliderContainer->setFixedHeight(24);
  sliderContainer->setAttribute(Qt::WA_TranslucentBackground);
  QStackedLayout *stackedLayout = new QStackedLayout(sliderContainer);
  stackedLayout->setStackingMode(QStackedLayout::StackAll);
  stackedLayout->setContentsMargins(0, 0, 0, 0);

  // IMPORTANT: Widget added first is at the bottom.
  stackedLayout->addWidget(bufferProgressBar);
  stackedLayout->addWidget(timelineSlider);
  bufferProgressBar->lower();
  timelineSlider->raise();

  bottomLayout->addWidget(sliderContainer, 1);
  bottomLayout->addWidget(repeatButton);
  bottomLayout->addWidget(shuffleButton);
}

void BottomWidget::setupConnections() {
  // Map internal button clicks to public signals
  connect(prevButton, &QPushButton::clicked, this,
          &BottomWidget::previousClicked);
  connect(nextButton, &QPushButton::clicked, this, &BottomWidget::nextClicked);
  connect(playPauseButton, &QPushButton::clicked, this,
          &BottomWidget::playPauseClicked);
  volumeButton->installEventFilter(this);
  connect(volumeButton, &QPushButton::clicked, this,
          &BottomWidget::volumeMuteClicked);
  connect(volumeSlider, &QSlider::valueChanged, this,
          &BottomWidget::volumeChanged);
  connect(repeatButton, &QPushButton::toggled, this,
          &BottomWidget::repeatToggled);
  connect(shuffleButton, &QPushButton::toggled, this,
          &BottomWidget::shuffleToggled);
  connect(timelineSlider, &QSlider::sliderMoved, this, [this](int value) {
    timeLabel->setText(
        QString("%1 / %2").arg(formatTime(value / 1000),
                               formatTime(timelineSlider->maximum() / 1000)));
  });
  connect(timelineSlider, &QSlider::sliderReleased, this,
          [this]() { emit seekRequested(timelineSlider->value() / 1000); });
}
void BottomWidget::updatePlayButtonState(bool isPlaying) {
  m_isPlaying = isPlaying;
  const QSize mediaIconSize(14, 14);

  if (isPlaying) {
    playPauseButton->setIcon(
        tintedIcon(QStyle::SP_MediaPause, mediaIconSize,
                   ThemeManager::instance().getMediaIconColor()));
  } else {
    playPauseButton->setIcon(
        tintedIcon(QStyle::SP_MediaPlay, mediaIconSize,
                   ThemeManager::instance().getMediaIconColor()));
  }
}
void BottomWidget::updateVolumeIcon(bool isMuted) {
  m_isMuted = isMuted;
  if (isMuted) {
    volumeButton->setIcon(
        tintedIcon(QStyle::SP_MediaVolumeMuted, QSize(24, 24),
                   ThemeManager::instance().getMutedIconColor()));
  } else {
    volumeButton->setIcon(
        tintedIcon(QStyle::SP_MediaVolume, QSize(24, 24),
                   ThemeManager::instance().getIconTintColor()));
  }
}

void BottomWidget::refreshTheme() {
  updatePlayButtonState(m_isPlaying);
  updateVolumeIcon(m_isMuted);

  const QSize mediaIconSize(14, 14);
  const QSize extraIconSize(20, 20);
  repeatButton->setIcon(tintedCustomIcon(":/assets/repeat-button.svg", extraIconSize,
                                         ThemeManager::instance().getMediaIconColor()));
  shuffleButton->setIcon(tintedCustomIcon(":/assets/shuffle.svg", extraIconSize,
                                          ThemeManager::instance().getMediaIconColor()));
}

QIcon BottomWidget::tintedIcon(QStyle::StandardPixmap sp, const QSize &size,
                               const QColor &color) const {
  QPixmap src = style()->standardIcon(sp).pixmap(size);
  QPixmap tinted(src.size());
  tinted.fill(Qt::transparent);

  QPainter painter(&tinted);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.drawPixmap(0, 0, src);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(tinted.rect(), color);

  return QIcon(tinted);
}

QIcon BottomWidget::tintedCustomIcon(const QString &path, const QSize &size,
                                     const QColor &color) const {
  QPixmap src(path);
  if (src.isNull()) {
    return QIcon();
  }
  src = src.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  QPixmap tinted(src.size());
  tinted.fill(Qt::transparent);

  QPainter painter(&tinted);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.drawPixmap(0, 0, src);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(tinted.rect(), color);

  return QIcon(tinted);
}
QString BottomWidget::formatTime(int totalSeconds) {
  int seconds = totalSeconds % 60;
  int minutes = (totalSeconds / 60) % 60;

  // Форматируем строку как ММ:СС
  return QString("%1:%2")
      .arg(minutes, 2, 10, QChar('0'))
      .arg(seconds, 2, 10, QChar('0'));
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

  timeLabel->setText(QString("%1 / %2").arg(formatTime(positionMs / 1000),
                                            formatTime(durationMs / 1000)));
}
int BottomWidget::getVolumeValue() const { return volumeSlider->value(); }
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

void BottomWidget::resetSlider() { timelineSlider->setValue(0); }

void BottomWidget::updateBufferedAmount(qint64 bufferedMs) {
  int val = static_cast<int>(bufferedMs);
  if (val > bufferProgressBar->maximum()) {
    val = bufferProgressBar->maximum();
  }
  bufferProgressBar->setValue(val);
}