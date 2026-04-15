#pragma once
#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>

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
    void setDuration(int duration);
    void updatePosition(int position, int duration);
    

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

    void setupUi();
    void setupConnections();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void leaveEvent(QEvent *event) override;
};