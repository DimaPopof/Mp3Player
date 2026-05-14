#include "Visualizer.h"

#include "ThemeManager.h"
#include "core/AudioPlayer.h"
#include <QAction>
#include <QColor>
#include <QContextMenuEvent>
#include <QDebug>
#include <QHideEvent>
#include <QLinearGradient>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QShowEvent>
#include <algorithm>
#include <cmath>
#include <complex>

namespace {

constexpr qreal kBarWidth = 2.0;
constexpr qreal kBarGap = 6.0;
constexpr qreal kBarSpacing = kBarWidth + kBarGap;

qreal compressAmplitude(qreal value) {
  // Усилитель входного сигнала (чувствительность).
  // Если полоски слишком низкие — увеличь это число (например, до 15.0
  // или 20.0).
  constexpr qreal inputGain = 15.0;

  // 1. Ограничиваем входящее значение от 0.0 до 1.0 (чтобы не было ошибок
  // логарифма)
  const qreal clampedValue = std::clamp(value, 0.0, 1.0);

  // 2. Сама логарифмическая компрессия
  const qreal normalized =
      std::log10(1.0 + clampedValue * inputGain) / std::log10(1.0 + inputGain);

  // 3. Возвращаем итоговое значение, жестко зафиксированное в рамках [0.0
  // ... 1.0]
  return std::clamp(normalized, 0.0, 1.0);
}

// BoomAnalyzer-like normalization (to 0..1 range).
float boomNormalize(float v) {
  if (v <= 0.0f)
    return 0.0f;
  constexpr float kLog256 = 2.40823996531f; // log10(256)
  const float h = std::log10(v * 256.0f) / (kLog256 * 1.1f);
  return std::clamp(h, 0.0f, 1.0f);
}

} // namespace

Visualizer::Visualizer(QWidget *parent)
    : QWidget(parent), m_player(nullptr), m_timer(nullptr) {
  setAutoFillBackground(false);
  setAttribute(Qt::WA_StyledBackground, true);
  setAttribute(Qt::WA_OpaquePaintEvent, true);

  // Pre-allocate buffers once to avoid allocations on each frame.
  m_audioData.resize(FFT_SIZE, 0.0f);
  m_barHeights.resize(64, 0.0f);
  m_peakHeights.resize(64, 0.0f);
  m_peakSpeeds.resize(64, 0.01f);
  m_smoothHeights.resize(64, 0.0f); // ensure paint path has data
  ensureBarColors(64);
  m_fftComplex.resize(FFT_SIZE);
  m_rawFFTData.resize(64, 0.0f);

  m_bitReverse.resize(FFT_SIZE);
  for (std::size_t i = 1, j = 0; i < FFT_SIZE; i++) {
    size_t bit = FFT_SIZE >> 1;
    for (; j & bit; bit >>= 1)
      j ^= bit;
    j ^= bit;
    m_bitReverse[i] = j;
  }
  m_bitReverse[0] = 0;

  m_hannWindowFixed.resize(FFT_SIZE);
  for (std::size_t i = 0; i < FFT_SIZE; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(FFT_SIZE - 1);
    m_hannWindowFixed[i] =
        0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * t);
  }

  m_twiddleFactors.reserve(FFT_SIZE);
  for (size_t len = 2; len <= FFT_SIZE; len <<= 1) {
    for (size_t j = 0; j < len / 2; j++) {
      float ang = 2.0f * static_cast<float>(M_PI) * static_cast<float>(j) /
                  static_cast<float>(len);
      m_twiddleFactors.push_back(
          std::complex<float>(std::cos(ang), std::sin(ang)));
    }
  }

  m_timer = new QTimer(this);
  m_timer->setTimerType(Qt::PreciseTimer);
  connect(m_timer, &QTimer::timeout, this, &Visualizer::updateVisuals);

  // Initialize Math LUTs
  m_gammaLUT.resize(1024);
  m_widthLUT.resize(1024);
  for (int i = 0; i < 1024; ++i) {
    float t = i / 1023.0f;
    m_gammaLUT[i] = std::pow(t, 0.88f);
    m_widthLUT[i] = std::pow(t, 1.8f);
  }

  m_timer->start(16); // Start right away for idle animation
}

Visualizer::~Visualizer() {}

void Visualizer::playVisuals() {
  if (!m_timer->isActive()) {
    m_timer->start(16); // ~60 FPS
  }
}

void Visualizer::stopVisuals() {
  // We keep the timer running to allow the bars to drop naturally
  // and to run the idle wave animation.
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
  bool isPlaying = m_player && m_player->isAudioPlaying();

  if (!isPlaying) {
    // Reset AGC between tracks so each song learns its own dynamic range.
    m_dynamicMax = 0.001f;
    m_idleFrames++;
    if (m_idleFrames >
        180) { // After ~3 seconds of silence, start idle animation
      if (m_idleFade < 1.0f) {
        m_idleFade += 0.006f; // Takes ~1.6 seconds to fully arise
        if (m_idleFade > 1.0f) {
          m_idleFade = 1.0f;
        }
      }
      if (m_timer->interval() != 33) {
        m_timer->setInterval(33); // Drop to 30 FPS to save CPU when idle
      }
      wave_animation();
      update();
      return;
    }
  } else {
    m_idleFrames = 0;
    m_idleFade = 0.0f;
    if (m_timer->interval() != 16) {
      m_timer->setInterval(16); // Back to ~60 FPS when playing
    }
  }

  constexpr std::size_t kFixedBarCount = 64;
  if (m_barHeights.size() != kFixedBarCount) {
    m_barHeights.resize(kFixedBarCount, 0.0f);
    m_peakHeights.resize(kFixedBarCount, 0.0f);
    m_peakSpeeds.resize(kFixedBarCount, 0.01f);
    m_smoothHeights.resize(kFixedBarCount, 0.0f);
    ensureBarColors(kFixedBarCount);
  }

  // 1. Читаем новые семплы во временный буфер
  std::vector<float> newSamples(FFT_SIZE);
  const std::size_t samplesRead =
      readForVisualizer(newSamples.data(), FFT_SIZE);

  if (samplesRead > 0) {
    // 2. Сдвигаем старые данные влево, чтобы освободить место справа
    std::copy(m_audioData.begin() + samplesRead, m_audioData.end(),
              m_audioData.begin());

    // 3. Копируем новые семплы в конец нашего окна
    std::copy(newSamples.begin(), newSamples.begin() + samplesRead,
              m_audioData.end() - samplesRead);
  }

  if (m_inputScale <= 0.0f || !isPlaying) {
    std::fill(m_audioData.begin(), m_audioData.end(), 0.0f);
  } else if (m_inputScale < 1.0f && samplesRead > 0) {
    for (std::size_t i = m_audioData.size() - samplesRead;
         i < m_audioData.size(); ++i) {
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
  const float K_barDrop = 0.018f;
  const float InitialPeakSpeed = 0.009f / visualHeight;
  const float F_peakAccel = 1.09f;

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
        m_peakHeights[i] =
            std::max(0.0f, std::max(m_peakHeights[i], m_barHeights[i]));
      }
    }

    // 1. Применяем EMA (Экспоненциальное сглаживание) для плавного движения
    // полосок
    const float currentH = m_smoothHeights[i];
    const float smoothTargetH = m_barHeights[i];

    if (smoothTargetH > currentH) {
      // Быстрый подъем (почти моментально реагирует на скачки)
      m_smoothHeights[i] = currentH + (smoothTargetH - currentH) * 0.8f;
    } else {
      // Плавный спад (с задержкой опускается вниз)
      m_smoothHeights[i] = currentH + (smoothTargetH - currentH) * 0.3f;
    }
  }

  update();
}

void Visualizer::ensureBarColors(std::size_t barCount) {
  if (m_cachedColors.size() == barCount) {
    return;
  }

  m_cachedColors.resize(barCount);
  if (barCount == 0) {
    return;
  }

  for (std::size_t i = 0; i < barCount; ++i) {
    const qreal hue = static_cast<qreal>(i) / static_cast<qreal>(barCount);
    QColor baseColor = QColor::fromHsvF(hue, 0.75, 1.0, 1.0);

    BarColors &colors = m_cachedColors[i];

    colors.barPenColor = baseColor;
    colors.barPenColor.setAlphaF(0.95);

    colors.smearCenterColor = baseColor;
    colors.smearCenterColor.setAlphaF(0.35);

    colors.smearEdgeColor =
        QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 0);

    colors.glowCenterColor = baseColor;
    colors.glowCenterColor.setAlphaF(0.55);

    colors.glowEdgeColor =
        QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 0);

    colors.coreColor = baseColor;
    colors.coreColor.setAlphaF(0.9);

    colors.glowPixmap = QPixmap(64, 64);
    colors.glowPixmap.fill(Qt::transparent);
    QPainter p(&colors.glowPixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    QRadialGradient glow(32, 32, 32);
    glow.setColorAt(0.0, colors.glowCenterColor);
    glow.setColorAt(1.0, colors.glowEdgeColor);
    p.setBrush(glow);
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, 64, 64);
  }
}

void Visualizer::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const bool isIdle = !m_player || !m_player->isAudioPlaying();
  QColor idleColor;
  if (isIdle) {
    idleColor = ThemeManager::instance().getVisualizerIdleColor();
    if (idleColor != m_lastIdleColor || m_idleGlowPixmap.isNull()) {
      m_lastIdleColor = idleColor;
      m_idleGlowPixmap = QPixmap(64, 64);
      m_idleGlowPixmap.fill(Qt::transparent);
      QPainter p(&m_idleGlowPixmap);
      p.setRenderHint(QPainter::Antialiasing, true);
      QRadialGradient glow(32, 32, 32);
      glow.setColorAt(0.0, idleColor);
      glow.setColorAt(
          1.0, QColor(idleColor.red(), idleColor.green(), idleColor.blue(), 0));
      p.setBrush(glow);
      p.setPen(Qt::NoPen);
      p.drawEllipse(0, 0, 64, 64);
    }
  }

  if (m_smoothHeights.empty()) {
    return;
  }

  const std::size_t count = m_smoothHeights.size();
  if (count == 0) {
    return;
  }

  const qreal baseline = static_cast<qreal>(height());
  const qreal maxVisualHeight =
      static_cast<qreal>(height() * 0.85); // Увеличено до 100% высоты
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
  if (m_enableBars) {
    for (std::size_t i = 0; i < count; ++i) {
      const qreal t =
          std::clamp(static_cast<qreal>(m_smoothHeights[i]), 0.0, 1.0);
      const qreal activeT =
          std::clamp((t - noiseGate) / (1.0 - noiseGate), 0.0, 1.0);
      if (activeT <= 0.0) {
        continue;
      }
      int gammaIdx = static_cast<int>(activeT * 1023.0f);
      if (gammaIdx < 0)
        gammaIdx = 0;
      if (gammaIdx > 1023)
        gammaIdx = 1023;
      const qreal boostedT = m_gammaLUT[gammaIdx];

      const QColor &color = isIdle ? idleColor : m_cachedColors[i].barPenColor;

      const qreal x = barCenterX(i);
      const qreal heightPx = maxVisualHeight * boostedT;

      int widthIdx = static_cast<int>(boostedT * 1023.0f);
      if (widthIdx < 0)
        widthIdx = 0;
      if (widthIdx > 1023)
        widthIdx = 1023;
      const qreal widthT = m_widthLUT[widthIdx];

      const qreal dynamicBarWidth =
          minBarWidth + (maxBarWidth - minBarWidth) * widthT;

      const qreal yTop = baseline - heightPx;

      QPen barPen(color);
      barPen.setCapStyle(Qt::RoundCap);
      barPen.setWidthF(dynamicBarWidth);
      painter.setPen(barPen);
      painter.drawLine(QPointF(x, yTop), QPointF(x, baseline));
    }
  }

  // Glow circles on peaks.
  if (m_enableGlow) {
    if (!isIdle) {
      painter.setCompositionMode(QPainter::CompositionMode_Screen);
    }
    for (std::size_t i = 0; i < count; ++i) {
      const qreal t =
          std::clamp(static_cast<qreal>(m_smoothHeights[i]), 0.0, 1.0);
      const qreal activeT =
          std::clamp((t - noiseGate) / (1.0 - noiseGate), 0.0, 1.0);
      if (activeT <= 0.0) {
        continue;
      }
      int gammaIdx = static_cast<int>(activeT * 1023.0f);
      if (gammaIdx < 0)
        gammaIdx = 0;
      if (gammaIdx > 1023)
        gammaIdx = 1023;
      const qreal boostedT = m_gammaLUT[gammaIdx];

      const qreal x = barCenterX(i);
      const qreal y = baseline - maxVisualHeight * boostedT;
      const qreal radius =
          std::max<qreal>(2.0, barSpacing * 1.4 * std::sqrt(boostedT));

      const QPixmap &pixmap =
          isIdle ? m_idleGlowPixmap : m_cachedColors[i].glowPixmap;
      QRectF destRect(x - radius, y - radius, radius * 2.0, radius * 2.0);
      painter.drawPixmap(destRect, pixmap, pixmap.rect());

      // Inner almost-solid core, smaller than the outer glow.
      const QColor &coreColor =
          isIdle ? idleColor : m_cachedColors[i].coreColor;
      const qreal coreRadius = std::max<qreal>(0.9, radius * 0.38);
      painter.setBrush(coreColor);
      painter.setPen(Qt::NoPen);
      painter.drawEllipse(QPointF(x, y), coreRadius, coreRadius);
    }
  }
}
void Visualizer::calculateFFT(const std::vector<float> &in_raw) {
  const std::size_t n = std::min(in_raw.size(), m_hannWindowFixed.size());

  for (std::size_t i = 0; i < n; ++i) {
    m_fftComplex[i] =
        std::complex<float>(in_raw[i] * m_hannWindowFixed[i], 0.0f);
  }
  for (std::size_t i = n; i < FFT_SIZE; ++i) {
    m_fftComplex[i] = std::complex<float>(0.0f, 0.0f);
  }

  for (std::size_t i = 0; i < n; ++i) {
    if (i < m_bitReverse[i]) {
      std::swap(m_fftComplex[i], m_fftComplex[m_bitReverse[i]]);
    }
  }

  size_t twiddleIdx = 0;
  for (size_t len = 2; len <= n; len <<= 1) {
    size_t halfLen = len / 2;
    for (size_t i = 0; i < n; i += len) {
      for (size_t j = 0; j < halfLen; j++) {
        std::complex<float> u = m_fftComplex[i + j];
        std::complex<float> v =
            m_fftComplex[i + j + halfLen] * m_twiddleFactors[twiddleIdx + j];
        m_fftComplex[i + j] = u + v;
        m_fftComplex[i + j + halfLen] = u - v;
      }
    }
    twiddleIdx += halfLen;
  }

  // Return 64 bins as linear amplitudes (Boom math is applied later in
  // updateVisuals).
  const float step = 1.06f;
  const float lowf = 1.0f;
  std::size_t m = 0;

  std::fill(m_rawFFTData.begin(), m_rawFFTData.end(), 0.0f);

  for (float f = lowf; static_cast<size_t>(f) < n / 2 && m < 64;
       f = std::ceil(f * step)) {
    const float f1 = std::ceil(f * step);
    float max_amp = 0.0f;

    for (size_t q = static_cast<size_t>(f);
         q < n / 2 && q < static_cast<size_t>(f1); ++q) {
      const float a =
          std::abs(m_fftComplex[q]) / 50.0f; // BoomAnalyzer scale(...)
      if (a > max_amp)
        max_amp = a;
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

void Visualizer::wave_animation() {
  constexpr std::size_t kFixedBarCount = 64;
  if (m_barHeights.size() != kFixedBarCount) {
    m_barHeights.resize(kFixedBarCount, 0.0f);
    m_peakHeights.resize(kFixedBarCount, 0.0f);
    m_peakSpeeds.resize(kFixedBarCount, 0.01f);
    m_smoothHeights.resize(kFixedBarCount, 0.0f);
    ensureBarColors(kFixedBarCount);
  }

  // Slow breathing speed for time t
  // Let m_wavePhase be t in radians, from 0 to 2*PI
  m_wavePhase += 0.015f; // Adjust speed
  if (m_wavePhase >= 2.0f * M_PI) {
    m_wavePhase -= 2.0f * static_cast<float>(M_PI);
  }

  const float timeCos = std::cos(m_wavePhase);

  for (std::size_t i = 0; i < kFixedBarCount; ++i) {
    // x goes from 0.0 to 1.0 across the bars
    float x = static_cast<float>(i) / static_cast<float>(kFixedBarCount - 1);
    float spaceCos = std::cos(2.0f * static_cast<float>(M_PI) * x);

    // Standing wave equation:
    // When timeCos = 1: peaks at edges (1.0), trough in middle (0.2)
    // When timeCos = -1: peak in middle (1.0), troughs at edges (0.2)
    float h = (0.675f + 0.325f * spaceCos * timeCos) * m_idleFade;

    m_barHeights[i] = h;
    m_smoothHeights[i] = h;
    m_peakHeights[i] = h;
  }
}

void Visualizer::hideEvent(QHideEvent *event) {
  // Когда окно сворачивается или скрывается, полностью глушим таймер отрисовки!
  if (m_timer->isActive()) {
    m_timer->stop();
  }
  // Обязательно вызываем базовый метод
  QWidget::hideEvent(event);
}

void Visualizer::showEvent(QShowEvent *event) {
  // Когда окно снова появляется на экране, запускаем таймер
  if (!m_timer->isActive()) {
    // Проверяем, играет ли музыка, чтобы задать правильный FPS
    bool isPlaying = m_player && m_player->isAudioPlaying();
    if (isPlaying) {
      m_timer->start(16); // 60 FPS
    } else {
      m_timer->start(33); // 30 FPS для холостого хода
    }
  }
  QWidget::showEvent(event);
}

void Visualizer::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);

  QAction *glowAct = menu.addAction("Включить Glow");
  glowAct->setCheckable(true);
  glowAct->setChecked(m_enableGlow);

  // Соединяем клик с изменением переменной
  connect(glowAct, &QAction::triggered, [this](bool checked) {
    m_enableGlow = checked;
    update();
  });

  menu.exec(event->globalPos());
}