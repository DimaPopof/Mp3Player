#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <core/TrackMetadata.h>

class InteractiveBackground; // Forward declaration

class LeftWidget : public QWidget { 
    Q_OBJECT

public:
    explicit LeftWidget(QWidget *parent = nullptr);
    void applyFontSize(int pointSize);
    void setMetadata(const QString &text);
    void setCover(const QPixmap &pixmap);
    void resetTrackDisplay();
    void updateTrackDisplay(const TrackInfo &trackInfo);

signals:
    void openFolderRequested();
    void constantFolderRequested();   

private:
    QPushButton *openFolderButton;
    QPushButton *constantFolderButton;
    QLabel *coverLabel;
    QLabel *metadataLabel;
    QPixmap currentCoverPixmap;

    void setupUi();
    void setupConnections();
    void updateCoverImage();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};