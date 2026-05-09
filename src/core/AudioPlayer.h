// AudioPlayer.h
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QUrl>
#include <QString>
#include <QTimer>
#include <cstddef>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "miniaudio.h"
#include "RingBuffer.h"

class Visualizer;

class AudioPlayer : public QObject {
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer();
    static AudioPlayer *currentInstance();
    int position() const; // Returns seconds for legacy support if needed
    qint64 positionMs() const;
    int duration() const; // Returns seconds
    qint64 durationMs() const;
    void loadMedia(const QUrl &fileUrl);
    void play();
    void pause();
    void stop();
    void setVolume(float volumeLevel);
    float getVolume() const;
    bool isAudioPlaying() const;
    void setPosition(int seconds);
    std::size_t readVisualizer(float* dst, std::size_t maxSamples);
    void pushToVisualizerBuffer(float* pcmFrames, ma_uint32 frameCount);
    ma_pcm_rb* getRingBuffer() { return &rb; }
    void setVisualizer(Visualizer* vis) { m_visualizer = vis; }
    Visualizer* getVisualizer() const { return m_visualizer; }

    int getBufferedSeconds() const;
    qint64 getBufferedMs() const;

signals:
    void playbackStateChanged(bool isPlaying);
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void bufferedAmountChanged(qint64 bufferedMs);
    void trackFinished();

private slots:
    void updateTime();

private:
    friend void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    
    void decoderThreadFunc();

    std::atomic<float> m_fadeMultiplier{1.0f}; 
    std::atomic<float> m_targetFade{1.0f};
    RingBuffer visualizerBuffer;
    
    ma_device device;
    ma_decoder decoder;
    ma_pcm_rb rb; // Miniaudio PCM ring buffer
    
    std::recursive_mutex m_audioMutex; // Protects decoder and rb lifecycle
    std::thread decoderThread;
    std::atomic<bool> stopDecoder{false};
    std::atomic<bool> decoderThreadRunning{false};
    std::atomic<bool> seekRequested{false};
    std::atomic<int> seekTargetSeconds{0};
    
    std::atomic<ma_uint64> m_framesPlayed{0}; // Total frames played for current track
    std::atomic<bool> isSoundLoaded{false};
    std::atomic<bool> isPlaying{false};
    std::atomic<bool> m_isBufferReady{false};
    std::atomic<float> m_volume{1.0f};
    std::atomic<qint64> m_durationMs{0};
    
    Visualizer* m_visualizer = nullptr;
    QTimer *positionTimer;
    static AudioPlayer *s_currentInstance;
    
    // Decoding buffer config
    static const ma_uint32 SAMPLE_RATE = 44100;
    static const ma_uint32 CHANNELS = 2;
    static const int BUFFER_SIZE_SECONDS = 30; // buffer 30 sec = 10 MB 
};
#endif // AUDIOPLAYER_H
