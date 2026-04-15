#include "RightWidget.h"
#include <QTreeView>
#include <QFileSystemModel>
#include <QFileInfo>
#include "models/CustomSortProxyModel.h"
#include "models/FileBrowserModelFactory.h"

RightWidget::RightWidget(QWidget *parent) : QWidget(parent){
    setObjectName("rightWidget");
    setupUi();
    setupConnections();
}

void RightWidget::setupUi() {
    QVBoxLayout *rightLayout = new QVBoxLayout(this);
    rightLayout->setContentsMargins(10, 10, 10, 10);

    viewStack = new QStackedWidget(this);
    
    treeFileSystemModel = FileBrowserModelFactory::createTreeModel(this);
    treeProxyModel = qobject_cast<CustomSortProxyModel*>(treeFileSystemModel);
    treeSourceModel = qobject_cast<QFileSystemModel*>(treeProxyModel->sourceModel());
    if (treeProxyModel) {
        treeSourceModel = qobject_cast<QFileSystemModel*>(treeProxyModel->sourceModel()); 
    }
    // Set custom icon provider for tree view
    if (treeSourceModel) {
        treeSourceModel->setIconProvider(new CustomIconProvider());
    }   
    fileTreeView = new VsCodeTreeView(viewStack);
    fileTreeView->setModel(treeProxyModel);
    highlightDelegate = new HighlightDelegate(this);
    fileTreeView->setFrameShape(QFrame::NoFrame); // Убирает рамку (аналог border: none)
    fileTreeView->setAutoFillBackground(false);
    fileTreeView->setAttribute(Qt::WA_TranslucentBackground);
    fileTreeView->viewport()->setAutoFillBackground(false); // Отключает заливку фона
    fileTreeView->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    fileTreeView->setItemDelegate(highlightDelegate);
    fileTreeView->setColumnHidden(1, true);
    fileTreeView->setColumnHidden(2, true);
    fileTreeView->setColumnHidden(3, true);
    fileTreeView->setHeaderHidden(true);
    fileTreeView->setExpandsOnDoubleClick(false);
    fileTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
        const QString scrollBarStyle =
            "QScrollBar:vertical {"
            "background: rgba(255, 255, 255, 0.04);"
            "width: 12px;"
            "margin: 0px;"
            "border: none;"
            "}"
            "QScrollBar::handle:vertical {"
            "background: #666666;"
            "min-height: 28px;"
            "border-radius: 6px;"
            "}"
            "QScrollBar::handle:vertical:hover {"
            "background: #7a7a7a;"
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
            "height: 0px;"
            "background: none;"
            "border: none;"
            "}"
            "QScrollBar:horizontal {"
            "background: rgba(255, 255, 255, 0.04);"
            "height: 12px;"
            "margin: 0px;"
            "border: none;"
            "}"
            "QScrollBar::handle:horizontal {"
            "background: #666666;"
            "min-width: 28px;"
            "border-radius: 6px;"
            "}"
            "QScrollBar::handle:horizontal:hover {"
            "background: #7a7a7a;"
            "}"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
            "width: 0px;"
            "background: none;"
            "border: none;"
            "}";
        fileTreeView->setStyleSheet(scrollBarStyle);

    fileListView = new VsCodeListView(viewStack);
    listFileSystemModel = FileBrowserModelFactory::createListModel(this);
    listProxyModel = qobject_cast<CustomSortProxyModel*>(listFileSystemModel);
    listSourceModel = qobject_cast<QFileSystemModel*>(listProxyModel->sourceModel());   
    if (listProxyModel) {
        listSourceModel = qobject_cast<QFileSystemModel*>(listProxyModel->sourceModel()); 
    }
    // Set custom icon provider for list view
    if (listSourceModel) {
        listSourceModel->setIconProvider(new CustomIconProvider());
    } 
    fileListView->setFrameShape(QFrame::NoFrame);
    fileListView->setAutoFillBackground(false);
    fileListView->setAttribute(Qt::WA_TranslucentBackground);
    fileListView->viewport()->setAutoFillBackground(false);
    fileListView->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    fileListView->setItemDelegate(highlightDelegate);
    fileListView->setViewMode(QListView::ListMode);
    fileListView->setModel(listProxyModel);
    fileListView->setSelectionMode(QAbstractItemView::SingleSelection);
        fileListView->setStyleSheet(scrollBarStyle);
    fileListView->setSpacing(0);
    fileListView->setUniformItemSizes(true);

    viewStack->addWidget(fileTreeView);
    viewStack->addWidget(fileListView);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    treeButton = new QPushButton(QIcon(":/assets/tree.png"), "", this);
    listButton = new QPushButton(QIcon(":/assets/list.png"), "", this);
    const QString viewToggleStyle =
        "QPushButton {"
        "background: transparent;"
        "border: 1px solid transparent;"
        "border-radius: 6px;"
        "padding: 6px;"
        "}"
        "QPushButton:hover {"
        "background-color: #444444;"
        "}"
        "QPushButton:pressed {"
        "background-color: #666666;"
        "}";
    treeButton->setStyleSheet(viewToggleStyle);
    listButton->setStyleSheet(viewToggleStyle);
    buttonsLayout->addWidget(treeButton);
    buttonsLayout->addWidget(listButton);
    buttonsLayout->addStretch();

    QFrame *separator = new QFrame(this);
    separator->setFixedHeight(3);
    separator->setStyleSheet("background-color: rgba(10, 10, 10, 130); border: none;");

    rightLayout->addLayout(buttonsLayout);
    rightLayout->addWidget(separator);
    rightLayout->addWidget(viewStack, 10);
}

void RightWidget::setupConnections() {

    connect(fileTreeView, &QTreeView::doubleClicked, this, &RightWidget::treeViewDoubleClicked);
    connect(fileListView, &QListView::doubleClicked, this, &RightWidget::onListDoubleClicked);
    connect(treeButton, &QPushButton::clicked, this, &RightWidget::toggleFileView);
    connect(listButton, &QPushButton::clicked, this, &RightWidget::toggleListView);
}
void RightWidget::applySize(int pointSize) {
    QFont font = this->font(); // Берем текущий шрифт виджета
    font.setPointSize(pointSize);

    fileTreeView->setFont(font);
    fileListView->setFont(font);

    int iconSizeDim = static_cast<int>(pointSize * 1.45);
    QSize newIconSize(iconSizeDim, iconSizeDim);
    fileTreeView->setIconSize(newIconSize);
    fileListView->setIconSize(newIconSize);

    const int verticalPadding = qMax(1, pointSize / 4);
    fileTreeView->setStyleSheet(QString("QTreeView::item { padding: %1px 0px; }").arg(verticalPadding));
    fileListView->setStyleSheet(QString("QListView::item { padding: %1px 0px; }").arg(verticalPadding));

    fileTreeView->requestDelayedItemsLayout();
    fileListView->requestDelayedItemsLayout();
}
void RightWidget::toggleFileView() {
    viewStack->setCurrentIndex(0);
}

void RightWidget::toggleListView() {
    viewStack->setCurrentIndex(1);
}

void RightWidget::treeViewDoubleClicked(const QModelIndex &index) {
    if (!index.isValid() || !treeProxyModel || !treeSourceModel) {
        return;
    }

    // Map the proxy index from the view to the source model index
    QModelIndex sourceIndex = treeProxyModel->mapToSource(index);
    
    // Retrieve the file path directly from the stored source model
    QString filePath = treeSourceModel->filePath(sourceIndex);
    QFileInfo fileInfo(filePath);
    
    // Handle directory clicks: toggle expand/collapse state
    if (fileInfo.isDir()) {
        if (fileTreeView->isExpanded(index)) {
            fileTreeView->collapse(index);
        } else {
            fileTreeView->expand(index);
        }
        return;
    }

    // Handle file clicks: emit signal if the file is an MP3
    if (fileInfo.isFile() && fileInfo.suffix().toLower() == "mp3") {
        emit trackPlayRequested(filePath);
    }
}
void RightWidget::onListDoubleClicked(const QModelIndex &index) {
    if (!index.isValid() || !listProxyModel || !listSourceModel) {
        return;
    }

    // 1. Конвертация индексов
    QModelIndex sourceIndex = listProxyModel->mapToSource(index);
    QString filePath = listSourceModel->filePath(sourceIndex);
    QString fileName = listSourceModel->fileName(sourceIndex);
    QFileInfo fileInfo(filePath);

    // 2. Обработка перехода на уровень выше ".."
    if (fileName == "..") {
        QDir currentDir(listSourceModel->rootPath());
        if (currentDir.cdUp()) {
            emit listFolderNavigationRequested(currentDir.absolutePath());
        }
        return;
    }

    // 3. Обработка входа в обычную папку
    if (fileInfo.isDir()) {
        emit listFolderNavigationRequested(filePath);
        return;
    }

    // 4. Обработка запуска трека
    if (fileInfo.isFile() && fileInfo.suffix().toLower() == "mp3") {
        emit trackPlayRequested(filePath); 
    }
}
void RightWidget::setTreeFolder(const QString &folderPath) {
    // Используем сохраненные типизированные указатели
    if (!treeSourceModel || !treeProxyModel) return;

    // Запускаем сканирование папки
    treeSourceModel->setRootPath(folderPath);
    
    // Конвертируем индексы и обновляем интерфейс
    QModelIndex sourceIndex = treeSourceModel->index(folderPath);
    QModelIndex proxyIndex = treeProxyModel->mapFromSource(sourceIndex);
    
    fileTreeView->setRootIndex(proxyIndex);
}
void RightWidget::setListFolder(const QString &folderPath) {
    if (!listSourceModel || !listProxyModel) return;

    listSourceModel->setRootPath(folderPath);
    QModelIndex sourceIndex = listSourceModel->index(folderPath);
    QModelIndex proxyIndex = listProxyModel->mapFromSource(sourceIndex);
    fileListView->setRootIndex(proxyIndex);
}
void RightWidget::selectFileInTree(const QString &filePath) {
    if (!treeSourceModel || !treeProxyModel) return;

    QModelIndex sourceIndex = treeSourceModel->index(filePath);
    QModelIndex proxyIndex = treeProxyModel->mapFromSource(sourceIndex);
    
    if (proxyIndex.isValid()) {
        fileTreeView->setCurrentIndex(proxyIndex);
    }
}
void RightWidget::setPlayingTrackHighlight(const QString &filePath) {

    if (highlightDelegate) {
        highlightDelegate->setPlayingFilePath(filePath);
        fileTreeView->viewport()->update();
        fileListView->viewport()->update(); 
    }
}
QString RightWidget::getCurrentFolderPath() const {
    if (treeSourceModel) { 
        return treeSourceModel->rootPath(); 
    }
    return QString();
}
void RightWidget::setTreeRootPath(const QString& path) {
    setTreeFolder(path);
    setListFolder(path); // Update both views to match the saved path
}
void RightWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    
    // Включаем сглаживание, чтобы углы были красивыми и не пиксельными
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Задаем цвет заливки (ваш темный полупрозрачный фон)
    painter.setBrush(QColor(30, 30, 30, 235));
    
    // Отключаем обводку
    painter.setPen(Qt::NoPen);
    
    // Рисуем прямоугольник по размеру всего виджета со скруглением 10px
    painter.drawRoundedRect(rect(), 10, 10);
}