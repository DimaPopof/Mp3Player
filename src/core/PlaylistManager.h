// PlaylistManager.h
#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QStringList>
#include <QDir>
#include <QString>
#include <QFileInfoList>

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

private:
    QStringList trackList;
};

#endif // PLAYLISTMANAGER_H