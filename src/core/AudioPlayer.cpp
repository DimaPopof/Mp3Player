// AudioPlayer.cpp
#include "AudioPlayer.h"
#include <QDir>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

AudioPlayer::AudioPlayer(QObject *parent) : QObject(parent), isSoundLoaded(false), isPlaying(false) {
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        return;
    }

    // Initialize the timer for position updates
    positionTimer = new QTimer(this);
    connect(positionTimer, &QTimer::timeout, this, &AudioPlayer::updateTime);
    //positionTimer->start(500);
}

AudioPlayer::~AudioPlayer() {
    if (isSoundLoaded) {
        ma_sound_uninit(&sound);
    }
    ma_engine_uninit(&engine);
}

void AudioPlayer::loadMedia(const QUrl &fileUrl) {
    if (isSoundLoaded) {
        ma_sound_uninit(&sound);
        isSoundLoaded = false;
    }

    QString filePath = fileUrl.toLocalFile();

    // Use streaming flag to prevent loading the entire file into memory
    ma_uint32 flags = MA_SOUND_FLAG_STREAM;

#ifdef _WIN32
    std::wstring nativePath = QDir::toNativeSeparators(filePath).toStdWString();
    ma_result result = ma_sound_init_from_file_w(
        &engine,
        nativePath.c_str(),
        flags,
        nullptr,
        nullptr,
        &sound
    );
#else
    ma_result result = ma_sound_init_from_file(
        &engine,
        filePath.toUtf8().constData(),
        flags,
        nullptr,
        nullptr,
        &sound
    );
#endif

    if (result == MA_SUCCESS) {
        isSoundLoaded = true;
        
        // Calculate duration exactly once during load
        int trackDuration = duration();
        emit durationChanged(trackDuration);
    }
}

void AudioPlayer::play() {
    if (isSoundLoaded) {
        ma_sound_start(&sound);
        isPlaying = true;
        emit playbackStateChanged(true);
        
        // Start checking position every 1000 milliseconds (1 second)
        positionTimer->start(1000);
    }
}

void AudioPlayer::pause() {
    if (isSoundLoaded) {
        ma_sound_stop(&sound);
        isPlaying = false;
        emit playbackStateChanged(false);
        positionTimer->stop();
    }
}

void AudioPlayer::stop() {
    if (isSoundLoaded) {
        ma_sound_stop(&sound);
        ma_sound_seek_to_pcm_frame(&sound, 0);
        isPlaying = false;
        emit playbackStateChanged(false);
        positionTimer->stop();
        emit positionChanged(0);
    }
}

void AudioPlayer::setVolume(float volumeLevel) {
    if (isSoundLoaded) {
        ma_sound_set_volume(&sound, volumeLevel);
    }
}

bool AudioPlayer::isAudioPlaying() const {
    if (!isSoundLoaded) return false;
    return ma_sound_is_playing(&sound);
}

void AudioPlayer::setPosition(int seconds) {
    if (!isSoundLoaded) return;
    
    ma_uint32 sampleRate;
    ma_sound_get_data_format(&sound, NULL, NULL, &sampleRate, NULL, 0);
    
    // Convert target seconds back to PCM frames
    ma_uint64 targetFrame = static_cast<ma_uint64>(seconds) * sampleRate;
    ma_sound_seek_to_pcm_frame(&sound, targetFrame);
}

void AudioPlayer::updateTime() {
    if (!isSoundLoaded) return;
    
    // Check if the audio file reached the end
    if (ma_sound_at_end(&sound) && isPlaying) {
        isPlaying = false;
        positionTimer->stop();
        emit trackFinished();
        return;
    }
    
    ma_uint64 cursorFrames;
    ma_sound_get_cursor_in_pcm_frames(&sound, &cursorFrames);
    
    ma_uint32 sampleRate;
    ma_sound_get_data_format(&sound, NULL, NULL, &sampleRate, NULL, 0);
    
    if (sampleRate > 0) {
        int position = cursorFrames / sampleRate;
        emit positionChanged(position);
    }
}

int AudioPlayer::duration() const {
    if (!isSoundLoaded) {
        return 0;
    }

    float lengthInSeconds = 0.0f;
    ma_result result = ma_sound_get_length_in_seconds(const_cast<ma_sound*>(&sound), &lengthInSeconds);
    
    if (result == MA_SUCCESS) {
        return static_cast<int>(lengthInSeconds); // Возвращаем целые секунды
    }

    return 0;
}

int AudioPlayer::position() const {
    if (!isSoundLoaded) {
        return 0;
    }

    float cursorInSeconds = 0.0f;
    ma_result result = ma_sound_get_cursor_in_seconds(const_cast<ma_sound*>(&sound), &cursorInSeconds);
    
    if (result == MA_SUCCESS) {
        return static_cast<int>(cursorInSeconds); // Возвращаем целые секунды
    }

    return 0;
}