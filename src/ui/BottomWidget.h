#pragma once
#include <QColor>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSize>
#include <QSlider>
#include <QStackedLayout>
#include <QString>
#include <QStyle>
#include <QWidget>

class ClickableSlider; // Forward declaration

class BottomWidget : public QWidget {
  Q_OBJECT

public:
  explicit BottomWidget(QWidget *parent = nullptr);

  // Methods to update UI from MainWindow
  void setTimeLabel(const QString &timeString);
  void setVolumeValue(int value);
  int getVolumeValue() const;
  void updatePlayButtonState(bool isPlaying);
  void updateVolumeIcon(bool isMuted);
  void setDuration(qint64 durationMs);
  void updatePosition(qint64 positionMs, qint64 durationMs);
  void updateBufferedAmount(qint64 bufferedMs);
  void resetSlider();
  void refreshTheme();

signals:
  // Signals emitted when user interacts with this widget
  void previousClicked();
  void nextClicked();
  void playPauseClicked();
  void volumeMuteClicked();
  void volumeChanged(int value);
  void seekRequested(int position);

private:
  QString formatTime(int ms);
  QPushButton *prevButton;
  QPushButton *playPauseButton;
  QPushButton *nextButton;
  QPushButton *volumeButton;
  QSlider *volumeSlider;
  QLabel *timeLabel;
  ClickableSlider *timelineSlider;
  QProgressBar *bufferProgressBar;
  bool m_isPlaying{false};
  bool m_isMuted{false};

  void setupUi();
  void setupConnections();
  QIcon tintedIcon(QStyle::StandardPixmap sp, const QSize &size,
                   const QColor &color) const;

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;
  void leaveEvent(QEvent *event) override;
};