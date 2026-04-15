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