// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QPixmap>
#include <QResizeEvent>
#include <QShortcut>
#include <QDebug>

#include <QAbstractItemModel>
#include <QString>
#include "HighlightDelegate.h"

#include "core/PlaylistManager.h"
#include "core/AudioPlayer.h"
#include "InteractiveBackground.h"



class LeftWidget;
class RightWidget;
class BottomWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void openFolderDialog();
    void openConstantFolder();
    
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

    // --- Блок UI элементов ---
    
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QSplitter *topSplitter;

    LeftWidget *leftWidget;
    RightWidget *rightWidget;
    BottomWidget *bottomWidget;
    

    QShortcut *enterShortcut;
    QShortcut *returnShortcut;
};

#endif // MAINWINDOW_H