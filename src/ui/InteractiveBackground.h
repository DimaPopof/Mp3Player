// src/InteractiveBackground.h
#ifndef INTERACTIVEBACKGROUND_H
#define INTERACTIVEBACKGROUND_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QTimer>
#include <QPaintEvent>

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
    void initializeClouds(int width, int height);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateAnimation(); // Called by the timer to update physics
    void refreshClouds();   // Re-creates clouds with current theme colors

private:
    QVector<Cloud> clouds;          // List of all cloud objects
    QTimer *animationTimer;        // Drives the animation loop (e.g., 60 FPS)

};

#endif // INTERACTIVEBACKGROUND_H