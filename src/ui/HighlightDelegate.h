#ifndef HIGHLIGHTDELEGATE_H
#define HIGHLIGHTDELEGATE_H

#include "ThemeManager.h"
#include <QFileInfo>
#include <QFileSystemModel>
#include <QPainter>
#include <QString>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QTreeView>

class HighlightDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit HighlightDelegate(QObject *parent = nullptr)
      : QStyledItemDelegate(parent) {}

  void setPlayingFilePath(const QString &path) {
    // Stores the file that should be shown as currently playing.
    playingFilePath = path;
  }

  void setSearchQuery(const QString &query) { searchQuery = query; }

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override {
    // Use default size hint without any extra styling artifacts
    QSize size = option.decorationSize;
    if (!size.isValid()) {
      size = QSize(16, 16);
    }
    size.rheight() = qMax(size.height() + 8, option.fontMetrics.height() + 8);
    return size;
  }

protected:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    // Get necessary info without calling initStyleOption to avoid default
    // styling artifacts
    const QString itemPath =
        index.data(QFileSystemModel::FilePathRole).toString();
    const bool isFile = QFileInfo(itemPath).isFile();
    const bool isTreeView =
        qobject_cast<const QTreeView *>(option.widget) != nullptr;
    const bool isPlaying =
        !playingFilePath.isEmpty() && itemPath == playingFilePath;

    // Get state flags directly from option
    const bool isSelected = option.state.testFlag(QStyle::State_Selected);
    const bool isHovered = option.state.testFlag(QStyle::State_MouseOver);

    // Fixed colors from ThemeManager
    const QRect rowRect = option.rect;
    QRect fullRowRect = rowRect;
    if (option.widget) {
      fullRowRect.setLeft(0);
      fullRowRect.setWidth(option.widget->width());
    }

    const QColor hoverColor = ThemeManager::instance().getHoverColor();
    const QColor selectedColor = ThemeManager::instance().getSelectionColor();
    const QColor playingColor = ThemeManager::instance().getPlayingColor();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    // Draw background highlight only
    if (isPlaying) {
      painter->setBrush(playingColor);
      painter->drawRoundedRect(fullRowRect, 8, 8);
    } else if (isSelected) {
      painter->setBrush(selectedColor);
      painter->drawRoundedRect(fullRowRect, 8, 8);
    } else if (isHovered) {
      painter->setBrush(hoverColor);
      painter->drawRoundedRect(fullRowRect, 8, 8);
    }

    // Get icon and draw it
    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    const int indentation = option.widget ? option.widget->style()->pixelMetric(
                                                QStyle::PM_TreeViewIndentation,
                                                nullptr, option.widget)
                                          : 0;
    const int iconWidth =
        option.decorationSize.isValid() ? option.decorationSize.width() : 16;
    const int iconHeight =
        option.decorationSize.isValid() ? option.decorationSize.height() : 16;
    const int iconLeft =
        rowRect.left() + 4 + ((isFile && isTreeView) ? -indentation : 0);
    const QRect iconRect(iconLeft,
                         rowRect.top() + (rowRect.height() - iconHeight) / 2,
                         iconWidth, iconHeight);

    if (!icon.isNull()) {
      const QIcon::Mode mode =
          isPlaying ? QIcon::Normal
                    : (isSelected ? QIcon::Selected : QIcon::Normal);
      icon.paint(painter, iconRect, Qt::AlignCenter, mode, QIcon::Off);
    }

    // Draw text
    const int textLeft = iconRect.right() + 8;
    QRect textRect(textLeft, rowRect.top(), rowRect.right() - textLeft - 4,
                   rowRect.height());

    // Text contrast: use dark text on bright selected/playing backgrounds,
    // normal theme text color otherwise
    QColor textColor;
    if (isPlaying || isSelected) {
      textColor = ThemeManager::instance().getMediaIconColor();
    } else {
      textColor = ThemeManager::instance().getTextColor();
    }

    QFont font = painter->font();

    if (isPlaying) {
      font.setBold(true);
    }
    painter->setFont(font);

    QString text = index.data(Qt::DisplayRole).toString();

    if (!searchQuery.isEmpty() &&
        text.contains(searchQuery, Qt::CaseInsensitive)) {
      painter->save();

      QTextLayout layout(text, font);
      layout.beginLayout();
      QTextLine line = layout.createLine();
      layout.endLayout();

      QList<QTextLayout::FormatRange> selections;
      QColor accent = ThemeManager::instance().getAccentColor();
      QColor contrastText = ThemeManager::instance().getMediaIconColor();

      int idx = text.indexOf(searchQuery, 0, Qt::CaseInsensitive);
      while (idx != -1) {
        QTextLayout::FormatRange range;
        range.start = idx;
        range.length = searchQuery.length();
        range.format.setBackground(accent);
        range.format.setForeground(contrastText);
        selections.append(range);

        idx = text.indexOf(searchQuery, idx + searchQuery.length(),
                           Qt::CaseInsensitive);
      }

      painter->setPen(textColor);

      // Calculate vertical centering
      int yOffset = (textRect.height() - line.height()) / 2;

      // We must offset the drawing by the layout's internal positions
      layout.draw(painter, QPoint(textRect.left(), textRect.top() + yOffset),
                  selections);

      painter->restore();
    } else {
      painter->setPen(textColor);
      painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    painter->restore();
  }

  bool editorEvent(QEvent *event, QAbstractItemModel *model,
                   const QStyleOptionViewItem &option,
                   const QModelIndex &index) override {
    // Suppress any focus rectangle drawing
    return QStyledItemDelegate::editorEvent(event, model, option, index);
  }

private:
  QString playingFilePath;
  QString searchQuery;
};

#endif // HIGHLIGHTDELEGATE_H