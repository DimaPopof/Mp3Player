#include "RightWidget.h"
#include "ThemeManager.h"
#include "models/CustomSortProxyModel.h"
#include "models/FileBrowserModelFactory.h"
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QStackedWidget>
#include <QTreeView>
#include <QVBoxLayout>

RightWidget::RightWidget(QWidget *parent) : QWidget(parent) {
  setMinimumWidth(400);
  setObjectName("rightWidget");
  setAttribute(Qt::WA_StyledBackground, true);
  setupUi();
  setupConnections();
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          &RightWidget::refreshTheme);
  refreshTheme();
}

void RightWidget::setupUi() {
  QVBoxLayout *rightLayout = new QVBoxLayout(this);
  rightLayout->setContentsMargins(10, rightLayout->spacing(), 0, 10);

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
  }
  fileTreeView = new VsCodeTreeView(viewStack);
  fileTreeView->setObjectName("fileTreeView");
  fileTreeView->setModel(treeProxyModel);
  highlightDelegate = new HighlightDelegate(this);
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
  fileTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  fileTreeView->setFocusPolicy(Qt::NoFocus); // Disable focus rectangle
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
  fileListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  fileListView->setSpacing(0);
  fileListView->setUniformItemSizes(true);
  fileListView->setFocusPolicy(Qt::NoFocus); // Disable focus rectangle
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
  searchResultsView->setAttribute(Qt::WA_MacShowFocusRect,
                                  false); // Disable focus on macOS
  searchResultsView->setStyleSheet(
      "QListView { outline: none; border: none; "
      "}"); // Extra
            // suppressionItemView::SingleSelection);
  QPalette searchPal = searchResultsView->palette();
  searchPal.setColor(QPalette::Highlight, Qt::transparent);
  searchPal.setColor(QPalette::HighlightedText, Qt::transparent);
  searchResultsView->setPalette(searchPal);
  searchResultsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  searchResultsView->setSpacing(0);
  searchResultsView->setUniformItemSizes(true);

  viewStack->addWidget(fileTreeView);      // index 0
  viewStack->addWidget(fileListView);      // index 1
  viewStack->addWidget(searchResultsView); // index 2

  QHBoxLayout *buttonsLayout = new QHBoxLayout();
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

  darkThemeButton = new QPushButton("Dark", this);
  darkThemeButton->setObjectName("darkThemeButton");
  darkThemeButton->setCheckable(true);

  pastelThemeButton = new QPushButton("Pastel", this);
  pastelThemeButton->setObjectName("pastelThemeButton");
  pastelThemeButton->setCheckable(true);

  searchBox = new QLineEdit(this);
  searchBox->setObjectName("searchBox");
  searchBox->setPlaceholderText("Search...");
  searchBox->setClearButtonEnabled(true);

  buttonsLayout->addWidget(treeButton);
  buttonsLayout->addWidget(listButton);
  buttonsLayout->addWidget(darkThemeButton);
  buttonsLayout->addWidget(pastelThemeButton);
  buttonsLayout->addStretch();
  buttonsLayout->addWidget(searchBox);
  buttonsLayout->addSpacing(10);

  QFrame *separator = new QFrame(this);
  separator->setObjectName("separatorLine");
  separator->setFrameShape(QFrame::HLine);
  separator->setFrameShadow(QFrame::Plain);

  rightLayout->addLayout(buttonsLayout);
  rightLayout->addWidget(separator);
  rightLayout->addWidget(viewStack, 10);
}

void RightWidget::setupConnections() {

  connect(fileTreeView, &QTreeView::doubleClicked, this,
          &RightWidget::treeViewDoubleClicked);
  connect(fileListView, &QListView::doubleClicked, this,
          &RightWidget::onListDoubleClicked);
  connect(searchResultsView, &QListView::doubleClicked, this,
          &RightWidget::onSearchResultsDoubleClicked);
  connect(treeButton, &QPushButton::clicked, this,
          &RightWidget::toggleFileView);
  connect(listButton, &QPushButton::clicked, this,
          &RightWidget::toggleListView);
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
}

void RightWidget::onSearchTextChanged(const QString &text) {
  if (currentSearchWorker) {
    currentSearchWorker->stop();
    currentSearchWorker->wait();
    currentSearchWorker->deleteLater();
    currentSearchWorker = nullptr;
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

void RightWidget::applySize(int pointSize) {
  QFont font = this->font(); // Берем текущий шрифт виджета
  font.setPointSize(pointSize);

  fileTreeView->setFont(font);
  fileListView->setFont(font);
  searchResultsView->setFont(font);

  int iconSizeDim = static_cast<int>(pointSize * 1.45);
  QSize newIconSize(iconSizeDim, iconSizeDim);
  fileTreeView->setIconSize(newIconSize);
  fileListView->setIconSize(newIconSize);
  searchResultsView->setIconSize(newIconSize);

  const int verticalPadding = qMax(1, pointSize / 4);
  fileTreeView->setStyleSheet(
      QString("QTreeView::item { padding: %1px 0px; }").arg(verticalPadding));
  fileListView->setStyleSheet(
      QString("QListView::item { padding: %1px 0px; }").arg(verticalPadding));
  searchResultsView->setStyleSheet(
      QString("QListView::item { padding: %1px 0px; }").arg(verticalPadding));

  fileTreeView->requestDelayedItemsLayout();
  fileListView->requestDelayedItemsLayout();
  searchResultsView->requestDelayedItemsLayout();
}

void RightWidget::refreshTheme() {
  const QColor tintColor = ThemeManager::instance().getIconTintColor();
  treeButton->setIcon(tintedIcon(":/assets/tree.png", tintColor));
  listButton->setIcon(tintedIcon(":/assets/list.png", tintColor));
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