// src/InteractiveBackground.cpp
#include "InteractiveBackground.h"
#include "ThemeManager.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QRegion>
#include <QtMath>

InteractiveBackground::InteractiveBackground(QWidget *parent)
    : QWidget(parent) {

  // Setup animation timer (30 frames per second is fine, but we will optimize
  // repaints)
  animationTimer = new QTimer(this);
  connect(animationTimer, &QTimer::timeout, this,
          &InteractiveBackground::updateAnimation);
  animationTimer->start(33); // ~30 FPS (1000ms / 30)

  // Re-create clouds when the theme changes
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          &InteractiveBackground::refreshClouds);
}

void InteractiveBackground::initializeClouds(int width, int height) {
  clouds.clear();

  CloudThemeConfig config = ThemeManager::instance().getCloudConfig();

  int numberOfClouds = 16;
  for (int i = 0; i < numberOfClouds; ++i) {
    Cloud cloud;

    int startX = QRandomGenerator::global()->bounded(0, width);
    int startY = QRandomGenerator::global()->bounded(0, height);
    cloud.position = QPointF(startX, startY);

    double vx = -1.0 + (QRandomGenerator::global()->generateDouble() * 1.1);
    double vy = -1.0 + (QRandomGenerator::global()->generateDouble() * 1.1);
    cloud.velocity = QPointF(vx, vy);

    cloud.radius =
        QRandomGenerator::global()->bounded(config.minRadius, config.maxRadius);
    int alpha =
        QRandomGenerator::global()->bounded(config.minAlpha, config.maxAlpha);
    cloud.color = config.baseColor;
    cloud.color.setAlpha(alpha);

    clouds.append(cloud);
  }
}

void InteractiveBackground::refreshClouds() {
  // 1. Получаем конфигурацию для текущей темы из менеджера
  CloudThemeConfig config = ThemeManager::instance().getCloudConfig();

  // 2. Применяем новые параметры ко всем облакам
  for (Cloud &cloud : clouds) {
    cloud.radius =
        QRandomGenerator::global()->bounded(config.minRadius, config.maxRadius);

    int alpha =
        QRandomGenerator::global()->bounded(config.minAlpha, config.maxAlpha);

    cloud.color = config.baseColor;
    cloud.color.setAlpha(alpha); // Устанавливаем сгенерированную прозрачность
  }
}

void InteractiveBackground::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Safely avoid drawing under the CustomTitleBar (top 40 pixels)
  // Since CustomTitleBar has perfectly antialiased rounded corners via QSS,
  // drawing under it causes partial transparency overlap glitches (fringing).
  QRect bgRect = rect();
  bgRect.setTop(40);

  // Combine clip rects to only draw where needed to save CPU
  painter.setClipRegion(event->region().intersected(bgRect));

  // Draw the solid background color
  painter.fillRect(bgRect, palette().window().color());

  // Draw each cloud
  painter.setPen(Qt::NoPen);
  for (const Cloud &cloud : clouds) {
    // Only draw clouds that intersect the event region
    QRectF cloudBounds(cloud.position.x() - cloud.radius - 2,
                       cloud.position.y() - cloud.radius - 2,
                       cloud.radius * 2 + 4, cloud.radius * 2 + 4);
    if (event->region().intersects(cloudBounds.toRect())) {
      painter.setBrush(cloud.color);
      painter.drawEllipse(cloud.position, cloud.radius, cloud.radius);
    }
  }
}

void InteractiveBackground::updateAnimation() {
  QRegion dirtyRegion;

  for (int i = 0; i < clouds.size(); ++i) {
    Cloud &cloud = clouds[i];

    // Record old position for dirty region
    QRect oldRect(cloud.position.x() - cloud.radius - 2,
                  cloud.position.y() - cloud.radius - 2, cloud.radius * 2 + 4,
                  cloud.radius * 2 + 4);
    dirtyRegion += oldRect;

    // 1. Update position based on current velocity
    cloud.position += cloud.velocity;

    // 2. Boundary Collision (Bounce off walls)
    // Adjust these to account for the widget's actual size
    if (cloud.position.x() - cloud.radius < 0) {
      cloud.position.setX(cloud.radius);
      cloud.velocity.setX(qAbs(cloud.velocity.x())); // Force positive velocity
    } else if (cloud.position.x() + cloud.radius > width()) {
      cloud.position.setX(width() - cloud.radius);
      cloud.velocity.setX(-qAbs(cloud.velocity.x())); // Force negative velocity
    }

    if (cloud.position.y() - cloud.radius < 0) {
      cloud.position.setY(cloud.radius);
      cloud.velocity.setY(qAbs(cloud.velocity.y()));
    } else if (cloud.position.y() + cloud.radius > height()) {
      cloud.position.setY(height() - cloud.radius);
      cloud.velocity.setY(-qAbs(cloud.velocity.y()));
    }

    // Record new position for dirty region
    QRect newRect(cloud.position.x() - cloud.radius - 2,
                  cloud.position.y() - cloud.radius - 2, cloud.radius * 2 + 4,
                  cloud.radius * 2 + 4);
    dirtyRegion += newRect;
  }

  // Trigger a repaint to show the updated positions only in dirty regions
  update(dirtyRegion);
}