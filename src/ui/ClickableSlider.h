// ClickableSlider.h
#ifndef CLICKABLESLIDER_H
#define CLICKABLESLIDER_H

#include <QMouseEvent>
#include <QPainter>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>

class ClickableSlider : public QSlider {
  Q_OBJECT

public:
  explicit ClickableSlider(Qt::Orientation orientation,
                           QWidget *parent = nullptr)
      : QSlider(orientation, parent) {}

protected:
  // Override mouse press event to jump directly to the clicked position
  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton) {
      QStyleOptionSlider opt;
      initStyleOption(&opt);

      // Calculate the exact value based on click coordinates
      int sliderLength =
          style()->pixelMetric(QStyle::PM_SliderLength, &opt, this);
      int availableWidth = width() - sliderLength;
      int clickPosition = event->pos().x() - sliderLength / 2;

      int val = QStyle::sliderValueFromPosition(
          minimum(), maximum(), clickPosition, availableWidth, opt.upsideDown);

      // Update the UI and trigger the seek signal
      setValue(val);
      emit sliderMoved(val);
    }

    // Call the base class to allow normal dragging after the click
    QSlider::mousePressEvent(event);
  }
};

#endif // CLICKABLESLIDER_H