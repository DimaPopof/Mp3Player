// AudioPlayer.h
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "miniaudio.h"
#include "readerwriterqueue.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>

constexpr size_t CHUNK_FRAMES = 4096;
struct AudioChunk {
  float data[CHUNK_FRAMES * 2]; // Stereo interleaved
  size_t validFrames;
  uint32_t generation;
};

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
  std::size_t readVisualizer(float *dst, std::size_t maxSamples);
  void pushToVisualizerBuffer(float *pcmFrames, ma_uint32 frameCount);
  // ma_pcm_rb* getRingBuffer() { return &rb; } // Removed since rb is removed
  void setVisualizer(Visualizer *vis) { m_visualizer = vis; }
  Visualizer *getVisualizer() const { return m_visualizer; }

  int getBufferedSeconds() const;
  qint64 getBufferedMs() const;

  void processAudioData(float *out, size_t framesNeeded);

signals:
  void playbackStateChanged(bool isPlaying);
  void positionChanged(qint64 positionMs);
  void durationChanged(qint64 durationMs);
  void bufferedAmountChanged(qint64 bufferedMs);
  void trackFinished();

private slots:
  void updateTime();

private:
  friend void data_callback(ma_device *pDevice, void *pOutput,
                            const void *pInput, ma_uint32 frameCount);

  void decoderThreadFunc();
  bool fetchNextChunk();
  void processFrames(float *out, size_t framesToProcess, float targetVolume);

  std::atomic<float> m_fadeMultiplier{1.0f};
  std::atomic<float> m_targetFade{1.0f};
  moodycamel::ReaderWriterQueue<float> visualizerQueue;
  moodycamel::ReaderWriterQueue<AudioChunk *> freeQueue;
  moodycamel::ReaderWriterQueue<AudioChunk *> readyQueue;

  ma_device device;
  ma_decoder decoder;

  std::recursive_mutex m_audioMutex; // Protects decoder and rb lifecycle
  std::thread decoderThread;
  std::atomic<bool> stopDecoder{false};
  std::atomic<bool> decoderThreadRunning{false};
  std::atomic<bool> seekRequested{false};
  std::atomic<int> seekTargetSeconds{0};

  std::atomic<ma_uint64> m_framesPlayed{
      0}; // Total frames played for current track
  std::atomic<ma_uint64> m_totalFramesDecoded{
      0}; // Total frames decoded and currently buffered
  std::atomic<bool> isSoundLoaded{false};
  std::atomic<bool> isPlaying{false};
  std::atomic<bool> m_isBufferReady{false};
  std::atomic<uint32_t> m_currentGeneration{0};
  std::atomic<float> m_volume{1.0f};
  std::atomic<qint64> m_durationMs{0};

  AudioChunk *m_currentChunk = nullptr;
  size_t m_chunkReadOffset = 0;
  float m_lastVolume = 0.0f;
  float m_fadeStep = 1.0f / 44100.0f;

  Visualizer *m_visualizer = nullptr;
  QTimer *positionTimer;
  static AudioPlayer *s_currentInstance;

  // Decoding buffer config
  static const ma_uint32 SAMPLE_RATE = 44100;
  static const ma_uint32 CHANNELS = 2;
  static const int BUFFER_SIZE_SECONDS = 30; // buffer 30 sec = 10 MB
};
#endif // AUDIOPLAYER_H
