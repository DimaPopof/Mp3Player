// AudioPlayer.h
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QUrl>
#include <QString>
#include <QTimer>
#include "miniaudio.h"

class AudioPlayer : public QObject {
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer();
    int position() const;
    int duration() const;
    void loadMedia(const QUrl &fileUrl);
    void play();
    void pause();
    void stop();
    void setVolume(float volumeLevel);
    bool isAudioPlaying() const;
    void setPosition(int seconds);

signals:
    void playbackStateChanged(bool isPlaying);
    // Signals for UI updates
    void positionChanged(int positionInSeconds);
    void durationChanged(int durationInSeconds);
    void trackFinished();

private slots:
    // Slot to calculate current playback time
    void updateTime();

private:
    ma_engine engine;
    ma_sound sound;
    bool isSoundLoaded;
    bool isPlaying;
    
    QTimer *positionTimer;
};

#endif // AUDIOPLAYER_H