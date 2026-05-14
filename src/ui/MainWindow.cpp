// MainWindow.cpp
#include "MainWindow.h"
#include "BottomWidget.h"
#include "CustomTitleBar.h"
#include "InteractiveBackground.h"
#include "LeftWidget.h"
#include "RightWidget.h"
#include "ThemeManager.h"
#include "Visualizer.h"
#include "core/TrackMetadata.h"
#include <QApplication>
#include <QChar>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QKeyEvent>
#include <QKeySequence>
#include <QPainter>
#include <QProcess>
#include <QSettings>
#include <QShortcut>
#include <QStyle>
#include <QUrl>
#include <algorithm>

#if defined(Q_OS_WIN)
#include <objbase.h>
#include <shlobj.h>
#endif

#if defined(Q_OS_WIN)
namespace {
bool revealFileInExplorer(const QString &filePath) {
  const QFileInfo fileInfo(filePath);
  if (!fileInfo.exists()) {
    return false;
  }

  const QString folderPath = QDir::toNativeSeparators(fileInfo.absolutePath());
  const QString nativePath = QDir::toNativeSeparators(fileInfo.absoluteFilePath());

  const HRESULT coInitResult =
      CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  const bool didInitializeCom = SUCCEEDED(coInitResult) || coInitResult == S_FALSE;

  PIDLIST_ABSOLUTE folderPidl = nullptr;
  PIDLIST_ABSOLUTE filePidl = nullptr;

  const HRESULT folderResult = SHParseDisplayName(
      reinterpret_cast<LPCWSTR>(folderPath.utf16()), nullptr, &folderPidl, 0,
      nullptr);
  const HRESULT fileResult = SHParseDisplayName(
      reinterpret_cast<LPCWSTR>(nativePath.utf16()), nullptr, &filePidl, 0,
      nullptr);

  if (FAILED(folderResult) || FAILED(fileResult) || !folderPidl || !filePidl) {
    if (filePidl) {
      CoTaskMemFree(filePidl);
    }
    if (folderPidl) {
      CoTaskMemFree(folderPidl);
    }
    if (didInitializeCom) {
      CoUninitialize();
    }
    return false;
  }

  PCUITEMID_CHILD childPidl = ILFindLastID(filePidl);
  const HRESULT selectResult =
      childPidl ? SHOpenFolderAndSelectItems(folderPidl, 1, &childPidl, 0)
                : E_FAIL;

  CoTaskMemFree(filePidl);
  CoTaskMemFree(folderPidl);
  if (didInitializeCom) {
    CoUninitialize();
  }
  return SUCCEEDED(selectResult);
}
} // namespace
#endif

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
  setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
  setAttribute(Qt::WA_TranslucentBackground);

  centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  mainLayout = new QVBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  InteractiveBackground *backgroundWidget = new InteractiveBackground(centralWidget);
  backgroundWidget->setObjectName("interactiveBackground");
  QVBoxLayout *backgroundLayout = new QVBoxLayout(backgroundWidget);
  backgroundLayout->setContentsMargins(0, 0, 0, 0);
  backgroundLayout->setSpacing(0);

  titleBar = new CustomTitleBar(backgroundWidget);
  backgroundLayout->addWidget(titleBar, 0);

  topSplitter = new QSplitter(Qt::Horizontal, backgroundWidget);
  topSplitter->setObjectName("topSplitter");
  topSplitter->setHandleWidth(10);
  topSplitter->setStyleSheet("QSplitter::handle { background: transparent; }");

  // Instantiate the custom widgets
  QWidget *leftContainer = new QWidget(backgroundWidget);
  QVBoxLayout *leftContainerLayout = new QVBoxLayout(leftContainer);
  leftContainerLayout->setContentsMargins(10, 10, 0,
                                          10); // left=10, top=10, bottom=10
  leftContainerLayout->setSpacing(0);

  leftWidget = new LeftWidget(leftContainer);
  leftWidget->pass_AudioPlayer_To_Visualizer()->setAudioPlayer(audioPlayer);
  
  leftContainerLayout->addWidget(leftWidget);
  // Removed container minimum width so QSplitter calculates exactly based on LeftWidget + margins

  QWidget *rightContainer = new QWidget(backgroundWidget);
  QVBoxLayout *rightContainerLayout = new QVBoxLayout(rightContainer);
  rightContainerLayout->setContentsMargins(0, 10, 10,
                                            10); // top=10, right=10, bottom=10
  rightContainerLayout->setSpacing(0);

  rightWidget = new RightWidget(rightContainer);

  rightContainerLayout->addWidget(rightWidget);
  // Removed container minimum width so QSplitter calculates exactly based on RightWidget + margins

  bottomWidget = new BottomWidget(centralWidget);

  topSplitter->addWidget(leftContainer);
  topSplitter->addWidget(rightContainer);
  topSplitter->setMinimumWidth(800);

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
          &MainWindow::onTrackManualPlayRequested);
  connect(rightWidget, &RightWidget::listFolderNavigationRequested, this,
          [this](const QString &folderPath) {
            this->loadListDirectory(folderPath, false);
          });

  // Context menu signals
  connect(rightWidget, &RightWidget::playNextRequested, this,
          &MainWindow::onPlayNextRequested);
  connect(rightWidget, &RightWidget::addToQueueRequested, this,
          &MainWindow::onAddToQueueRequested);
  connect(rightWidget, &RightWidget::queueTrackPlayRequested, this,
          &MainWindow::onQueueTrackPlayRequested);
  connect(rightWidget, &RightWidget::openFileLocationRequested, this,
          &MainWindow::onOpenFileLocationRequested);
  // Keep the application's queue consistent when files are renamed in the view
  connect(rightWidget, &RightWidget::fileRenamed, this,
          &MainWindow::onFileRenamed);

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

  connect(titleBar, &CustomTitleBar::toggleViewRequested, this, &MainWindow::toggleViewMode);

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
    constantFolderPath = folderPath;
    rightWidget->setTreeRootPath(folderPath);
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
  // 1. First check the high-priority queue
  if (playlistManager->getCurrentQueueIndex() + 1 < (int)playlistManager->getQueue().size()) {
    playlistManager->setCurrentQueueIndex(playlistManager->getCurrentQueueIndex() + 1);
    QString nextPath = playlistManager->getQueue()[playlistManager->getCurrentQueueIndex()];
    playlistManager->maintainQueueHistory();
    rightWidget->setQueue(playlistManager->getQueue());
    qDebug() << "[Queue] Playing next track from queue at index"
             << playlistManager->getCurrentQueueIndex() << ":" << nextPath;
    playTrackByPath(nextPath);
    return;
  }

  // 2. Fall back to PlaylistManager if queue is empty or finished
  QString nextPath = playlistManager->nextTrack(currentTrackPath);

  if (!nextPath.isEmpty()) {
    playlistManager->setCurrentQueueIndex(playlistManager->addTrackToQueue(nextPath, playlistManager->getCurrentQueueIndex() == -1 ? 0 : playlistManager->getCurrentQueueIndex() + 1));
    playlistManager->maintainQueueHistory();
    rightWidget->setQueue(playlistManager->getQueue());
    playTrackByPath(nextPath);
  } else {
    audioPlayer->stop();
  }
}
void MainWindow::updateTrackInfo(const QString &filePath) {
  TrackInfo trackInfo = TrackMetadataExtractor::extract(filePath);
  rightWidget->setPlayingTrackHighlight(filePath);
  leftWidget->updateTrackDisplay(trackInfo);
}
void MainWindow::playTrackByPath(const QString &filePath) {

  // Update queue index if the track is not the one we expected from the queue
  if (playlistManager->getCurrentQueueIndex() >= 0 && playlistManager->getCurrentQueueIndex() < (int)playlistManager->getQueue().size()) {
    if (playlistManager->getQueue()[playlistManager->getCurrentQueueIndex()] != filePath) {
      playlistManager->setCurrentQueueIndex(-1);
    }
  }

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
  // 1. Check high-priority queue first
  if (playlistManager->getCurrentQueueIndex() + 1 < (int)playlistManager->getQueue().size()) {
    playlistManager->setCurrentQueueIndex(playlistManager->getCurrentQueueIndex() + 1);
    QString nextTrackPath = playlistManager->getQueue()[playlistManager->getCurrentQueueIndex()];
    playlistManager->maintainQueueHistory();
    rightWidget->setQueue(playlistManager->getQueue());
    qDebug() << "[Queue] Skipping to next track in queue at index"
             << playlistManager->getCurrentQueueIndex() << ":" << nextTrackPath;
    playTrackByPath(nextTrackPath);
    return;
  }

  if (currentTrackPath.isEmpty()) {
    return;
  }

  // 2. Fall back to PlaylistManager
  QString nextTrackPath = playlistManager->nextTrack(currentTrackPath);
  if (!nextTrackPath.isEmpty()) {
    playlistManager->setCurrentQueueIndex(playlistManager->addTrackToQueue(nextTrackPath, playlistManager->getCurrentQueueIndex() == -1 ? 0 : playlistManager->getCurrentQueueIndex() + 1));
    playlistManager->maintainQueueHistory();
    rightWidget->setQueue(playlistManager->getQueue());
    playTrackByPath(nextTrackPath);
  }
}

void MainWindow::playPreviousTrack() {
  if (currentTrackPath.isEmpty()) {
    return;
  }

  // 1. Check if we have a previous track in the queue
  if (playlistManager->getCurrentQueueIndex() > 0) {
    playlistManager->setCurrentQueueIndex(playlistManager->getCurrentQueueIndex() - 1);
    QString prevTrackPath = playlistManager->getQueue()[playlistManager->getCurrentQueueIndex()];
    playlistManager->maintainQueueHistory();
    rightWidget->setQueue(playlistManager->getQueue());
    qDebug() << "[Queue] Skipping to previous track in queue at index"
             << playlistManager->getCurrentQueueIndex() << ":" << prevTrackPath;
    playTrackByPath(prevTrackPath);
    return;
  }

  // 2. Fall back to PlaylistManager
  QString previousTrackPath = playlistManager->previousTrack(currentTrackPath);
  if (!previousTrackPath.isEmpty()) {
    playlistManager->setCurrentQueueIndex(playlistManager->addTrackToQueue(previousTrackPath, playlistManager->getCurrentQueueIndex() == -1 ? 0 : playlistManager->getCurrentQueueIndex()));
    playlistManager->maintainQueueHistory();
    rightWidget->setQueue(playlistManager->getQueue());
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

void MainWindow::closeEvent(QCloseEvent *event) {
  QSettings settings;
  const QString currentFolder = rightWidget ? rightWidget->getCurrentFolderPath()
                                            : QString();
  if (!currentFolder.isEmpty()) {
    settings.setValue("defaultFolder", currentFolder);
  }

  QMainWindow::closeEvent(event);
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

void MainWindow::onPlayNextRequested(const QString &filePath) {
  qDebug() << "[Queue] Play Next requested. Inserting after current index:"
           << filePath;
  if (playlistManager->getCurrentQueueIndex() == -1) {
    playlistManager->addTrackToQueue(filePath, 0);
  } else {
    // If next is current index, but +1
    playlistManager->addTrackToQueue(filePath, playlistManager->getCurrentQueueIndex() + 1);
  }
  rightWidget->setQueue(playlistManager->getQueue());
}

void MainWindow::onAddToQueueRequested(const QString &filePath) {
  qDebug() << "[Queue] Add to Queue requested. Adding to back of queue:"
           << filePath;
  playlistManager->addTrackToQueue(filePath, -1);
  rightWidget->setQueue(playlistManager->getQueue());
}

void MainWindow::onQueueTrackPlayRequested(int index) {
  if (index >= 0 && index < (int)playlistManager->getQueue().size()) {
    playlistManager->setCurrentQueueIndex(index);
    qDebug() << "[Queue] Manual play from queue at index" << index << ":"
             << playlistManager->getQueue()[index];
    playlistManager->maintainQueueHistory();
    rightWidget->setQueue(playlistManager->getQueue());
    playTrackByPath(playlistManager->getQueue()[playlistManager->getCurrentQueueIndex()]);
  }
}

void MainWindow::onOpenFileLocationRequested(const QString &filePath) {
  qDebug() << "Open File Location requested for:" << filePath;
#if defined(Q_OS_WIN)
  if (revealFileInExplorer(filePath)) {
    return;
  }

  const QFileInfo fileInfo(filePath);
  QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
#elif defined(Q_OS_MAC)
  QProcess::startDetached("open", QStringList() << "-R" << filePath);
#else
  // Try to reveal/select the file in common Linux file managers first.
  auto tryStart = [&](const QString &program, const QStringList &args) {
    return QProcess::startDetached(program, args);
  };

  bool revealed = false;
  if (QFileInfo::exists(filePath)) {
    revealed = tryStart("nautilus", QStringList() << "--select" << filePath) ||
               tryStart("dolphin", QStringList() << "--select" << filePath) ||
               tryStart("nemo", QStringList() << "--no-desktop" << filePath);
  }

  if (!revealed) {
    const QFileInfo fileInfo(filePath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
  }
#endif
}

void MainWindow::onTrackManualPlayRequested(const QString &filePath) {
  qDebug() << "[Queue] Manual track play requested. Inserting and playing:"
           << filePath;

  if (playlistManager->getCurrentQueueIndex() == -1) {
    playlistManager->setCurrentQueueIndex(playlistManager->addTrackToQueue(filePath, 0));
  } else {
    playlistManager->setCurrentQueueIndex(playlistManager->addTrackToQueue(filePath, playlistManager->getCurrentQueueIndex() + 1));
  }

  playlistManager->maintainQueueHistory();
  rightWidget->setQueue(playlistManager->getQueue());
  playTrackByPath(filePath);
}

void MainWindow::onFileRenamed(const QString &oldPath, const QString &newPath) {
  qDebug() << "[FileRename]" << oldPath << "->" << newPath;

  // Normalize paths for cross-platform mounted drives (Windows disk mounted to
  // Linux or vice versa)
  QString normalizedOldPath = QFileInfo(oldPath).absoluteFilePath();
  QString normalizedNewPath = QFileInfo(newPath).absoluteFilePath();

  // Update current track path if it was renamed
  if (currentTrackPath == normalizedOldPath ||
      currentTrackPath.toLower() == normalizedOldPath.toLower()) {
    currentTrackPath = normalizedNewPath;
  }

  // Replace any occurrences in the queue
  std::deque<QString> currentQueue = playlistManager->getQueue();
  bool queueChanged = false;
  for (int i = 0; i < (int)currentQueue.size(); ++i) {
    if (currentQueue[i] == normalizedOldPath ||
        currentQueue[i].toLower() == normalizedOldPath.toLower()) {
      currentQueue[i] = normalizedNewPath;
      queueChanged = true;
    }
  }
  if (queueChanged) {
    playlistManager->setQueue(currentQueue);
  }

  // Update UI
  rightWidget->setQueue(playlistManager->getQueue());
}

void MainWindow::toggleViewMode() {
  if (isMaximized()) {
    showNormal();
    topSplitter->setSizes({400, 10000}); // 400 for LeftWidget, rest for right
  } else {
    showMaximized();
    topSplitter->setSizes({700, 10000}); // 700 for LeftWidget, rest for right
  }
}

