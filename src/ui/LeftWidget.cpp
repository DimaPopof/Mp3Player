#include "LeftWidget.h"
#include "core/AudioPlayer.h"
#include <QEvent>
#include <QTimer>

namespace {
constexpr int kTrackInfoMinHeight = 170;
constexpr qreal kTrackInfoAspectThreshold = 3.25;
} // namespace

LeftWidget::LeftWidget(QWidget *parent)
    : QWidget(parent), leftLayout(nullptr), trackInfoLayout(nullptr),
      trackInfoContainer(nullptr), visualizer(nullptr) {
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
  setMinimumWidth(420); // Set minimum width on the widget to properly stop the QSplitter
  setObjectName("leftWidget");
  setAttribute(Qt::WA_StyledBackground, true);
  setupUi();
  setupConnections();
}

void LeftWidget::setupUi() {
  leftLayout = new QVBoxLayout(this);
  leftLayout->setContentsMargins(0, 0, 0, 0);

  QHBoxLayout *folderButtonsLayout = new QHBoxLayout();

  openFolderButton = new QPushButton(this);
  openFolderButton->setObjectName("openFolderButton");
  openFolderButton->setIcon(QIcon(":/assets/folder.png"));
  openFolderButton->setFixedSize(50, 50);
  openFolderButton->setIconSize(QSize(45, 45));
  openFolderButton->setFlat(true);

  folderButtonsLayout->addWidget(openFolderButton);
  folderButtonsLayout->addStretch();

  coverLabel = new QLabel(this);
  coverLabel->setAlignment(Qt::AlignCenter);
  coverLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  coverLabel->setMinimumSize(50, 50);
  coverLabel->setMaximumHeight(300);

  visualizer = new Visualizer(this);
  visualizer->setObjectName("visualizerWidget");
  visualizer->setMinimumHeight(50);
  visualizer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  // 2. Настройка текста
  metadataLabel = new MarqueeLabel("Artist - Title", this);
  metadataLabel->setObjectName("metadataLabel");
  metadataLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  // Removed setAlignment and setWordWrap since MarqueeLabel handles it
  // differently
  metadataLabel->setMinimumHeight(40);

  trackInfoContainer = new QWidget(this);
  trackInfoContainer->setObjectName("trackInfoContainer");
  trackInfoContainer->setSizePolicy(QSizePolicy::MinimumExpanding,
                                    QSizePolicy::Fixed);
  // Removed trackInfoContainer->setMinimumWidth() to let LeftWidget handle resizing naturally
  trackInfoContainer->setFixedHeight(kTrackInfoMinHeight);

  // Создаем Layout и сразу привязываем его к нашему новому контейнеру
  trackInfoLayout = new QVBoxLayout(trackInfoContainer);
  trackInfoLayout->setContentsMargins(0, 0, 0, 0);
  trackInfoLayout->setSpacing(0);

  QFrame *separator = new QFrame(trackInfoContainer);
  separator->setObjectName("trackSeparator");
  separator->setFixedHeight(3);

  // Добавляем элементы внутрь Layout'а (который живет в контейнере)
  trackInfoLayout->addWidget(visualizer);
  trackInfoLayout->addWidget(separator);
  trackInfoLayout->addWidget(metadataLabel);

  // 4. Финальная сборка
  leftLayout->addLayout(folderButtonsLayout, 0);
  leftLayout->setAlignment(folderButtonsLayout, Qt::AlignTop);
  leftLayout->addWidget(coverLabel, 1);

  // ВАЖНО: Теперь мы добавляем в левую панель сам ВИДЖЕТ-КОНТЕЙНЕР, а не
  // Layout!
  leftLayout->addWidget(trackInfoContainer, 0);
  updateTrackInfoContainerHeight();
}

void LeftWidget::setupConnections() {
  connect(openFolderButton, &QPushButton::clicked, this,
          &LeftWidget::openFolderRequested);
}
void LeftWidget::updateTrackDisplay(const TrackInfo &trackInfo) {
  // 1. Обновляем текст
  metadataLabel->setText(trackInfo.artist + " - " + trackInfo.title);

  // 2. Обновляем обложку
  currentCoverPixmap = trackInfo.cover;
  updateCoverImage();
}
void LeftWidget::updateCoverImage() {
  if (!currentCoverPixmap.isNull()) {
    // Get the current dimensions of the label
    QSize currentSize = coverLabel->size();

    // FastTransformation is crucial here to prevent UI freezes on Linux/Wayland
    // when dragging fast to trigger window snapping resizes.
    QPixmap scaledPixmap = currentCoverPixmap.scaled(
        currentSize, Qt::KeepAspectRatio, Qt::FastTransformation);

    coverLabel->setPixmap(scaledPixmap);
  } else {
    coverLabel->clear();
  }
}
void LeftWidget::resetTrackDisplay() {
  metadataLabel->setText("Artist - Title");
  currentCoverPixmap = QPixmap();
  updateCoverImage();
}

void LeftWidget::updateTrackInfoContainerHeight() {
  if (!trackInfoContainer) {
    return;
  }

  const int currentWidth = trackInfoContainer->width();
  if (currentWidth <= 0) {
    return;
  }

  const int targetHeight =
      qMax(kTrackInfoMinHeight,
           static_cast<int>(currentWidth / kTrackInfoAspectThreshold));

  if (trackInfoContainer->height() != targetHeight) {
    // Defer the setFixedHeight call to avoid an infinite resize layout loop
    // which can deadlock Wayland compositors during fast snap actions.
    QTimer::singleShot(0, this, [this, targetHeight]() {
      if (trackInfoContainer && trackInfoContainer->height() != targetHeight) {
        trackInfoContainer->setFixedHeight(targetHeight);
      }
    });
  }
}

void LeftWidget::applyFontSize(int pointSize) {
  QFont font = this->font(); // Берем текущий шрифт виджета
  font.setPointSize(pointSize);

  metadataLabel->setFont(font);
  coverLabel->setFont(font);
}

bool LeftWidget::eventFilter(QObject *obj, QEvent *event) {
  if (obj == coverLabel && event->type() == QEvent::Resize) {
    updateCoverImage();
  }
  // Передаем остальные события базовому классу
  return QWidget::eventFilter(obj, event);
}

void LeftWidget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  updateTrackInfoContainerHeight();
}
