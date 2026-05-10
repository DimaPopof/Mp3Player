// src/InteractiveBackground.cpp
#include "InteractiveBackground.h"
#include "ThemeManager.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>

InteractiveBackground::InteractiveBackground(QWidget *parent)
    : QWidget(parent), isMouseOver(false) {

  // Enable mouse tracking to get coordinates even without clicks
  setMouseTracking(true);

  // Setup animation timer (30 frames per second to save CPU)
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

    double vx = -1.0 + (QRandomGenerator::global()->generateDouble() * 1.3);
    double vy = -1.0 + (QRandomGenerator::global()->generateDouble() * 1.3);
    cloud.velocity = QPointF(vx, vy);

    cloud.radius = QRandomGenerator::global()->bounded(config.minRadius, config.maxRadius);
    int alpha = QRandomGenerator::global()->bounded(config.minAlpha, config.maxAlpha);
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
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Draw the solid background color
  painter.fillRect(rect(), palette().window().color());

  // Draw each cloud
  painter.setPen(Qt::NoPen);
  for (const Cloud &cloud : clouds) {
    painter.setBrush(cloud.color);
    // Current drawing is just a simple circle.
    // This is where we will add elastic spline drawing later.
    painter.drawEllipse(cloud.position, cloud.radius, cloud.radius);
  }
}

void InteractiveBackground::updateAnimation() {
  qreal mouseRepulsionForce = 0.5;    // Strength of the mouse push
  qreal mouseDetectionRadius = 100.0; // How close the mouse needs to be

  for (int i = 0; i < clouds.size(); ++i) {
    Cloud &cloud = clouds[i];

    // 1. Update position based on current velocity
    cloud.position += cloud.velocity;

    // 2. Interaction with Mouse Cursor
    if (isMouseOver) {
      // Calculate vector from mouse to cloud center
      QPointF vectorFromMouse = cloud.position - lastMousePosition;
      qreal distanceSquared = vectorFromMouse.x() * vectorFromMouse.x() +
                              vectorFromMouse.y() * vectorFromMouse.y();

      // If the cloud is close to the mouse, apply a repulsive force
      if (distanceSquared < mouseDetectionRadius * mouseDetectionRadius) {
        qreal distance = qSqrt(distanceSquared);
        if (distance > 0.1) { // Avoid division by zero
          // Normalize the vector and scale by force (inverse distance would be
          // better)
          QPointF repulsionVector =
              (vectorFromMouse / distance) * mouseRepulsionForce;
          cloud.velocity += repulsionVector;
        }
      }
    }

    // 3. Boundary Collision (Bounce off walls)
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

    // 4. Implement some friction/speed limit to prevent infinite acceleration
    qreal currentSpeedSquared = cloud.velocity.x() * cloud.velocity.x() +
                                cloud.velocity.y() * cloud.velocity.y();
    qreal maxSpeed = 1.2;
    if (currentSpeedSquared > maxSpeed * maxSpeed) {
      qreal currentSpeed = qSqrt(currentSpeedSquared);
      cloud.velocity = (cloud.velocity / currentSpeed) * maxSpeed;
    }
  }

  // Trigger a repaint to show the updated positions
  update();
}

void InteractiveBackground::mouseMoveEvent(QMouseEvent *event) {
  lastMousePosition = event->position();
  isMouseOver = true;
  QWidget::mouseMoveEvent(event);
}

void InteractiveBackground::leaveEvent(QEvent *event) {
  isMouseOver = false;
  QWidget::leaveEvent(event);
}