#pragma once
#include "MarqueeLabel.h"
#include "Visualizer.h"
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <core/TrackMetadata.h>


class Visualizer;

class LeftWidget : public QWidget {
  Q_OBJECT

public:
  explicit LeftWidget(QWidget *parent = nullptr);
  void applyFontSize(int pointSize);
  void setMetadata(const QString &text);
  void setCover(const QPixmap &pixmap);
  void resetTrackDisplay();
  void updateTrackDisplay(const TrackInfo &trackInfo);

  Visualizer *pass_AudioPlayer_To_Visualizer() const { return visualizer; }
signals:
  void openFolderRequested();

private:
  QPushButton *openFolderButton;
  QLabel *coverLabel;
  MarqueeLabel *metadataLabel;
  QPixmap currentCoverPixmap;
  QVBoxLayout *leftLayout;
  QVBoxLayout *trackInfoLayout;
  QWidget *trackInfoContainer;
  Visualizer *visualizer;

  void setupUi();
  void setupConnections();
  void updateCoverImage();
  void updateTrackInfoContainerHeight();

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
};