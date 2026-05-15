// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QCloseEvent>
#include <QMainWindow>
#include <QPixmap>
#include <QResizeEvent>
#include <QShortcut>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <deque>

#include "HighlightDelegate.h"
#include <QAbstractItemModel>
#include <QString>

#include "InteractiveBackground.h"
#include "core/AudioPlayer.h"
#include "core/PlaylistManager.h"

class LeftWidget;
class RightWidget;
class BottomWidget;
class CustomTitleBar;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  void resizeEvent(QResizeEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void openFolderDialog();
  void toggleViewMode();

  void togglePlayback();
  void toggleMute();

  void playNextTrack();
  void playPreviousTrack();
  void onTrackFinished();
  void changeVolume(int value);

  void updateDuration(qint64 durationMs);
  void updatePosition(qint64 positionMs);
  void seekAudio(int position);

  void skipForward();
  void skipBackward();

  void zoomInText();
  void zoomOutText();

  // Context menu stubs
  void onEnqueueNextRequested(const QString &filePath);
  void onAddToQueueRequested(const QString &filePath);
  void onQueueTrackPlayRequested(int index);
  void onOpenFileLocationRequested(const QString &filePath);
  void onTrackManualPlayRequested(const QString &filePath);
  void onFileRenamed(const QString &oldPath, const QString &newPath);

  // Repeat / Shuffle
  void onRepeatToggled(bool enabled) { m_isRepeat = enabled; }
  void onShuffleToggled(bool enabled) { m_isShuffle = enabled; }

private:
  HighlightDelegate *highlightDelegate;
  PlaylistManager *playlistManager;
  AudioPlayer *audioPlayer;

  bool isMediaLoaded{false};
  qint64 currentDurationMs{0};
  int baseFontSize{14};
  QString constantFolderPath;
  QString treeCurrentRootPath{QDir::homePath()};
  QString listCurrentRootPath{QDir::homePath()};
  bool isMuted{false};
  int savedVolume{35};
  bool m_isRepeat{false};
  bool m_isShuffle{false};

  QString currentRootPath;
  QString currentTrackPath;

  QPixmap currentCoverPixmap;

  QString formatTime(int seconds);
  void setupUi();
  void setupConnections();
  void loadDefaultFolder();
  void loadListDirectory(const QString &folderPath, bool resetPlayback);
  void updateTrackInfo(const QString &filePath);
  void applyZoom();
  void playTrackByPath(const QString &filePath);
  void advanceToNextTrack(bool autoAdvance);

  // --- Блок UI элементов ---

  QWidget *centralWidget;
  QVBoxLayout *mainLayout;
  QSplitter *topSplitter;

  LeftWidget *leftWidget;
  RightWidget *rightWidget;
  BottomWidget *bottomWidget;
  CustomTitleBar *titleBar;

  QShortcut *enterShortcut;
  QShortcut *returnShortcut;
};

#endif // MAINWINDOW_H