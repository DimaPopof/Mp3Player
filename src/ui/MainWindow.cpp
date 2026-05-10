// MainWindow.cpp
#include "MainWindow.h"
#include "BottomWidget.h"
#include "InteractiveBackground.h"
#include "LeftWidget.h"
#include "RightWidget.h"
#include "ThemeManager.h"
#include "Visualizer.h"
#include "core/TrackMetadata.h"
#include "models/CustomSortProxyModel.h"
#include "models/FileBrowserModelFactory.h"
#include <QApplication>
#include <QChar>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QKeyEvent>
#include <QKeySequence>
#include <QPainter>
#include <QSettings>
#include <QShortcut>
#include <QStyle>
#include <QUrl>
#include <algorithm>
#include <cstddef>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  QCoreApplication::setOrganizationName("MySoft");
  QCoreApplication::setApplicationName("Mp3Player");

  QSettings settings;
  constantFolderPath =
      settings.value("defaultFolder", QDir::homePath()).toString();

  playlistManager = new PlaylistManager(this);
  audioPlayer = new AudioPlayer(this);

  setupUi();
  setupConnections();
  qApp->installEventFilter(this);

  applyZoom();
  changeVolume(50);
  bottomWidget->updatePlayButtonState(false);
  loadDefaultFolder();
}

void MainWindow::setupUi() {
  centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  mainLayout = new QVBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  InteractiveBackground *backgroundWidget =
      new InteractiveBackground(centralWidget);
  backgroundWidget->setObjectName("interactiveBackground");
  QVBoxLayout *backgroundLayout = new QVBoxLayout(backgroundWidget);
  backgroundLayout->setContentsMargins(0, 0, 0, 0);
  backgroundLayout->setSpacing(0);

  topSplitter = new QSplitter(Qt::Horizontal, backgroundWidget);
  topSplitter->setObjectName("topSplitter");
  topSplitter->setHandleWidth(5);

  // Instantiate the custom widgets
  leftWidget = new LeftWidget(backgroundWidget);
  leftWidget->setMinimumWidth(400);
  leftWidget->pass_AudioPlayer_To_Visualizer()->setAudioPlayer(audioPlayer);
  rightWidget = new RightWidget(backgroundWidget);
  rightWidget->setMinimumWidth(400);
  bottomWidget = new BottomWidget(centralWidget);

  // Add widgets to the splitters
  topSplitter->addWidget(leftWidget);
  topSplitter->addWidget(rightWidget);

  // Set minimum widths AFTER adding to splitter to ensure it respects them
  leftWidget->setMinimumWidth(400);
  rightWidget->setMinimumWidth(400);
  topSplitter->setMinimumWidth(800);

  topSplitter->setSizes({400, 700});
  topSplitter->setStretchFactor(0, 0); // LeftWidget stays smaller
  topSplitter->setStretchFactor(1, 1); // RightWidget takes extra space
  topSplitter->setCollapsible(0, false);
  topSplitter->setCollapsible(1, false);

  backgroundLayout->addWidget(topSplitter, 1);

  mainLayout->addWidget(backgroundWidget, 1);
  mainLayout->addWidget(bottomWidget, 0);

  centralWidget->setMinimumWidth(850);
  backgroundWidget->initializeClouds(1000, 700);
  resize(1000, 700);
  setMinimumSize(850, 500);
}
void MainWindow::setupConnections() {
  // Connect LeftWidget signals
  connect(leftWidget, &LeftWidget::openFolderRequested, this,
          &MainWindow::openFolderDialog);
  connect(leftWidget, &LeftWidget::constantFolderRequested, this,
          &MainWindow::openConstantFolder);

  // Connect BottomWidget signals
  connect(bottomWidget, &BottomWidget::playPauseClicked, this,
          &MainWindow::togglePlayback);
  connect(bottomWidget, &BottomWidget::volumeMuteClicked, this,
          &MainWindow::toggleMute);
  connect(bottomWidget, &BottomWidget::nextClicked, this,
          &MainWindow::playNextTrack);
  connect(bottomWidget, &BottomWidget::previousClicked, this,
          &MainWindow::playPreviousTrack);
  connect(bottomWidget, &BottomWidget::volumeChanged, this,
          &MainWindow::changeVolume);
  connect(bottomWidget, &BottomWidget::seekRequested, this,
          &MainWindow::seekAudio);
  connect(audioPlayer, &AudioPlayer::playbackStateChanged, bottomWidget,
          &BottomWidget::updatePlayButtonState);
  connect(audioPlayer, &AudioPlayer::bufferedAmountChanged, bottomWidget,
          &BottomWidget::updateBufferedAmount);
  connect(audioPlayer, &AudioPlayer::positionChanged, this,
          [this](qint64 positionMs) {
            qint64 durationMs = audioPlayer->durationMs();
            bottomWidget->updatePosition(positionMs, durationMs);
          });

  connect(audioPlayer, &AudioPlayer::durationChanged, bottomWidget,
          [this](qint64 durationMs) { bottomWidget->setDuration(durationMs); });
  // Connect RightWidget signals
  connect(rightWidget, &RightWidget::trackPlayRequested, this,
          &MainWindow::playTrackByPath);
  connect(rightWidget, &RightWidget::listFolderNavigationRequested, this,
          [this](const QString &folderPath) {
            this->loadListDirectory(folderPath, false);
          });
  // AudioPlayer to MainWindow logic (these stay here)
  // connect(audioPlayer, &AudioPlayer::playbackStateChanged, this,
  // &MainWindow::updatePlayButton);
  connect(audioPlayer, &AudioPlayer::durationChanged, this,
          &MainWindow::updateDuration);
  connect(audioPlayer, &AudioPlayer::positionChanged, this,
          &MainWindow::updatePosition);

  connect(audioPlayer, &AudioPlayer::trackFinished, this,
          &MainWindow::onTrackFinished);

  // Setup Shortcuts (these belong to MainWindow logic)
  QShortcut *skipForwardShortcut =
      new QShortcut(QKeySequence(Qt::Key_Right), this);
  connect(skipForwardShortcut, &QShortcut::activated, this,
          &MainWindow::skipForward);

  QShortcut *skipBackwardShortcut =
      new QShortcut(QKeySequence(Qt::Key_Left), this);
  connect(skipBackwardShortcut, &QShortcut::activated, this,
          &MainWindow::skipBackward);

  resize(1000, 700);
  setMinimumSize(600, 400);
  setWindowTitle("MP3 Player");
}

void MainWindow::loadDefaultFolder() {
  if (!constantFolderPath.isEmpty()) {
    rightWidget->setTreeRootPath(constantFolderPath);
  }
}

MainWindow::~MainWindow() {}

// Handles window resize actions to scale the picture
void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
}
void MainWindow::openFolderDialog() {
  QString folderPath = QFileDialog::getExistingDirectory(
      this, "Select MP3 Folder", QDir::homePath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (!folderPath.isEmpty()) {
    rightWidget->setTreeRootPath(folderPath);
  }
}
void MainWindow::openConstantFolder() {
  // Open dialog to let the user select a new default folder
  QString folderPath = QFileDialog::getExistingDirectory(
      this, "Select Default MP3 Folder", constantFolderPath,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  // Save and load the new folder if the user made a selection
  if (!folderPath.isEmpty()) {
    constantFolderPath = folderPath;
    bottomWidget->resetSlider();
    // Save the new path to system settings
    QSettings settings;
    settings.setValue("defaultFolder", constantFolderPath);

    rightWidget->setTreeRootPath(
        constantFolderPath); // This will update both views to match the saved
                             // path
  }
}
QString MainWindow::formatTime(int seconds) {
  int minutes = seconds / 60;
  int remainingSeconds = seconds % 60;
  return QString("%1:%2")
      .arg(minutes, 2, 10, QChar('0'))
      .arg(remainingSeconds, 2, 10, QChar('0'));
}
void MainWindow::loadListDirectory(const QString &folderPath,
                                   bool resetPlayback) {
  rightWidget->setListFolder(folderPath);
  playlistManager->loadDirectory(folderPath);

  if (resetPlayback) {
    audioPlayer->stop();
    isMediaLoaded = false;
    bottomWidget->updatePlayButtonState(false);
    bottomWidget->resetSlider();
    leftWidget->resetTrackDisplay();
  }
}
void MainWindow::onTrackFinished() {
  // 1. Получаем путь следующего трека
  QString nextPath = playlistManager->nextTrack(currentTrackPath);

  // 2. Если плейлист вернул путь (не пустой), запускаем его
  if (!nextPath.isEmpty()) {
    playTrackByPath(nextPath);
  } else {
    // Если треков больше нет, можно просто остановить плеер
    audioPlayer->stop();
  }
}
void MainWindow::updateTrackInfo(const QString &filePath) {
  TrackInfo trackInfo = TrackMetadataExtractor::extract(filePath);
  rightWidget->setPlayingTrackHighlight(filePath);
  leftWidget->updateTrackDisplay(trackInfo);
}
void MainWindow::playTrackByPath(const QString &filePath) {
  // 1. Store the current path so Next/Previous buttons know where to start
  currentTrackPath = filePath;

  // Extract the folder path and load all MP3s into the manager
  QFileInfo fileInfo(filePath);
  QString folderPath = fileInfo.absolutePath();

  if (playlistManager) {
    playlistManager->loadDirectory(folderPath);
  }

  // Delegate UI updates to RightWidget
  if (filePath.startsWith(treeCurrentRootPath)) {
    rightWidget->selectFileInTree(filePath);
  }

  // Manage audio playback logic
  audioPlayer->stop();
  QUrl fileUrl = QUrl::fromLocalFile(filePath);
  audioPlayer->loadMedia(fileUrl);
  bottomWidget->resetSlider();

  // Apply the stored volume state instead of reading the UI slider
  changeVolume(
      bottomWidget->getVolumeValue()); // This will ensure the audio player is
                                       // in sync with the current volume level,
                                       // including mute state

  updateTrackInfo(filePath);

  audioPlayer->play();
  isMediaLoaded = true;
}
void MainWindow::togglePlayback() {
  if (!isMediaLoaded) {
    QStringList tracks = playlistManager->getTrackList();
    if (tracks.isEmpty()) {
      return;
    }
    // Play the first track in the folder
    playTrackByPath(tracks.first());
  } else {
    if (audioPlayer->isAudioPlaying()) {
      audioPlayer->pause();
    } else {
      audioPlayer->play();
    }
  }
}

void MainWindow::playNextTrack() {
  if (currentTrackPath.isEmpty()) {
    return;
  }

  QString nextTrackPath = playlistManager->nextTrack(currentTrackPath);
  if (!nextTrackPath.isEmpty()) {
    playTrackByPath(nextTrackPath);
  }
}

void MainWindow::playPreviousTrack() {
  if (currentTrackPath.isEmpty()) {
    return;
  }

  QString previousTrackPath = playlistManager->previousTrack(currentTrackPath);
  if (!previousTrackPath.isEmpty()) {
    playTrackByPath(previousTrackPath);
  }
}
void MainWindow::changeVolume(int value) {
  // 1. Управляем аудио-движком
  float volumeLevel = static_cast<float>(value) / 100.0f;
  audioPlayer->setVolume(volumeLevel);

  // 2. Обновляем состояние и интерфейс
  if (value == 0) {
    isMuted = true;
    bottomWidget->updateVolumeIcon(true);
  } else {
    if (isMuted) {
      savedVolume = value;
      isMuted = false;
    }
    bottomWidget->updateVolumeIcon(false);
  }
}

void MainWindow::updateDuration(qint64 durationMs) {
  currentDurationMs = durationMs;
  bottomWidget->setDuration(durationMs);
}

void MainWindow::updatePosition(qint64 positionMs) {
  bottomWidget->updatePosition(positionMs, currentDurationMs);
}

void MainWindow::seekAudio(int position) { audioPlayer->setPosition(position); }
void MainWindow::skipForward() {
  if (!isMediaLoaded) {
    return;
  }

  qint64 currentPosMs = audioPlayer->positionMs();

  qint64 skipAmountMs = 15000;
  qint64 newPositionMs = currentPosMs + skipAmountMs;

  if (newPositionMs > currentDurationMs) {
    newPositionMs = currentDurationMs;
  }

  seekAudio(newPositionMs / 1000); // audioPlayer still seeks in seconds
}

void MainWindow::skipBackward() {
  if (!isMediaLoaded) {
    return;
  }

  qint64 currentPosMs = audioPlayer->positionMs();

  qint64 skipAmountMs = 15000;
  qint64 newPositionMs = currentPosMs - skipAmountMs;

  if (newPositionMs < 0) {
    newPositionMs = 0;
  }

  seekAudio(newPositionMs / 1000);
}
void MainWindow::toggleMute() {
  if (isMuted) {
    isMuted = false;
    bottomWidget->setVolumeValue(savedVolume);
    changeVolume(savedVolume);

  } else {
    // --- ВЫКЛЮЧАЕМ ЗВУК (MUTE) ---
    isMuted = true;

    // 1. Запоминаем текущую громкость, спросив ее у интерфейса
    savedVolume = bottomWidget->getVolumeValue();

    // 2. Двигаем ползунок в 0 визуально
    bottomWidget->setVolumeValue(0);

    // 3. Вызываем наш метод changeVolume.
    // Он сам обновит audioPlayer и вызовет bottomWidget->updateVolumeIcon(true)
    changeVolume(0);
  }
}
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    const bool isCtrlPressed =
        keyEvent->modifiers().testFlag(Qt::ControlModifier);

    if (isCtrlPressed) {
      if (keyEvent->key() == Qt::Key_Equal || keyEvent->key() == Qt::Key_Plus) {
        zoomInText();
        return true;
      }

      if (keyEvent->key() == Qt::Key_Minus ||
          keyEvent->key() == Qt::Key_Underscore) {
        zoomOutText();
        return true;
      }
    }
  }

  return QMainWindow::eventFilter(watched, event);
}
void MainWindow::applyZoom() {
  baseFontSize = std::clamp(baseFontSize, 8, 32);

  QFont newFont = qApp->font();
  newFont.setPointSize(baseFontSize);
  qApp->setFont(newFont);

  rightWidget->setFont(newFont);
  leftWidget->setFont(newFont);
  bottomWidget->setFont(newFont);

  rightWidget->applySize(baseFontSize);
  leftWidget->applyFontSize(baseFontSize);

  // Force a stylesheet reload so that QSS parses sizes using the new font
  // metrics
  ThemeManager::instance().setTheme(ThemeManager::instance().currentTheme());
}
void MainWindow::zoomInText() {

  if (baseFontSize < 32) {
    ++baseFontSize;
    applyZoom();
  }
}

void MainWindow::zoomOutText() {
  if (baseFontSize > 8) {
    --baseFontSize;
    applyZoom();
  }
}