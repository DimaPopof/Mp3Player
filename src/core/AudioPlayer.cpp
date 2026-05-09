#include "AudioPlayer.h"
#include <QDebug>
#include <iostream>
#include <QFileInfo>
#include "../ui/Visualizer.h"

AudioPlayer* AudioPlayer::s_currentInstance = nullptr;

// Data callback - runs on a high priority audio thread
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    Q_UNUSED(pInput);
    if (!pDevice || !pDevice->pUserData) return;
    
    AudioPlayer* player = (AudioPlayer*)pDevice->pUserData;

    // NO MUTEX HERE. Use atomic flags for speed.
    if (player->isPlaying && player->isSoundLoaded && player->m_isBufferReady) {
        ma_uint32 available = ma_pcm_rb_available_read(&player->rb);
        ma_uint32 framesToProcess = std::min(frameCount, available);

        if (framesToProcess > 0) {
            void* pReadBuffer = nullptr;
            ma_uint32 acquiredFrames = framesToProcess;
            // ma_pcm_rb_acquire_read is thread-safe for 1R/1W
            ma_result res = ma_pcm_rb_acquire_read(&player->rb, &acquiredFrames, &pReadBuffer);
            
            if (res == MA_SUCCESS && pReadBuffer && acquiredFrames > 0) {
                memcpy(pOutput, pReadBuffer, acquiredFrames * AudioPlayer::CHANNELS * sizeof(float));
                
                float* outputF32 = (float*)pOutput;
                
                // First, prepare data for visualizer at a constant 0.5 volume and flush denormals
                for (ma_uint32 i = 0; i < acquiredFrames * AudioPlayer::CHANNELS; ++i) {
                    float val = outputF32[i];
                    // Prevent denormal (subnormal) floats which cause severe CPU penalties in FFT
                    if (val > -1e-15f && val < 1e-15f) {
                        val = 0.0f;
                    }
                    outputF32[i] = val * 0.4f; // Constant 0.4 for visualizer
                }
                
                // Push to visualizer buffer
                player->pushToVisualizerBuffer(outputF32, acquiredFrames);
                
                // Now, apply the actual user volume to the output
                // Since it's currently at 0.4, we multiply by (userVolume / 0.4) = userVolume * 2.5
                float userVolumeMultiplier = player->m_volume.load() * 2.5f;
                for (ma_uint32 i = 0; i < acquiredFrames * AudioPlayer::CHANNELS; ++i) {
                    outputF32[i] *= userVolumeMultiplier;
                }

                ma_pcm_rb_commit_read(&player->rb, acquiredFrames);
                
                // Update total frames played for position tracking
                player->m_framesPlayed += acquiredFrames;

                // If we didn't get enough frames, fill the rest with silence
                if (acquiredFrames < frameCount) {
                    ma_uint32 remaining = frameCount - acquiredFrames;
                    float* pRemainingOutput = (float*)pOutput + (acquiredFrames * AudioPlayer::CHANNELS);
                    memset(pRemainingOutput, 0, remaining * AudioPlayer::CHANNELS * sizeof(float));
                    player->pushToVisualizerBuffer(pRemainingOutput, remaining);
                }
            } else {
                memset(pOutput, 0, frameCount * AudioPlayer::CHANNELS * sizeof(float));
                player->pushToVisualizerBuffer((float*)pOutput, frameCount);
            }
        } else {
            // Buffer underflow - output silence
            memset(pOutput, 0, frameCount * AudioPlayer::CHANNELS * sizeof(float));
            player->pushToVisualizerBuffer((float*)pOutput, frameCount);
        }
    } else {
        // Output silence if paused or stopped
        memset(pOutput, 0, frameCount * AudioPlayer::CHANNELS * sizeof(float));
        player->pushToVisualizerBuffer((float*)pOutput, frameCount);
    }
}

AudioPlayer::AudioPlayer(QObject *parent) : 
        QObject(parent), visualizerBuffer(16384), m_volume(1.0f) {
    qDebug() << "AudioPlayer: Initializing...";
    s_currentInstance = this;

    ma_result res = ma_pcm_rb_init(ma_format_f32, CHANNELS, SAMPLE_RATE * BUFFER_SIZE_SECONDS, NULL, NULL, &rb);
    if (res != MA_SUCCESS) {
        qDebug() << "AudioPlayer: Failed to initialize ring buffer:" << (int)res;
    }

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = CHANNELS;
    deviceConfig.sampleRate        = SAMPLE_RATE;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = this;

    res = ma_device_init(NULL, &deviceConfig, &device);
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
    if(decoderThread.joinable()) {
        decoderThread.join();
    }

    ma_device_uninit(&device);
    ma_pcm_rb_uninit(&rb);

    if (m_visualizer) {
        m_visualizer->stopVisuals();
    }
    s_currentInstance = nullptr;
}

void AudioPlayer::loadMedia(const QUrl& url) {
    QString filePath = url.toLocalFile();
    if(filePath.isEmpty()) return;

    // Signal that the buffer is not ready
    m_isBufferReady = false;

    // Shutdown current decoder thread
    stopDecoder = true;
    if(decoderThread.joinable()) {
        decoderThread.join();
    }
    
    std::lock_guard<std::recursive_mutex> lock(m_audioMutex);
    
    if (isSoundLoaded) {
        ma_decoder_uninit(&decoder);
        isSoundLoaded = false;
    }

    // Reset state
    m_framesPlayed = 0;
    stopDecoder = false;
    seekRequested = false;

    // init Decoder
    ma_decoder_config decoderConfig = ma_decoder_config_init_default();
    decoderConfig.format = ma_format_f32;
    decoderConfig.channels = CHANNELS;
    decoderConfig.sampleRate = SAMPLE_RATE;

    ma_result result = ma_decoder_init_file(filePath.toUtf8().constData(), &decoderConfig, &decoder);
    if (result != MA_SUCCESS) {
        qDebug() << "AudioPlayer: Failed to init decoder:" << (int)result;
        return;
    }
     
    // Reset ring buffer
    ma_pcm_rb_reset(&rb);
    
    ma_uint64 lengthInFrames;
    ma_decoder_get_length_in_pcm_frames(&decoder, &lengthInFrames);
    qint64 durationMs = (qint64)((lengthInFrames * 1000) / SAMPLE_RATE);
    m_durationMs.store(durationMs);
    emit durationChanged(durationMs);
    
    if (m_visualizer) {
        QFileInfo fi(filePath);
        QString fileName = fi.fileName();
        QMetaObject::invokeMethod(m_visualizer, [this, fileName]() {
            m_visualizer->setWindowTitle(fileName);
        }, Qt::QueuedConnection);
    }

    isSoundLoaded = true;
    m_isBufferReady = true; // Buffer is now valid (though empty)
    decoderThread = std::thread(&AudioPlayer::decoderThreadFunc, this);
    
    if (isPlaying) {
        play(); 
    }
}


void AudioPlayer::decoderThreadFunc() {
    decoderThreadRunning = true;
    const ma_uint32 CHUNK_SIZE = 4096;
    float tempBuffer[CHUNK_SIZE * CHANNELS]; 
    bool eofReached = false;

    while(!stopDecoder) {
        // Handle Seek Request
        if (seekRequested) {
            std::lock_guard<std::recursive_mutex> lock(m_audioMutex);
            ma_uint64 seekFrame = (ma_uint64)seekTargetSeconds * SAMPLE_RATE;
            ma_result res = ma_decoder_seek_to_pcm_frame(&decoder, seekFrame);
            if (res == MA_SUCCESS) {
                m_isBufferReady = false; // Briefly disable to clear safely
                ma_pcm_rb_reset(&rb);
                m_framesPlayed = seekFrame;
                m_isBufferReady = true;
                eofReached = false; // Reset EOF flag on seek
            }
            seekRequested = false;
        }

        ma_uint32 availableRead = ma_pcm_rb_available_read(&rb);
        
        // If we reached the end of the file, just wait for the buffer to drain
        if (eofReached) {
            if (availableRead == 0) {
                if (isPlaying) {
                    isPlaying = false;
                    emit trackFinished();
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        ma_uint32 availableWrite = ma_pcm_rb_available_write(&rb);
        
        // If buffer is practically full, just wait.
        if (availableWrite < CHUNK_SIZE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        ma_uint64 framesToRead = CHUNK_SIZE;
        ma_uint64 framesRead = 0;
        
        ma_result result;
        {
            // Only lock for the actual decoder read
            std::lock_guard<std::recursive_mutex> lock(m_audioMutex);
            result = ma_decoder_read_pcm_frames(&decoder, tempBuffer, framesToRead, &framesRead);
        }
        
        if (framesRead > 0) {
            ma_uint32 totalFramesToWrite = (ma_uint32)framesRead;
            float* pReadPtr = tempBuffer;
            
            // Loop to handle ring buffer wrap-around
            while (totalFramesToWrite > 0 && !stopDecoder) {
                ma_uint32 framesToWriteThisIteration = totalFramesToWrite;
                void* pWriteBuf = nullptr;
                if (ma_pcm_rb_acquire_write(&rb, &framesToWriteThisIteration, &pWriteBuf) == MA_SUCCESS) {
                    if (framesToWriteThisIteration == 0) break;
                    memcpy(pWriteBuf, pReadPtr, framesToWriteThisIteration * CHANNELS * sizeof(float));
                    ma_pcm_rb_commit_write(&rb, framesToWriteThisIteration);
                    pReadPtr += framesToWriteThisIteration * CHANNELS;
                    totalFramesToWrite -= framesToWriteThisIteration;
                } else {
                    break;
                }
            }
        }

        if (result == MA_AT_END) {
            eofReached = true;
            continue; // Go to the top of the loop to handle EOF draining
        } else if (result != MA_SUCCESS) {
            break; // Fatal error
        }

        // Paced buffering:
        if (availableRead < (SAMPLE_RATE * 3)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    decoderThreadRunning = false;
}

void AudioPlayer::play() {
    if (!isSoundLoaded) return;
    isPlaying = true;
    emit playbackStateChanged(true);
    positionTimer->start(500); // UI syncs every 500ms, UI handles faked smooth animation
    if (m_visualizer) m_visualizer->playVisuals();
}


void AudioPlayer::pause() {
    isPlaying = false;
    emit playbackStateChanged(false);
    positionTimer->stop();
    if (m_visualizer) m_visualizer->stopVisuals();
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
    if (m_visualizer) m_visualizer->stopVisuals();
    setPosition(0);
}

int AudioPlayer::position() const {
    return (int)(m_framesPlayed.load() / SAMPLE_RATE);
}

qint64 AudioPlayer::positionMs() const {
    return (qint64)((m_framesPlayed.load() * 1000) / SAMPLE_RATE);
}

AudioPlayer* AudioPlayer::currentInstance() {
    return s_currentInstance;
}

int AudioPlayer::duration() const {
    return (int)(m_durationMs.load() / 1000);
}

qint64 AudioPlayer::durationMs() const {
    return m_durationMs.load();
}

void AudioPlayer::setVolume(float volumeLevel) {
    m_volume = volumeLevel;
}

float AudioPlayer::getVolume() const {
    return m_volume.load();
}

bool AudioPlayer::isAudioPlaying() const {
    return isPlaying;
}

std::size_t AudioPlayer::readVisualizer(float* dst, std::size_t maxSamples) {
    return visualizerBuffer.read(dst, maxSamples);
}

void AudioPlayer::pushToVisualizerBuffer(float* pcmFrames, ma_uint32 frameCount) {
    visualizerBuffer.write(pcmFrames, frameCount * CHANNELS);
}

int AudioPlayer::getBufferedSeconds() const {
    if (!isSoundLoaded) return 0;
    return position() + (int)(ma_pcm_rb_available_read((ma_pcm_rb*)&rb) / SAMPLE_RATE);
}

qint64 AudioPlayer::getBufferedMs() const {
    if (!isSoundLoaded) return 0;
    return positionMs() + (qint64)((ma_pcm_rb_available_read((ma_pcm_rb*)&rb) * 1000) / SAMPLE_RATE);
}

