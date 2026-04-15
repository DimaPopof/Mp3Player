#ifndef HIGHLIGHTDELEGATE_H
#define HIGHLIGHTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QString>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QTreeView>

class HighlightDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit HighlightDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void setPlayingFilePath(const QString &path) {
        // Stores the file that should be shown as currently playing.
        playingFilePath = path;
    }

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        // Current item path, used to decide hover and selection look.
        const QString itemPath = index.data(QFileSystemModel::FilePathRole).toString();
        // True when the row is a file inside the tree view.
        const bool isFile = QFileInfo(itemPath).isFile();
        // Tree view and list view use a slightly different layout.
        const bool isTreeView = qobject_cast<const QTreeView *>(opt.widget) != nullptr;
        // True when this row is the active playing file.
        const bool isPlaying = !playingFilePath.isEmpty() && itemPath == playingFilePath;

        // Fixed colors keep hover and selection the same on all systems.
        const QRect rowRect = opt.rect;
        QRect fullRowRect = rowRect;
        if (opt.widget) {
            // Fill the entire visible row width, not only the style-provided item rect.
            fullRowRect.setLeft(0);
            fullRowRect.setWidth(opt.widget->width());
        }
        const QColor baseColor(30, 30, 30, 0);
        const QColor hoverColor(255, 255, 255, 18);
        const QColor selectedColor(4, 57, 94, 255);
        const QColor playingColor(255, 102, 102, 255);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        if (isPlaying) {
            // Highlight the currently playing file.
            painter->fillRect(fullRowRect, playingColor);
        } else if (opt.state.testFlag(QStyle::State_Selected)) {
            // Highlight the selected row.
            painter->fillRect(fullRowRect, selectedColor);
        } else if (opt.state.testFlag(QStyle::State_MouseOver)) {
            // Light hover effect when the mouse is over the row.
            painter->fillRect(fullRowRect, hoverColor);
        } else {
            painter->fillRect(fullRowRect, baseColor);
        }

        // File icon from the model.
        const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        // Indent tree items so they line up with branches.
        const int indentation = opt.widget ? opt.widget->style()->pixelMetric(QStyle::PM_TreeViewIndentation, &opt, opt.widget) : 0;
        const int iconWidth = opt.decorationSize.isValid() ? opt.decorationSize.width() : 16;
        const int iconHeight = opt.decorationSize.isValid() ? opt.decorationSize.height() : 16;
        const int iconLeft = rowRect.left() + 4 + ((isFile && isTreeView) ? -indentation : 0);
        const QRect iconRect(iconLeft, rowRect.top() + (rowRect.height() - iconHeight) / 2, iconWidth, iconHeight);

        if (!icon.isNull()) {
            const QIcon::Mode mode = isPlaying ? QIcon::Normal : (opt.state.testFlag(QStyle::State_Selected) ? QIcon::Selected : QIcon::Normal);
            icon.paint(painter, iconRect, Qt::AlignCenter, mode, QIcon::Off);
        }

        // Text starts after the icon.
        const int textLeft = iconRect.right() + 8;
        QRect textRect(textLeft, rowRect.top(), rowRect.right() - textLeft - 4, rowRect.height());

        // Selected rows use white text, playing row uses black text.
        QColor textColor = opt.palette.color(QPalette::Text);
        if (isPlaying) {
            textColor = Qt::black;
        } else if (opt.state.testFlag(QStyle::State_Selected)) {
            textColor = Qt::white;
        }

        painter->setPen(textColor);
        painter->drawText(textRect, opt.displayAlignment, index.data(Qt::DisplayRole).toString());

        painter->restore();
    }

private:
    QString playingFilePath;
};

#endif // HIGHLIGHTDELEGATE_H