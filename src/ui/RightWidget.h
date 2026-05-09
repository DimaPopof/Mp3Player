#pragma once
#include <QWidget>
#include <QStackedWidget>
#include <QTreeView>
#include <QListView>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QModelIndex>
#include "HighlightDelegate.h"
#include "CustomIconProvider.h"
#include <QPainter>
#include <QThread>
#include <QDirIterator>
#include <QStandardItemModel>
#include <atomic>

class SearchWorker : public QThread {
    Q_OBJECT
public:
    SearchWorker(const QString& rootPath, const QString& query, QObject* parent = nullptr) 
        : QThread(parent), rootPath(rootPath), query(query), stopRequested(false) {}

    void stop() { stopRequested.store(true, std::memory_order_relaxed); }

signals:
    void fileFound(const QString& filePath, const QString& fileName);
    void searchFinished();

protected:
    void run() override {
        QDirIterator it(rootPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        QString lowerQuery = query.toLower();
        while (it.hasNext() && !stopRequested.load(std::memory_order_relaxed)) {
            it.next();
            QString fileName = it.fileName();
            if (fileName.toLower().contains(lowerQuery)) {
                emit fileFound(it.filePath(), fileName);
            }
        }
        emit searchFinished();
    }

private:
    QString rootPath;
    QString query;
    std::atomic<bool> stopRequested;
};

class QTreeView;
class QLabel;
class QSlider;
class QFileSystemModel;
class CustomSortProxyModel;

class VsCodeTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit VsCodeTreeView(QWidget *parent = nullptr) : QTreeView(parent) {}
    void requestDelayedItemsLayout() { scheduleDelayedItemsLayout(); }

protected:
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const override {
        // 1. Отрисовываем стандартные стрелочки раскрытия папок
        QTreeView::drawBranches(painter, rect, index);

        // 2. ПРАВИЛЬНЫЙ подсчет глубины
        int depth = 0;
        QModelIndex p = index.parent();
        QModelIndex root = rootIndex(); // Получаем индекс главной открытой папки

        // Считаем родителей, пока не упремся в главную папку интерфейса
        while (p.isValid() && p != root) {
            depth++;
            p = p.parent();
        }

        // 3. Рисуем направляющие линии
        if (depth > 0) {
            painter->save();
            
            // Цвет линии (белый с прозрачностью 30/255)
            painter->setPen(QPen(QColor(255, 255, 255, 30), 1));

            int step = indentation(); 

            for (int i = 0; i < depth; ++i) {
                // Линия по центру каждого отступа
                int x = rect.left() + (i * step) + (step / 2);
                painter->drawLine(x, rect.top(), x, rect.bottom());
            }
            
            painter->restore();
        }
    }
};

class VsCodeListView : public QListView {
    Q_OBJECT
public:
    explicit VsCodeListView(QWidget *parent = nullptr) : QListView(parent) {}
    void requestDelayedItemsLayout() { scheduleDelayedItemsLayout(); }
};

class RightWidget : public QWidget {
    Q_OBJECT

public:
    explicit RightWidget(QWidget *parent = nullptr);
    void setTreeRootPath(const QString& path); //Tree is a main reference for current folder, so this method will update both views to match the saved path
    void setTreeFolder(const QString &folderPath);
    void setListFolder(const QString &folderPath);
    void selectFileInTree(const QString &filePath);
    void setPlayingTrackHighlight(const QString &filePath);
    void applySize(int pointSize);
    QString getCurrentFolderPath() const;
    

signals:
    void trackPlayRequested(const QString& filePath);
    void listFolderNavigationRequested(const QString &folderPath);
private slots:
    void onListDoubleClicked(const QModelIndex &index);
    void treeViewDoubleClicked(const QModelIndex &index);
    void onSearchTextChanged(const QString &text);
    void onSearchFileFound(const QString &filePath, const QString &fileName);
    void onSearchResultsDoubleClicked(const QModelIndex &index);

private:
    QStackedWidget *viewStack;
    VsCodeTreeView *fileTreeView;
    VsCodeListView *fileListView;
    VsCodeListView *searchResultsView;
    QPushButton *treeButton;
    QPushButton *listButton;
    QLineEdit *searchBox;
    QAbstractItemModel *treeFileSystemModel;
    QFileSystemModel *treeSourceModel;
    CustomSortProxyModel *treeProxyModel;

    HighlightDelegate *highlightDelegate;

    QAbstractItemModel *listFileSystemModel;
    QFileSystemModel *listSourceModel;
    CustomSortProxyModel *listProxyModel;
    
    QStandardItemModel *searchResultsModel;
    SearchWorker *currentSearchWorker{nullptr};
    int lastViewIndex{0};

    void setupUi();
    void setupConnections();
    void toggleFileView();
    void toggleListView();
protected:
    void paintEvent(QPaintEvent *event) override;
};