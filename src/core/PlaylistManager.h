// PlaylistManager.h
#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QDir>
#include <QFileInfoList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <deque>

// The PlaylistManager class stores and manages the list of MP3 files
class PlaylistManager : public QObject {
  Q_OBJECT

public:
  explicit PlaylistManager(QObject *parent = nullptr);

  // Loads all MP3 files from the specified folder
  void loadDirectory(const QString &folderPath);

  // Returns the current list of MP3 file paths
  QStringList getTrackList() const;

  int indexOfTrack(const QString &filePath) const;
  QString nextTrack(const QString &currentFilePath) const;
  QString previousTrack(const QString &currentFilePath) const;

  // Queue Operations
  int addTrackToQueue(const QString& trackPath, int insertIndex = -1);
  void maintainQueueHistory();
  const std::deque<QString>& getQueue() const { return trackQueue; }
  void setQueue(const std::deque<QString>& newQueue) { trackQueue = newQueue; }
  int getCurrentQueueIndex() const { return currentQueueIndex; }
  void setCurrentQueueIndex(int index) { currentQueueIndex = index; }
  void clearQueue() { trackQueue.clear(); currentQueueIndex = -1; }

private:
  QStringList trackList;
  std::deque<QString> trackQueue;
  int currentQueueIndex{-1};
};

#endif // PLAYLISTMANAGER_H