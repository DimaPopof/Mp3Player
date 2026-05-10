#include "AudioPlayer.h"
#include "../ui/Visualizer.h"
#include <QDebug>
#include <QFileInfo>
#include <iostream>

AudioPlayer *AudioPlayer::s_currentInstance = nullptr;

// Global state for callback to handle partial reads
thread_local AudioChunk *currentChunk = nullptr;
thread_local size_t chunkReadOffset = 0;
thread_local float lastVolume = 1.0f; // For volume ramping

// Data callback - runs on a high priority audio thread
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  Q_UNUSED(pInput);
  if (!pDevice || !pDevice->pUserData)
    return;

  AudioPlayer *player = (AudioPlayer *)pDevice->pUserData;

  float *out = static_cast<float *>(pOutput);
  size_t framesNeeded = frameCount;

  if (player->isPlaying && player->isSoundLoaded && player->m_isBufferReady) {

    float targetVolume = player->m_volume.load();

    while (framesNeeded > 0) {
      // If we don't have a chunk, grab the next one
      if (!currentChunk) {
        if (!player->readyQueue.try_dequeue(currentChunk)) {
          // UNDERRUN: No audio ready! Output silence
          memset(out, 0, framesNeeded * 2 * sizeof(float));
          break;
        }
        chunkReadOffset = 0;
      }

      // Calculate how much we can read from the current chunk
      size_t framesAvailable = currentChunk->validFrames - chunkReadOffset;
      size_t framesToCopy = std::min((size_t)framesNeeded, framesAvailable);

      // Copy and apply volume with ramping
      for (size_t i = 0; i < framesToCopy; ++i) {
        // Smooth volume over samples (ramp)
        if (std::abs(lastVolume - targetVolume) > 0.001f) {
          lastVolume += (targetVolume - lastVolume) * 0.01f;
        } else {
          lastVolume = targetVolume;
        }

        float l = currentChunk->data[(chunkReadOffset + i) * 2];
        float r = currentChunk->data[(chunkReadOffset + i) * 2 + 1];

        // Visualizer Push (Constant 0.4 volume, flush denormals)
        float vis_l = (std::abs(l) < 1e-15f ? 0.0f : l) * 0.4f;
        float vis_r = (std::abs(r) < 1e-15f ? 0.0f : r) * 0.4f;
        player->visualizerQueue.try_enqueue(vis_l);
        player->visualizerQueue.try_enqueue(vis_r);

        // Speaker Output
        out[i * 2] = l * lastVolume;
        out[i * 2 + 1] = r * lastVolume;
      }

      player->m_framesPlayed += framesToCopy;
      chunkReadOffset += framesToCopy;
      framesNeeded -= framesToCopy;
      out += framesToCopy * 2;

      // If chunk is fully read, return it to the free pool
      if (chunkReadOffset >= currentChunk->validFrames) {
        player->freeQueue.enqueue(currentChunk);
        currentChunk = nullptr;
      }
    }
  } else {
    // Output silence if paused or stopped
    memset(out, 0, frameCount * AudioPlayer::CHANNELS * sizeof(float));
  }
}

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent), visualizerQueue(131072), m_volume(1.0f) {
  qDebug() << "AudioPlayer: Initializing...";
  s_currentInstance = this;

  // Pre-allocate 30 seconds worth of chunks (~320 chunks for 44.1kHz / 4096
  // frames)
  for (int i = 0; i < 350; ++i) {
    freeQueue.enqueue(new AudioChunk());
  }

  ma_device_config deviceConfig =
      ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_f32;
  deviceConfig.playback.channels = CHANNELS;
  deviceConfig.sampleRate = SAMPLE_RATE;
  deviceConfig.dataCallback = data_callback;
  deviceConfig.pUserData = this;

  ma_result res = ma_device_init(NULL, &deviceConfig, &device);
  if (res != MA_SUCCESS) {
    qDebug() << "AudioPlayer: Failed to initialize audio device:" << (int)res;
  } else {
    ma_device_start(&device);
  }

  positionTimer = new QTimer(this);
  connect(positionTimer, &QTimer::timeout, this, &AudioPlayer::updateTime);
}

AudioPlayer::~AudioPlayer() {
  stopDecoder = true;
  if (decoderThread.joinable()) {
    decoderThread.join();
  }

  ma_device_uninit(&device);

  if (m_visualizer) {
    m_visualizer->stopVisuals();
  }
  s_currentInstance = nullptr;

  // Cleanup chunks
  AudioChunk *chunk;
  while (freeQueue.try_dequeue(chunk))
    delete chunk;
  while (readyQueue.try_dequeue(chunk))
    delete chunk;
}

void AudioPlayer::loadMedia(const QUrl &url) {
  QString filePath = url.toLocalFile();
  if (filePath.isEmpty())
    return;

  m_isBufferReady = false;

  stopDecoder = true;
  if (decoderThread.joinable()) {
    decoderThread.join();
  }

  std::lock_guard<std::recursive_mutex> lock(m_audioMutex);

  if (isSoundLoaded) {
    ma_decoder_uninit(&decoder);
    isSoundLoaded = false;
  }

  m_framesPlayed = 0;
  stopDecoder = false;
  seekRequested = false;

  ma_decoder_config decoderConfig = ma_decoder_config_init_default();
  decoderConfig.format = ma_format_f32;
  decoderConfig.channels = CHANNELS;
  decoderConfig.sampleRate = SAMPLE_RATE;

  ma_result result = ma_decoder_init_file(filePath.toUtf8().constData(),
                                          &decoderConfig, &decoder);
  if (result != MA_SUCCESS) {
    qDebug() << "AudioPlayer: Failed to init decoder:" << (int)result;
    return;
  }

  // Clear readyQueue and put back to freeQueue
  AudioChunk *chunk;
  while (readyQueue.try_dequeue(chunk)) {
    freeQueue.enqueue(chunk);
  }

  ma_uint64 lengthInFrames;
  ma_decoder_get_length_in_pcm_frames(&decoder, &lengthInFrames);
  qint64 durationMs = (qint64)((lengthInFrames * 1000) / SAMPLE_RATE);
  m_durationMs.store(durationMs);
  emit durationChanged(durationMs);

  if (m_visualizer) {
    QFileInfo fi(filePath);
    QString fileName = fi.fileName();
    QMetaObject::invokeMethod(
        m_visualizer,
        [this, fileName]() { m_visualizer->setWindowTitle(fileName); },
        Qt::QueuedConnection);
  }

  isSoundLoaded = true;
  m_isBufferReady = true;
  decoderThread = std::thread(&AudioPlayer::decoderThreadFunc, this);

  if (isPlaying) {
    play();
  }
}

void AudioPlayer::decoderThreadFunc() {
  decoderThreadRunning = true;
  bool eofReached = false;

  while (!stopDecoder) {
    if (seekRequested) {
      std::lock_guard<std::recursive_mutex> lock(m_audioMutex);
      ma_uint64 seekFrame = (ma_uint64)seekTargetSeconds * SAMPLE_RATE;
      ma_result res = ma_decoder_seek_to_pcm_frame(&decoder, seekFrame);
      if (res == MA_SUCCESS) {
        m_isBufferReady = false;
        // Clear readyQueue and put back to freeQueue
        AudioChunk *chunk;
        while (readyQueue.try_dequeue(chunk)) {
          freeQueue.enqueue(chunk);
        }
        m_framesPlayed = seekFrame;
        m_isBufferReady = true;
        eofReached = false;
      }
      seekRequested = false;
    }

    if (eofReached) {
      if (readyQueue.size_approx() == 0) {
        if (isPlaying) {
          isPlaying = false;
          emit trackFinished();
        }
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      continue;
    }

    AudioChunk *chunk;
    if (freeQueue.try_dequeue(chunk)) {
      ma_uint64 framesRead = 0;
      ma_result result;
      {
        std::lock_guard<std::recursive_mutex> lock(m_audioMutex);
        result = ma_decoder_read_pcm_frames(&decoder, chunk->data, CHUNK_FRAMES,
                                            &framesRead);
      }

      chunk->validFrames = framesRead;
      readyQueue.enqueue(chunk);

      if (result == MA_AT_END) {
        eofReached = true;
      } else if (result != MA_SUCCESS && framesRead == 0) {
        break;
      }

    } else {
      // freeQueue is empty, which means readyQueue is full (30s buffer filled).
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }
  decoderThreadRunning = false;
}

void AudioPlayer::play() {
  if (!isSoundLoaded)
    return;
  isPlaying = true;
  emit playbackStateChanged(true);
  positionTimer->start(500);
  if (m_visualizer)
    m_visualizer->playVisuals();
}

void AudioPlayer::pause() {
  isPlaying = false;
  emit playbackStateChanged(false);
  positionTimer->stop();
  if (m_visualizer)
    m_visualizer->stopVisuals();
}

void AudioPlayer::setPosition(int seconds) {
  seekTargetSeconds = seconds;
  seekRequested = true;
}

void AudioPlayer::updateTime() {
  emit positionChanged(positionMs());
  emit bufferedAmountChanged(getBufferedMs());
}

void AudioPlayer::stop() {
  isPlaying = false;
  emit playbackStateChanged(false);
  if (m_visualizer)
    m_visualizer->stopVisuals();
  setPosition(0);
}

int AudioPlayer::position() const {
  return (int)(m_framesPlayed.load() / SAMPLE_RATE);
}

qint64 AudioPlayer::positionMs() const {
  return (qint64)((m_framesPlayed.load() * 1000) / SAMPLE_RATE);
}

AudioPlayer *AudioPlayer::currentInstance() { return s_currentInstance; }

int AudioPlayer::duration() const { return (int)(m_durationMs.load() / 1000); }

qint64 AudioPlayer::durationMs() const { return m_durationMs.load(); }

void AudioPlayer::setVolume(float volumeLevel) { m_volume = volumeLevel; }

float AudioPlayer::getVolume() const { return m_volume.load(); }

bool AudioPlayer::isAudioPlaying() const { return isPlaying; }

std::size_t AudioPlayer::readVisualizer(float *dst, std::size_t maxSamples) {
  std::size_t readCount = 0;
  while (readCount < maxSamples) {
    float sample;
    if (visualizerQueue.try_dequeue(sample)) {
      dst[readCount++] = sample;
    } else {
      break;
    }
  }
  return readCount;
}

void AudioPlayer::pushToVisualizerBuffer(float *pcmFrames,
                                         ma_uint32 frameCount) {
  // Deprecated, we write directly from data_callback to visualizerQueue
}

int AudioPlayer::getBufferedSeconds() const {
  if (!isSoundLoaded)
    return 0;
  size_t chunks = readyQueue.size_approx();
  return position() + (int)((chunks * CHUNK_FRAMES) / SAMPLE_RATE);
}

qint64 AudioPlayer::getBufferedMs() const {
  if (!isSoundLoaded)
    return 0;
  size_t chunks = readyQueue.size_approx();
  return positionMs() +
         (qint64)(((chunks * CHUNK_FRAMES) * 1000) / SAMPLE_RATE);
}
