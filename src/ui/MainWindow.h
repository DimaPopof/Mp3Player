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

#include <QAbstractItemModel>
#include <QString>
#include "HighlightDelegate.h"

#include "core/PlaylistManager.h"
#include "core/AudioPlayer.h"

// 1. Предварительное объявление новых классов (Forward declarations)
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

    void updateDuration(int duration);
    void updatePosition(int position);
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
    int currentDuration{0};
    int baseFontSize{14};
    QString constantFolderPath;
    QString treeCurrentRootPath{QDir::homePath()};
    QString listCurrentRootPath{QDir::homePath()};
    bool isMuted{false};
    int savedVolume{50};
    
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

    // 2. Подключаем твои новые виджеты вместо десятков кнопок и слайдеров
    LeftWidget *leftWidget;
    RightWidget *rightWidget;
    BottomWidget *bottomWidget;
    
    // 3. Оставляем горячие клавиши, они относятся к главному окну
    QShortcut *enterShortcut;
    QShortcut *returnShortcut;
};

#endif // MAINWINDOW_H