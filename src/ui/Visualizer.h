#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QWidget>
#include <QTimer>
#include <QColor>
#include <vector>
#include <complex>

// Используем forward declaration, чтобы не инклудить AudioPlayer.h здесь
class AudioPlayer;

class Visualizer : public QWidget {
    Q_OBJECT

public:
    explicit Visualizer(QWidget *parent = nullptr);
    ~Visualizer() override;
    void setAudioPlayer(AudioPlayer *player);
    void setInputScale(float value);
    
    void playVisuals();
    void stopVisuals();

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override { return w / 6; }

protected:
    void paintEvent(QPaintEvent *event) override;
    //void resizeEvent(QResizeEvent *event) override; // Для кэширования фона

private slots:
    void updateVisuals();

private:

    std::size_t readForVisualizer(float *dst, std::size_t maxSamples);
    void ensureBarColors(std::size_t barCount);

    void calculateFFT(const std::vector<float>& in_raw);
    qreal compressAmplitude(qreal value);

    void wave_animation();

    AudioPlayer *m_player = nullptr; // Теперь может быть nullptr
    QTimer *m_timer;

    // Буферы
    std::vector<float> m_audioData;
    std::vector<float> m_barHeights;
    std::vector<float> m_peakHeights;
    std::vector<float> m_peakSpeeds;

    float m_dynamicMax = 0.001f; // Для автоматической регулировки усиления (AGC)
    float m_inputScale = 1.0f; // Для ручной регулировки усиления
    std::vector<float> m_rawFFTData;
    std::vector<float> m_smoothHeights; // Для плавного спада баров
    std::vector<float> m_smearHeights;  // Для эффекта "хвоста" пиков
    struct BarColors {
        QColor barPenColor;
        QColor smearCenterColor;
        QColor smearEdgeColor;
        QColor glowCenterColor;
        QColor glowEdgeColor;
        QColor coreColor;
        QPixmap glowPixmap;
    };
    std::vector<BarColors> m_cachedColors;
    QPixmap m_idleGlowPixmap;
    QColor m_lastIdleColor;

    // Массивы для ускорения математики (LUT)
    std::vector<float> m_gammaLUT;
    std::vector<float> m_widthLUT;
    
    // FFT оптимизация (члены класса, чтобы не выделять память в цикле)
    std::vector<std::complex<float>> m_fftComplex;
    std::vector<std::size_t> m_bitReverse;
    std::vector<float> m_hannWindowFixed;
    std::vector<std::complex<float>> m_twiddleFactors;
    
    float m_wavePhase = 0.0f;
    int m_idleFrames = 0;
    float m_idleFade = 0.0f;

    QPixmap m_bgCache; // Кэш фона для оптимизации CPU

    static constexpr std::size_t FFT_SIZE = 4096;
};

#endif // VISUALIZER_H