#include "Visualizer.h"

#include "core/AudioPlayer.h"
#include <QPainter>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QDebug>
#include <QColor>
#include <algorithm>
#include <cmath>
#include <complex>

namespace {

constexpr qreal kBarWidth = 2.0;
constexpr qreal kBarGap = 6.0;
constexpr qreal kBarSpacing = kBarWidth + kBarGap;

qreal compressAmplitude(qreal value) {
    // Усилитель входного сигнала (чувствительность). 
    // Если полоски слишком низкие — увеличь это число (например, до 15.0 или 20.0).
    constexpr qreal inputGain = 15.0; 
    
    // 1. Ограничиваем входящее значение от 0.0 до 1.0 (чтобы не было ошибок логарифма)
    const qreal clampedValue = std::clamp(value, 0.0, 1.0);
    
    // 2. Сама логарифмическая компрессия
    const qreal normalized = std::log10(1.0 + clampedValue * inputGain) / std::log10(1.0 + inputGain);
    
    // 3. Возвращаем итоговое значение, жестко зафиксированное в рамках [0.0 ... 1.0]
    return std::clamp(normalized, 0.0, 1.0);
}

// BoomAnalyzer-like normalization (to 0..1 range).
float boomNormalize(float v) {
    if (v <= 0.0f) return 0.0f;
    constexpr float kLog256 = 2.40823996531f; // log10(256)
    const float h = std::log10(v * 256.0f) / (kLog256 * 1.1f);
    return std::clamp(h, 0.0f, 1.0f);
}

} // namespace

Visualizer::Visualizer(QWidget *parent)
    : QWidget(parent),
      m_player(nullptr),
      m_timer(nullptr)
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, true);

    // Pre-allocate buffers once to avoid allocations on each frame.
    m_audioData.resize(FFT_SIZE, 0.0f);
    m_barHeights.resize(64, 0.0f);
    m_peakHeights.resize(64, 0.0f);
    m_peakSpeeds.resize(64, 0.01f);
    m_smoothHeights.resize(64, 0.0f); // ensure paint path has data
    m_smearHeights.resize(64, 0.0f);  // ensure paint path has data
    ensureBarColors(64);
    m_fftComplex.resize(FFT_SIZE);
    m_fftWindow.resize(FFT_SIZE, 0.0f);
    m_rawFFTData.resize(64, 0.0f);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Visualizer::updateVisuals);
    // We do NOT start it until audio starts playing
}

Visualizer::~Visualizer() {
}

void Visualizer::playVisuals() {
    if (!m_timer->isActive()) {
        m_timer->start(33); // ~30 FPS
    }
}

void Visualizer::stopVisuals() {
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void Visualizer::setInputScale(float value) {
    m_inputScale = std::clamp(value, 0.0f, 1.0f);
}

std::size_t Visualizer::readForVisualizer(float *dst, std::size_t maxSamples) {
    if (!m_player || !m_player->isAudioPlaying()) {
        std::fill(dst, dst + maxSamples, 0.0f); 
        return 0;
    }

    return m_player->readVisualizer(dst, maxSamples);
}

void Visualizer::updateVisuals() {
    if (m_player == nullptr || m_audioData.empty()) {
        return;
    }

    if (!m_player->isAudioPlaying()) {
        // Reset AGC between tracks so each song learns its own dynamic range.
        m_dynamicMax = 0.001f;
        return;
    }

    constexpr std::size_t kFixedBarCount = 64;
    if (m_barHeights.size() != kFixedBarCount) {
        m_barHeights.resize(kFixedBarCount, 0.0f);
        m_peakHeights.resize(kFixedBarCount, 0.0f);
        m_peakSpeeds.resize(kFixedBarCount, 0.01f);
        m_smoothHeights.resize(kFixedBarCount, 0.0f);
        m_smearHeights.resize(kFixedBarCount, 0.0f);
        ensureBarColors(kFixedBarCount);
    }

    const std::size_t samplesRead = readForVisualizer(m_audioData.data(), FFT_SIZE);
    if (samplesRead == 0) {
        update();
        return;
    }

    if (samplesRead < m_audioData.size()) {
        std::fill(m_audioData.begin() + static_cast<std::ptrdiff_t>(samplesRead), m_audioData.end(), 0.0f);
    }

    if (m_inputScale <= 0.0f) {
        std::fill(m_audioData.begin(), m_audioData.end(), 0.0f);
    } else if (m_inputScale < 1.0f) {
        for (std::size_t i = 0; i < samplesRead; ++i) {
            m_audioData[i] *= m_inputScale;
        }
    }

    calculateFFT(m_audioData);

    // AGC: track current track loudness envelope and adapt quiet tracks upward.
constexpr float kMinDynamicMax = 0.08f; 
    constexpr float kAttack = 0.05f;
    constexpr float kRelease = 0.995f;

    float frameMax = 0.0f;
    for (float v : m_rawFFTData) {
        frameMax = std::max(frameMax, v);
    }

    // НЕ ОБНОВЛЯЕМ AGC,
    // ЕСЛИ СИГНАЛ ПРАКТИЧЕСКИ ОТСУТСТВУЕТ,
    // ЧТОБЫ НЕ ПОДНИМАТЬ ШУМЫ
    if (frameMax > 0.005f) {
        if (frameMax > m_dynamicMax) {
            m_dynamicMax += (frameMax - m_dynamicMax) * kAttack;
        } else {
            m_dynamicMax = std::max(kMinDynamicMax, m_dynamicMax * kRelease);
        }
    }

    const float agcGain = 1.0f / std::max(m_dynamicMax, kMinDynamicMax);
    

    const float visualHeight = static_cast<float>(std::max(1, height()));
    const float K_barDrop = 0.035f;
    const float InitialPeakSpeed = 0.018f / visualHeight;
    const float F_peakAccel = 1.18f;

    constexpr float kNoiseGateThreshold = 0.002f;

    for (std::size_t i = 0; i < kFixedBarCount; ++i) {
        float rawValue = m_rawFFTData[i];

        if (rawValue < kNoiseGateThreshold) {
            rawValue = 0.0f;
        }

        const float agcValue = std::min(rawValue * agcGain, 4.0f);
        const float targetH = boomNormalize(agcValue);

        if (targetH > m_barHeights[i]) {
            m_barHeights[i] = targetH;

            if (targetH > m_peakHeights[i]) {
                m_peakHeights[i] = targetH;
                m_peakSpeeds[i] = InitialPeakSpeed;
            }
        } else {
            if (m_barHeights[i] > 0.0f) {
                m_barHeights[i] = std::max(0.0f, m_barHeights[i] - K_barDrop);
            }

            if (m_peakHeights[i] > 0.0f) {
                m_peakHeights[i] -= m_peakSpeeds[i];
                m_peakSpeeds[i] *= F_peakAccel;
                m_peakHeights[i] = std::max(0.0f, std::max(m_peakHeights[i], m_barHeights[i]));
            }
        }

        m_smoothHeights[i] = m_barHeights[i];
        m_smearHeights[i] = m_peakHeights[i];
    }

    update();
}

void Visualizer::ensureBarColors(std::size_t barCount) {
    if (m_barColors.size() == barCount) {
        return;
    }

    m_barColors.resize(barCount);
    if (barCount == 0) {
        return;
    }

    for (std::size_t i = 0; i < barCount; ++i) {
        const qreal hue = static_cast<qreal>(i) / static_cast<qreal>(barCount);
        m_barColors[i] = QColor::fromHsvF(hue, 0.75, 1.0, 1.0);
    }
}

void Visualizer::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_smoothHeights.empty()) {
        return;
    }

    const std::size_t count = m_smoothHeights.size();
    if (count == 0) {
        return;
    }

    const qreal baseline = static_cast<qreal>(height());
    const qreal maxVisualHeight = static_cast<qreal>(height()) * 0.70;
    const qreal responseGamma = 0.88f;
    const qreal noiseGate = 0.04;
    const qreal availableWidth = static_cast<qreal>(std::max(1, width()));
    const qreal cellWidth = availableWidth / static_cast<qreal>(count);
    const qreal minBarWidth = kBarWidth;
    const qreal maxBarWidth = std::max(minBarWidth, cellWidth * 1.5);
    const qreal barSpacing = cellWidth;
    const auto barCenterX = [&](std::size_t idx) {
        return (static_cast<qreal>(idx) + 0.5) * cellWidth;
    };


    // Bars (equivalent to out_smooth lines in musializer's fft_render).
    for (std::size_t i = 0; i < count; ++i) {
        const qreal t = std::clamp(static_cast<qreal>(m_smoothHeights[i]), 0.0, 1.0);
        const qreal activeT = std::clamp((t - noiseGate) / (1.0 - noiseGate), 0.0, 1.0);
        if (activeT <= 0.0) {
            continue;
        }
        const qreal boostedT = std::clamp(std::pow(activeT, responseGamma), 0.0, 1.0);
        const QColor &baseColor = m_barColors[i];
        QColor color = baseColor;
        color.setAlphaF(0.95);

        const qreal x = barCenterX(i);
        const qreal heightPx = maxVisualHeight * boostedT;
        const qreal widthT = std::pow(boostedT, 1.8);
        const qreal dynamicBarWidth = minBarWidth + (maxBarWidth - minBarWidth) * widthT;

        const qreal yTop = baseline - heightPx;

        QPen barPen(color);
        barPen.setCapStyle(Qt::RoundCap);
        barPen.setWidthF(dynamicBarWidth);
        painter.setPen(barPen);
        painter.drawLine(QPointF(x, yTop), QPointF(x, baseline));
    }

    // Smears (between out_smear and out_smooth).
    painter.setPen(Qt::NoPen);
    for (std::size_t i = 0; i < count; ++i) {
        const qreal startRaw = std::clamp(static_cast<qreal>(m_smearHeights[i]), 0.0, 1.0);
        const qreal endRaw = std::clamp(static_cast<qreal>(m_smoothHeights[i]), 0.0, 1.0);
        const qreal startNorm = std::clamp((startRaw - noiseGate) / (1.0 - noiseGate), 0.0, 1.0);
        const qreal endNorm = std::clamp((endRaw - noiseGate) / (1.0 - noiseGate), 0.0, 1.0);
        if (endNorm <= 0.0) {
            continue;
        }
        const qreal start = std::pow(startNorm, responseGamma);
        const qreal end = std::pow(endNorm, responseGamma);
        const QColor &cachedColor = m_barColors[i];
        QColor baseColor = cachedColor;
        baseColor.setAlphaF(0.35);

        const qreal x = barCenterX(i);
        const qreal yStart = baseline - maxVisualHeight * start;
        const qreal yEnd = baseline - maxVisualHeight * end;
        const qreal top = std::min(yStart, yEnd);
        const qreal bottom = std::max(yStart, yEnd);
        const qreal radius = std::max<qreal>(2.0, barSpacing * 1.2 * std::sqrt(std::max(end, 0.0)));

        QRectF smearRect(x - radius * 0.5, top, radius, std::max<qreal>(1.0, bottom - top));
        QLinearGradient smearGrad(smearRect.topLeft(), smearRect.bottomLeft());
        smearGrad.setColorAt(0.0, QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 0));
        smearGrad.setColorAt(0.5, baseColor);
        smearGrad.setColorAt(1.0, QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 0));
        painter.setBrush(smearGrad);
        painter.drawRoundedRect(smearRect, radius * 0.3, radius * 0.3);
    }

    // Glow circles on peaks.
    painter.setCompositionMode(QPainter::CompositionMode_Screen);
    for (std::size_t i = 0; i < count; ++i) {
        const qreal t = std::clamp(static_cast<qreal>(m_smoothHeights[i]), 0.0, 1.0);
        const qreal activeT = std::clamp((t - noiseGate) / (1.0 - noiseGate), 0.0, 1.0);
        if (activeT <= 0.0) {
            continue;
        }
        const qreal boostedT = std::clamp(std::pow(activeT, responseGamma), 0.0, 1.0);
        const QColor &cachedColor = m_barColors[i];
        QColor color = cachedColor;
        color.setAlphaF(0.55);

        const qreal x = barCenterX(i);
        const qreal y = baseline - maxVisualHeight * boostedT;
        const qreal radius = std::max<qreal>(2.0, barSpacing * 1.4 * std::sqrt(boostedT));

        QRadialGradient glow(QPointF(x, y), radius);
        glow.setColorAt(0.0, color);
        glow.setColorAt(1.0, QColor(color.red(), color.green(), color.blue(), 0));
        painter.setBrush(glow);
        painter.drawEllipse(QPointF(x, y), radius, radius);

        // Inner almost-solid core, smaller than the outer glow.
        QColor coreColor = cachedColor;
        coreColor.setAlphaF(0.9);
        const qreal coreRadius = std::max<qreal>(0.9, radius * 0.38);
        painter.setBrush(coreColor);
        painter.drawEllipse(QPointF(x, y), coreRadius, coreRadius);
    }
}
void Visualizer::calculateFFT(const std::vector<float>& in_raw) {
    const std::size_t n = std::min(in_raw.size(), m_fftWindow.size());

    for (std::size_t i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(n - 1);
        float hann = 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * t);
        m_fftWindow[i] = in_raw[i] * hann;
    }

    for (std::size_t i = n; i < m_fftWindow.size(); ++i) {
        m_fftWindow[i] = 0.0f;
    }

    for (std::size_t i = 0; i < n; ++i) {
        m_fftComplex[i] = m_fftWindow[i];
    }
    for (std::size_t i = n; i < m_fftComplex.size(); ++i) {
        m_fftComplex[i] = std::complex<float>(0.0f, 0.0f);
    }

    for (std::size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(m_fftComplex[i], m_fftComplex[j]);
        }
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        float ang = 2.0f * static_cast<float>(M_PI) / static_cast<float>(len);
        std::complex<float> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; j++) {
                std::complex<float> u = m_fftComplex[i + j];
                std::complex<float> v = m_fftComplex[i + j + len / 2] * w;
                m_fftComplex[i + j] = u + v;
                m_fftComplex[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    // Return 64 bins as linear amplitudes (Boom math is applied later in updateVisuals).
    const float step = 1.06f;
    const float lowf = 1.0f;
    std::size_t m = 0;

    std::fill(m_rawFFTData.begin(), m_rawFFTData.end(), 0.0f);

    for (float f = lowf; static_cast<size_t>(f) < n / 2 && m < 64; f = std::ceil(f * step)) {
        const float f1 = std::ceil(f * step);
        float max_amp = 0.0f;

        for (size_t q = static_cast<size_t>(f); q < n / 2 && q < static_cast<size_t>(f1); ++q) {
            const float a = std::abs(m_fftComplex[q]) / 50.0f; // BoomAnalyzer scale(...)
            if (a > max_amp) max_amp = a;
        }

        m_rawFFTData[m++] = max_amp;
    }
}
void Visualizer::setAudioPlayer(AudioPlayer *player) {
    if (m_player) {
        m_player->setVisualizer(nullptr);
    }
    m_player = player;
    if (m_player) {
        m_player->setVisualizer(this);
    }
}