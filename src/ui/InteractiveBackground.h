// src/InteractiveBackground.h
#ifndef INTERACTIVEBACKGROUND_H
#define INTERACTIVEBACKGROUND_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QTimer>
#include <QPaintEvent>
#include <QMouseEvent>

// Structure to represent a single moving cloud
struct Cloud {
    QPointF position; // Current center of the cloud
    QPointF velocity; // Movement vector (pixels per frame)
    qreal radius;     // Size of the cloud
    QColor color;     // Cloud color (with alpha)
};

class InteractiveBackground : public QWidget {
    Q_OBJECT

public:
    explicit InteractiveBackground(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    // We also need to track when the mouse leaves the widget
    void leaveEvent(QEvent *event) override;

private slots:
    void updateAnimation(); // Called by the timer to update physics

private:
    QVector<Cloud> clouds;          // List of all cloud objects
    QTimer *animationTimer;        // Drives the animation loop (e.g., 60 FPS)
    QPointF lastMousePosition;    // Tracks the current cursor location
    bool isMouseOver;              // True if the cursor is inside this widget

    void initializeClouds();       // Creates initial cloud objects
};

#endif // INTERACTIVEBACKGROUND_H