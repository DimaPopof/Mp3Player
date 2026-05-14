#include "RightWidget.h"
#include "ThemeManager.h"
#include "models/CustomSortProxyModel.h"
#include "models/FileBrowserModelFactory.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QTimer>
#include <QUuid>

#include <functional>

#include <QTreeView>
#include <QVBoxLayout>

namespace {
bool renameFilesystemEntry(QFileSystemModel *fileSystemModel,
                           const QModelIndex &sourceIndex,
                           const QString &newName, QString *oldPathOut,
                           QString *newPathOut) {
  if (!fileSystemModel || !sourceIndex.isValid()) {
    return false;
  }

  const QString oldPath = fileSystemModel->filePath(sourceIndex);
  if (oldPath.isEmpty()) {
    return false;
  }

  const QFileInfo oldInfo(oldPath);
  const QString parentPath = oldInfo.absolutePath();
  const QString newPath = QDir(parentPath).absoluteFilePath(newName);
  if (newPath.isEmpty() || oldPath == newPath) {
    return false;
  }

  QDir parentDir(parentPath);
  const QString oldName = oldInfo.fileName();

  auto renameOnce = [&parentDir](const QString &fromName,
                                 const QString &toName) {
    return parentDir.rename(fromName, toName);
  };

  bool renamed = renameOnce(oldName, newName);

#if defined(Q_OS_WIN)
  if (!renamed && oldPath.compare(newPath, Qt::CaseInsensitive) == 0) {
    const QString tempName =
        QString(".__rename_%1__")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (renameOnce(oldName, tempName)) {
      renamed = renameOnce(tempName, newName);
    }
  }
#endif

  if (!renamed) {
    return false;
  }

  if (oldPathOut) {
    *oldPathOut = oldPath;
  }
  if (newPathOut) {
    *newPathOut = newPath;
  }
  return true;
}
} // namespace

class FilesystemRenameDelegate : public HighlightDelegate {
public:
  using RenameCallback = std::function<void(const QString &, const QString &)>;

  explicit FilesystemRenameDelegate(RenameCallback onRenameCommitted,
                                    QObject *parent = nullptr)
      : HighlightDelegate(parent),
        onRenameCommitted(std::move(onRenameCommitted)) {}

  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override {
    auto *proxyModel = qobject_cast<QSortFilterProxyModel *>(model);
    auto *fileSystemModel =
        proxyModel ? qobject_cast<QFileSystemModel *>(proxyModel->sourceModel())
                   : qobject_cast<QFileSystemModel *>(model);

    if (!fileSystemModel) {
      HighlightDelegate::setModelData(editor, model, index);
      return;
    }

    const auto *lineEdit = qobject_cast<const QLineEdit *>(editor);
    const QString newName = lineEdit
                                ? lineEdit->text().trimmed()
                                : editor->property("text").toString().trimmed();
    if (newName.isEmpty()) {
      return;
    }

    const QModelIndex sourceIndex =
        proxyModel ? proxyModel->mapToSource(index) : index;
    if (!sourceIndex.isValid()) {
      return;
    }

    QString oldPath;
    QString newPath;
    if (renameFilesystemEntry(fileSystemModel, sourceIndex, newName, &oldPath,
                              &newPath)) {
      if (onRenameCommitted) {
        onRenameCommitted(oldPath, newPath);
      }
      return;
    }

    HighlightDelegate::setModelData(editor, model, index);
  }

private:
  RenameCallback onRenameCommitted;
};

RightWidget::RightWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("rightWidget");
  setAttribute(Qt::WA_StyledBackground, true);
  setMinimumWidth(
      300); // Set minimum width on the widget to properly stop the QSplitter
  setupUi();
  setupConnections();
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          &RightWidget::refreshTheme);
  refreshTheme();

  if (qApp) {
    qApp->installEventFilter(this);
  }
}

RightWidget::~RightWidget() {
  if (qApp) {
    qApp->removeEventFilter(this);
  }
}

void RightWidget::setupUi() {
  QVBoxLayout *rightLayout = new QVBoxLayout(this);
  rightLayout->setContentsMargins(0, 0, 0, 0);

  viewStack = new QStackedWidget(this);

  treeFileSystemModel = FileBrowserModelFactory::createTreeModel(this);
  treeProxyModel = qobject_cast<CustomSortProxyModel *>(treeFileSystemModel);
  treeSourceModel =
      qobject_cast<QFileSystemModel *>(treeProxyModel->sourceModel());
  if (treeProxyModel) {
    treeSourceModel =
        qobject_cast<QFileSystemModel *>(treeProxyModel->sourceModel());
  }
  // Set custom icon provider for tree view
  if (treeSourceModel) {
    treeSourceModel->setIconProvider(new CustomIconProvider());
    treeSourceModel->setReadOnly(false);

    // Propagate file rename events so the app can keep in-memory queues
    // consistent
    connect(treeSourceModel, &QFileSystemModel::fileRenamed, this,
            [this](const QString &newPath, const QString &oldName) {
              // QFileSystemModel emits (newPath, oldName) on rename
              QString oldPath = QFileInfo(newPath).absolutePath() +
                                QDir::separator() + oldName;
              emit fileRenamed(oldPath, newPath);
            });
  }
  fileTreeView = new VsCodeTreeView(viewStack);
  fileTreeView->setObjectName("fileTreeView");
  fileTreeView->setModel(treeProxyModel);
  highlightDelegate = new FilesystemRenameDelegate(
      [this](const QString &oldPath, const QString &newPath) {
        emit fileRenamed(oldPath, newPath);
      },
      this);
  fileTreeView->setFrameShape(
      QFrame::NoFrame); // Убирает рамку (аналог border: none)
  fileTreeView->setAutoFillBackground(false);
  fileTreeView->setAttribute(Qt::WA_TranslucentBackground);
  fileTreeView->viewport()->setAutoFillBackground(
      false); // Отключает заливку фона
  fileTreeView->viewport()->setAttribute(Qt::WA_TranslucentBackground);
  fileTreeView->setItemDelegate(highlightDelegate);
  fileTreeView->setColumnHidden(1, true);
  fileTreeView->setColumnHidden(2, true);
  fileTreeView->setColumnHidden(3, true);
  fileTreeView->setHeaderHidden(true);
  fileTreeView->setExpandsOnDoubleClick(false);
  fileTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
  fileTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
  fileTreeView->setEditTriggers(QAbstractItemView::EditKeyPressed);
  fileTreeView->setFocusPolicy(Qt::NoFocus); // Disable focus rectangle
  fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

  fileTreeView->setAttribute(Qt::WA_MacShowFocusRect,
                             false); // Disable focus on macOS
  fileTreeView->setStyleSheet(
      "QTreeView { outline: none; border: none; }"); // Extra suppression;
  QPalette treePal = fileTreeView->palette();
  treePal.setColor(QPalette::Highlight, Qt::transparent);
  treePal.setColor(QPalette::HighlightedText, Qt::transparent);
  fileTreeView->setPalette(treePal);
  fileListView = new VsCodeListView(viewStack);
  fileListView->setObjectName("fileListView");
  listFileSystemModel = FileBrowserModelFactory::createListModel(this);
  listProxyModel = qobject_cast<CustomSortProxyModel *>(listFileSystemModel);
  listSourceModel =
      qobject_cast<QFileSystemModel *>(listProxyModel->sourceModel());
  if (listProxyModel) {
    listSourceModel =
        qobject_cast<QFileSystemModel *>(listProxyModel->sourceModel());
  }
  // Set custom icon provider for list view
  if (listSourceModel) {
    listSourceModel->setIconProvider(new CustomIconProvider());
    listSourceModel->setReadOnly(false);
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
  fileListView->setEditTriggers(QAbstractItemView::EditKeyPressed);
  fileListView->setSpacing(0);
  fileListView->setUniformItemSizes(true);
  fileListView->setFocusPolicy(Qt::NoFocus); // Disable focus rectangle
  fileListView->setContextMenuPolicy(Qt::CustomContextMenu);

  fileListView->setAttribute(Qt::WA_MacShowFocusRect,
                             false); // Disable focus on macOS
  fileListView->setStyleSheet(
      "QListView { outline: none; border: none; }"); // Extra suppression
  QPalette listPal = fileListView->palette();
  listPal.setColor(QPalette::Highlight, Qt::transparent);
  listPal.setColor(QPalette::HighlightedText, Qt::transparent);
  fileListView->setPalette(listPal);

  searchResultsView = new VsCodeListView(viewStack);
  searchResultsView->setObjectName("searchResultsView");
  searchResultsModel = new QStandardItemModel(this);
  searchResultsView->setModel(searchResultsModel);
  searchResultsView->setFrameShape(QFrame::NoFrame);
  searchResultsView->setAutoFillBackground(false);
  searchResultsView->setAttribute(Qt::WA_TranslucentBackground);
  searchResultsView->viewport()->setAutoFillBackground(false);
  searchResultsView->viewport()->setAttribute(Qt::WA_TranslucentBackground);
  searchResultsView->setItemDelegate(highlightDelegate);
  searchResultsView->setViewMode(QListView::ListMode);
  searchResultsView->setSelectionMode(QAbstractItemView::SingleSelection);
  searchResultsView->setFocusPolicy(Qt::NoFocus); // Disable focus rectangle
  searchResultsView->setContextMenuPolicy(Qt::CustomContextMenu);
  searchResultsView->setAttribute(Qt::WA_MacShowFocusRect,
                                  false); // Disable focus on macOS
  searchResultsView->setStyleSheet(
      "QListView { outline: none; border: none; }");
  QPalette searchPal = searchResultsView->palette();
  searchPal.setColor(QPalette::Highlight, Qt::transparent);
  searchPal.setColor(QPalette::HighlightedText, Qt::transparent);
  searchResultsView->setPalette(searchPal);
  searchResultsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  searchResultsView->setSpacing(0);
  searchResultsView->setUniformItemSizes(true);

  searchResultsView->setUniformItemSizes(true);

  queueListView = new VsCodeListView(viewStack);
  queueListView->setObjectName("queueListView");
  queueModel = new QStandardItemModel(this);
  queueListView->setModel(queueModel);
  queueListView->setFrameShape(QFrame::NoFrame);
  queueListView->setAutoFillBackground(false);
  queueListView->setAttribute(Qt::WA_TranslucentBackground);
  queueListView->viewport()->setAutoFillBackground(false);
  queueListView->viewport()->setAttribute(Qt::WA_TranslucentBackground);
  queueListView->setItemDelegate(highlightDelegate);
  queueListView->setViewMode(QListView::ListMode);
  queueListView->setSelectionMode(QAbstractItemView::SingleSelection);
  queueListView->setFocusPolicy(Qt::NoFocus);
  queueListView->setContextMenuPolicy(Qt::CustomContextMenu);
  queueListView->setAttribute(Qt::WA_MacShowFocusRect, false);
  queueListView->setStyleSheet("QListView { outline: none; border: none; }");

  QPalette queuePal = queueListView->palette();
  queuePal.setColor(QPalette::Highlight, Qt::transparent);
  queuePal.setColor(QPalette::HighlightedText, Qt::transparent);
  queueListView->setPalette(queuePal);
  queueListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  queueListView->setSpacing(0);
  queueListView->setUniformItemSizes(true);

  viewStack->addWidget(fileTreeView);      // index 0
  viewStack->addWidget(fileListView);      // index 1
  viewStack->addWidget(searchResultsView); // index 2
  viewStack->addWidget(queueListView);     // index 3

  QHBoxLayout *buttonsLayout = new QHBoxLayout();
  buttonsLayout->setContentsMargins(12, 12, 12, 12);
  buttonsLayout->setSpacing(8);

  auto tintIcon = [](const QString &iconPath, const QColor &color) {
    QPixmap src(iconPath);
    QPixmap tinted(src.size());
    tinted.fill(Qt::transparent);

    QPainter p(&tinted);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawPixmap(0, 0, src);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(tinted.rect(), color);
    p.end();

    return QIcon(tinted);
  };

  treeButton = new QPushButton(this);
  treeButton->setObjectName("treeButton");
  treeButton->setIcon(tintIcon(":/assets/tree.png",
                               ThemeManager::instance().getIconTintColor()));
  treeButton->setIconSize(QSize(20, 20));

  listButton = new QPushButton(this);
  listButton->setObjectName("listButton");
  listButton->setIcon(tintIcon(":/assets/list.png",
                               ThemeManager::instance().getIconTintColor()));
  listButton->setIconSize(QSize(20, 20));

  queueButton = new QPushButton(this);
  queueButton->setObjectName("queueButton");
  queueButton->setIcon(
      tintIcon(":/assets/queue.png", // Using list icon as placeholder
               ThemeManager::instance().getIconTintColor()));
  queueButton->setIconSize(QSize(20, 20));
  queueButton->setToolTip("Show Queue");

  // Create a container for the theme switch
  QFrame *themeSwitchContainer = new QFrame(this);
  themeSwitchContainer->setObjectName("themeSwitchContainer");
  QHBoxLayout *themeSwitchLayout = new QHBoxLayout(themeSwitchContainer);
  themeSwitchLayout->setContentsMargins(0, 0, 0, 0);
  themeSwitchLayout->setSpacing(0);

  darkThemeButton = new QPushButton("Dark", themeSwitchContainer);
  darkThemeButton->setObjectName("darkThemeButton");
  darkThemeButton->setCheckable(true);
  darkThemeButton->setCursor(Qt::PointingHandCursor);

  QFrame *themeSeparator = new QFrame(themeSwitchContainer);
  themeSeparator->setObjectName("themeSeparator");
  themeSeparator->setFrameShape(QFrame::VLine);

  pastelThemeButton = new QPushButton("Pastel", themeSwitchContainer);
  pastelThemeButton->setObjectName("pastelThemeButton");
  pastelThemeButton->setCheckable(true);
  pastelThemeButton->setCursor(Qt::PointingHandCursor);

  themeSwitchLayout->addWidget(darkThemeButton);
  themeSwitchLayout->addWidget(themeSeparator);
  themeSwitchLayout->addWidget(pastelThemeButton);

  themeSwitchContainer->hide();

  searchBox = new QLineEdit(this);
  searchBox->setObjectName("searchBox");
  searchBox->setPlaceholderText("Search...");
  searchBox->setClearButtonEnabled(true);
  searchBox->setMinimumHeight(28);

  buttonsLayout->addWidget(treeButton);
  buttonsLayout->addWidget(listButton);
  buttonsLayout->addWidget(queueButton);
  buttonsLayout->addStretch();
  buttonsLayout->addWidget(themeSwitchContainer);
  buttonsLayout->addSpacing(10);
  buttonsLayout->addWidget(searchBox);

  QFrame *separator = new QFrame(this);
  separator->setObjectName("separatorLine");

  rightLayout->addLayout(buttonsLayout);
  rightLayout->addWidget(separator);
  rightLayout->addWidget(viewStack, 10);
}

void RightWidget::setupConnections() {

  connect(fileTreeView, &QTreeView::doubleClicked, this,
          &RightWidget::treeViewDoubleClicked);
  connect(fileListView, &QListView::doubleClicked, this,
          &RightWidget::onListDoubleClicked);
  connect(queueListView, &QListView::doubleClicked, this,
          &RightWidget::onQueueDoubleClicked);
  connect(searchResultsView, &QListView::doubleClicked, this,
          &RightWidget::onSearchResultsDoubleClicked);
  connect(treeButton, &QPushButton::clicked, this,
          &RightWidget::toggleFileView);
  connect(listButton, &QPushButton::clicked, this,
          &RightWidget::toggleListView);
  connect(queueButton, &QPushButton::clicked, this,
          &RightWidget::toggleQueueView);
  connect(darkThemeButton, &QPushButton::clicked, this, [this]() {
    ThemeManager::instance().setTheme(ThemeManager::ThemeId::DarkGlassmorphism);
  });
  connect(pastelThemeButton, &QPushButton::clicked, this, [this]() {
    ThemeManager::instance().setTheme(ThemeManager::ThemeId::PastelDream);
  });
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          &RightWidget::updateThemeButtonStates);
  connect(searchBox, &QLineEdit::textChanged, this,
          &RightWidget::onSearchTextChanged);

  connect(fileTreeView, &QTreeView::customContextMenuRequested, this,
          &RightWidget::onCustomContextMenuRequested);
  connect(fileListView, &QListView::customContextMenuRequested, this,
          &RightWidget::onCustomContextMenuRequested);
  connect(searchResultsView, &QListView::customContextMenuRequested, this,
          &RightWidget::onCustomContextMenuRequested);
  connect(queueListView, &QListView::customContextMenuRequested, this,
          &RightWidget::onCustomContextMenuRequested);
}

void RightWidget::onSearchTextChanged(const QString &text) {
  if (currentSearchWorker) {
    currentSearchWorker->stop();
    currentSearchWorker->wait();
    currentSearchWorker->deleteLater();
    currentSearchWorker = nullptr;
  }

  if (highlightDelegate) {
    highlightDelegate->setSearchQuery(text);
    fileTreeView->viewport()->update();
    fileListView->viewport()->update();
    searchResultsView->viewport()->update();
  }

  if (text.isEmpty()) {
    viewStack->setCurrentIndex(lastViewIndex);
    return;
  }

  if (viewStack->currentIndex() != 2) {
    lastViewIndex = viewStack->currentIndex();
  }

  viewStack->setCurrentIndex(2);
  searchResultsModel->clear();

  QString currentRoot = getCurrentFolderPath();
  if (currentRoot.isEmpty()) {
    currentRoot = QDir::homePath();
  }

  currentSearchWorker = new SearchWorker(currentRoot, text, this);
  connect(currentSearchWorker, &SearchWorker::fileFound, this,
          &RightWidget::onSearchFileFound);
  // Removed automatic deleteLater to prevent dangling pointers, RightWidget now
  // fully manages the worker's lifecycle.

  currentSearchWorker->start();
}

void RightWidget::onSearchFileFound(const QString &filePath,
                                    const QString &fileName) {
  QFileInfo info(filePath);

  if (!info.isFile() || info.suffix().toLower() != "mp3") {
    return;
  }

  CustomIconProvider iconProvider;
  QIcon icon = iconProvider.icon(info);

  QStandardItem *item = new QStandardItem(icon, fileName);
  // Path string
  item->setData(filePath, QFileSystemModel::FilePathRole);
  // Additional data for double click
  item->setData(info.isDir(), Qt::UserRole + 2);

  searchResultsModel->appendRow(item);
}

void RightWidget::onSearchResultsDoubleClicked(const QModelIndex &index) {
  if (!index.isValid())
    return;

  QString filePath =
      searchResultsModel->data(index, QFileSystemModel::FilePathRole)
          .toString();
  bool isDir = searchResultsModel->data(index, Qt::UserRole + 2).toBool();

  if (isDir) {
    searchBox->clear();
    emit listFolderNavigationRequested(filePath);
  } else {
    QFileInfo fileInfo(filePath);
    if (fileInfo.suffix().toLower() == "mp3") {
      emit trackPlayRequested(filePath);
    }
  }
}

void RightWidget::onCustomContextMenuRequested(const QPoint &pos) {
  QWidget *senderView = qobject_cast<QWidget *>(sender());
  if (!senderView)
    return;

  QModelIndex index;
  QString filePath;

  if (auto *tree = qobject_cast<QTreeView *>(senderView)) {
    index = tree->indexAt(pos);
    if (index.isValid() && treeProxyModel && treeSourceModel) {
      filePath = treeSourceModel->filePath(treeProxyModel->mapToSource(index));
    }
  } else if (auto *list = qobject_cast<QListView *>(senderView)) {
    index = list->indexAt(pos);
    if (index.isValid()) {
      if (list == fileListView && listProxyModel && listSourceModel) {
        filePath =
            listSourceModel->filePath(listProxyModel->mapToSource(index));
      } else if (list == searchResultsView) {
        filePath =
            searchResultsModel->data(index, QFileSystemModel::FilePathRole)
                .toString();
      } else if (list == queueListView) {
        filePath =
            queueModel->data(index, QFileSystemModel::FilePathRole).toString();
      }
    }
  }

  if (!index.isValid() || filePath.isEmpty())
    return;

  QMenu menu(this);
  menu.setObjectName("contextMenu");

  QAction *playNow = menu.addAction("Play Now");
  QAction *playNext = menu.addAction("Play Next");
  QAction *addToQueue = menu.addAction("Add to Queue");

  menu.addSeparator();

  QAction *openLocation = menu.addAction("Open File Location");

  QAction *renameFile = nullptr;
  if (senderView == fileTreeView || senderView == fileListView) {
    menu.addSeparator();
    renameFile = menu.addAction("Rename File");
  }

  QAction *selected = menu.exec(senderView->mapToGlobal(pos));
  if (!selected)
    return;

  if (selected == playNow)
    emit trackPlayRequested(filePath);
  else if (selected == playNext)
    emit playNextRequested(filePath);
  else if (selected == addToQueue)
    emit addToQueueRequested(filePath);
  else if (selected == openLocation)
    emit openFileLocationRequested(filePath);
  else if (renameFile && selected == renameFile) {
    if (auto *itemView = qobject_cast<QAbstractItemView *>(senderView)) {
      itemView->setCurrentIndex(index);
      // Trigger true in-place rename in the active view.
      itemView->edit(index);
    }
  }
}

void RightWidget::applySize(int pointSize) {
  QFont font = this->font(); // Берем текущий шрифт виджета
  font.setPointSize(pointSize);

  fileTreeView->setFont(font);
  fileListView->setFont(font);
  searchResultsView->setFont(font);
  queueListView->setFont(font);

  int iconSizeDim = static_cast<int>(pointSize * 1.45);
  QSize newIconSize(iconSizeDim, iconSizeDim);
  fileTreeView->setIconSize(newIconSize);
  fileListView->setIconSize(newIconSize);
  searchResultsView->setIconSize(newIconSize);
  queueListView->setIconSize(newIconSize);

  const int verticalPadding = qMax(1, pointSize / 4);
  fileTreeView->setStyleSheet(
      QString("QTreeView::item { padding: %1px 0px; }").arg(verticalPadding));
  fileListView->setStyleSheet(
      QString("QListView::item { padding: %1px 0px; }").arg(verticalPadding));
  searchResultsView->setStyleSheet(
      QString("QListView::item { padding: %1px 0px; }").arg(verticalPadding));
  queueListView->setStyleSheet(
      QString("QListView::item { padding: %1px 0px; }").arg(verticalPadding));

  fileTreeView->requestDelayedItemsLayout();
  fileListView->requestDelayedItemsLayout();
  searchResultsView->requestDelayedItemsLayout();
  queueListView->requestDelayedItemsLayout();
}

void RightWidget::refreshTheme() {
  const QColor tintColor = ThemeManager::instance().getIconTintColor();
  treeButton->setIcon(tintedIcon(":/assets/tree.png", tintColor));
  listButton->setIcon(tintedIcon(":/assets/list.png", tintColor));
  refreshQueueButtonIcon();
  updateThemeButtonStates();
}

void RightWidget::updateThemeButtonStates() {
  const bool isDark = ThemeManager::instance().currentTheme() ==
                      ThemeManager::ThemeId::DarkGlassmorphism;
  darkThemeButton->setChecked(isDark);
  pastelThemeButton->setChecked(!isDark);
}

QIcon RightWidget::tintedIcon(const QString &iconPath,
                              const QColor &color) const {
  QPixmap src(iconPath);
  QPixmap tinted(src.size());
  tinted.fill(Qt::transparent);

  QPainter painter(&tinted);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.drawPixmap(0, 0, src);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(tinted.rect(), color);

  return QIcon(tinted);
}
void RightWidget::toggleFileView() {
  searchBox->clear();
  lastViewIndex = 0;
  viewStack->setCurrentIndex(0);
}

void RightWidget::toggleListView() {
  searchBox->clear();
  lastViewIndex = 1;
  viewStack->setCurrentIndex(1);
}

void RightWidget::toggleQueueView() {
  searchBox->clear();
  lastViewIndex = 3;
  viewStack->setCurrentIndex(3);
  isBadgeVisible = false;
  refreshQueueButtonIcon();
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
  if (!treeSourceModel || !treeProxyModel)
    return;

  // Запускаем сканирование папки
  treeSourceModel->setRootPath(folderPath);

  // Конвертируем индексы и обновляем интерфейс
  QModelIndex sourceIndex = treeSourceModel->index(folderPath);
  QModelIndex proxyIndex = treeProxyModel->mapFromSource(sourceIndex);

  fileTreeView->setRootIndex(proxyIndex);
}
void RightWidget::setListFolder(const QString &folderPath) {
  if (!listSourceModel || !listProxyModel)
    return;

  listSourceModel->setRootPath(folderPath);
  QModelIndex sourceIndex = listSourceModel->index(folderPath);
  QModelIndex proxyIndex = listProxyModel->mapFromSource(sourceIndex);
  fileListView->setRootIndex(proxyIndex);
}
void RightWidget::selectFileInTree(const QString &filePath) {
  if (!treeSourceModel || !treeProxyModel)
    return;

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
    queueListView->viewport()->update();
    searchResultsView->viewport()->update();
  }
}
QString RightWidget::getCurrentFolderPath() const {
  if (treeSourceModel) {
    return treeSourceModel->rootPath();
  }
  return QString();
}
void RightWidget::setTreeRootPath(const QString &path) {
  setTreeFolder(path);
  setListFolder(path); // Update both views to match the saved path
}

void RightWidget::setQueue(const std::deque<QString> &queue) {
  if (!queueModel)
    return;

  // If new items were added, show the badge
  if ((int)queue.size() > queueModel->rowCount()) {
    isBadgeVisible = true;
  }

  queueModel->clear();

  CustomIconProvider iconProvider;
  for (const QString &filePath : queue) {
    QFileInfo info(filePath);
    QIcon icon = iconProvider.icon(info);
    QStandardItem *item = new QStandardItem(icon, info.fileName());
    item->setData(filePath, QFileSystemModel::FilePathRole);
    queueModel->appendRow(item);
  }
  refreshQueueButtonIcon();
}

void RightWidget::refreshQueueButtonIcon() {
  if (!queueButton)
    return;

  const QColor tintColor = ThemeManager::instance().getIconTintColor();
  QIcon baseIcon = tintedIcon(":/assets/queue.png", tintColor);

  if (isBadgeVisible && queueModel && queueModel->rowCount() > 0) {
    QPixmap basePixmap = baseIcon.pixmap(QSize(20, 20));

    // Load original heart icon without tinting
    QPixmap heartPixmap(":/assets/heart_icon.png");
    if (!heartPixmap.isNull()) {
      heartPixmap = heartPixmap.scaled(10, 10, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);

      QPainter painter(&basePixmap);
      painter.drawPixmap(basePixmap.width() - heartPixmap.width(), 0,
                         heartPixmap);
      painter.end();
    }

    queueButton->setIcon(QIcon(basePixmap));
  } else {
    queueButton->setIcon(baseIcon);
  }
}

void RightWidget::onQueueDoubleClicked(const QModelIndex &index) {
  if (!index.isValid())
    return;
  QString filePath =
      queueModel->data(index, QFileSystemModel::FilePathRole).toString();
  if (!filePath.isEmpty()) {
    emit queueTrackPlayRequested(index.row());
  }
}

bool RightWidget::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    if (searchBox && searchBox->hasFocus()) {
      QWidget *clickedWidget = qobject_cast<QWidget *>(watched);
      // If the clicked widget is not the searchBox and not a child of it (like
      // clear button)
      if (clickedWidget && clickedWidget != searchBox &&
          !searchBox->isAncestorOf(clickedWidget)) {
        searchBox->clearFocus();
      }
    }
  } else if (watched == searchBox && event->type() == QEvent::FocusIn) {
    // Select all text on focus for easier searching (fulfills "selected"
    // requirement)
    QTimer::singleShot(0, searchBox, &QLineEdit::selectAll);
  }
  return QWidget::eventFilter(watched, event);
}

void RightWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  // Включаем сглаживание, чтобы углы были красивыми и не пиксельными
  painter.setRenderHint(QPainter::Antialiasing);

  // Задаем цвет заливки из темы
  painter.setBrush(ThemeManager::instance().getPanelColor());

  // Включаем обводку из темы
  painter.setPen(QPen(ThemeManager::instance().getPanelBorderColor(), 1));
  // Рисуем прямоугольник со скруглением
  painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);
}