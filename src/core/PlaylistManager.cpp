// PlaylistManager.cpp
#include "PlaylistManager.h"
#include <QDebug>

PlaylistManager::PlaylistManager(QObject *parent) : QObject(parent) {
    // Constructor initializes the object
}

void PlaylistManager::loadDirectory(const QString &folderPath) {
    QDir directory(folderPath);

    // Check if the directory exists
    if (!directory.exists()) {
        return;
    }

    // Set the filter to look for .mp3 files
    QStringList nameFilters;
    nameFilters << "*.mp3";

    // Extract file paths using the filter
    QFileInfoList fileInfoList = directory.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);

    // Clear the previous playlist data
    trackList.clear();

    // Add new file paths to the track list
    for (const QFileInfo &fileInfo : fileInfoList) {
        trackList.append(fileInfo.absoluteFilePath());
    }
}

QStringList PlaylistManager::getTrackList() const {
    return trackList;
}

int PlaylistManager::indexOfTrack(const QString &filePath) const {
    return trackList.indexOf(filePath);
}

QString PlaylistManager::nextTrack(const QString &currentFilePath) const {

    int currentIndex = indexOfTrack(currentFilePath);

    if (trackList.isEmpty() || currentIndex == -1) {
        return QString();
    }

    int nextIndex = (currentIndex + 1) % trackList.size();
    QString nextPath = trackList.at(nextIndex);
    return nextPath;
}

QString PlaylistManager::previousTrack(const QString &currentFilePath) const {
    int currentIndex = indexOfTrack(currentFilePath);
    if (trackList.isEmpty() || currentIndex == -1) {
        return QString();
    }

    int previousIndex = currentIndex - 1;
    if (previousIndex < 0) {
        previousIndex = trackList.size() - 1;
    }

    return trackList.at(previousIndex);
}
int PlaylistManager::addTrackToQueue(const QString& trackPath, int insertIndex) {
  // Find and remove existing duplicate track to ensure uniqueness
  int duplicateIndex = -1;
  for (int i = 0; i < (int)trackQueue.size(); ++i) {
    if (trackQueue[i] == trackPath) {
      duplicateIndex = i;
      break;
    }
  }

  if (duplicateIndex != -1) {
    trackQueue.erase(trackQueue.begin() + duplicateIndex);
    if (duplicateIndex <= currentQueueIndex && currentQueueIndex > 0) {
      currentQueueIndex--;
    }
    // Adjust target index if we removed element before it
    if (insertIndex != -1 && duplicateIndex < insertIndex) {
      insertIndex--;
    }
  }

  // Insert the track
  int finalIndex = -1;
  if (insertIndex == -1 || insertIndex >= (int)trackQueue.size()) {
    trackQueue.push_back(trackPath);
    finalIndex = trackQueue.size() - 1;
  } else {
    trackQueue.insert(trackQueue.begin() + insertIndex, trackPath);
    finalIndex = insertIndex;
  }
  
  return finalIndex;
}

void PlaylistManager::maintainQueueHistory() {
  while (currentQueueIndex > 10 && !trackQueue.empty()) {
    trackQueue.pop_front();
    currentQueueIndex--;
  }
}
