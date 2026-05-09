#include "CustomSortProxyModel.h"
#include <QString>
#include <QFileSystemModel>

CustomSortProxyModel::CustomSortProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
    setRecursiveFilteringEnabled(true);
}

bool CustomSortProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    // 1. Keep ".." permanently visible for easy navigation up
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    QString dataStr = sourceModel()->data(index).toString();
    if (dataStr == "..") {
        return true;
    }

    // 2. Delegate to the builtin QSortFilterProxyModel which handles recursive checks safely
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool CustomSortProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    if (left.column() == 0 && right.column() == 0) {
        
        // 1. Get the source file system model
        QFileSystemModel *fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        
        // 2. Read the text strings and preserve '..' above all other entries
        QString leftStr = sourceModel()->data(left).toString();
        QString rightStr = sourceModel()->data(right).toString();

        if (leftStr == ".." && rightStr != "..") {
            return true;
        }
        if (rightStr == ".." && leftStr != "..") {
            return false;
        }

        // 3. Prioritize directories over files
        if (fsModel) {
            bool leftIsDir = fsModel->isDir(left);
            bool rightIsDir = fsModel->isDir(right);

            if (leftIsDir != rightIsDir) {
                return leftIsDir; // Returns true if 'left' is a directory, pushing it up
            }
        }

        // 4. Validate values
        if (leftStr.isEmpty() || rightStr.isEmpty()) {
            return QSortFilterProxyModel::lessThan(left, right);
        }

        QChar leftChar = leftStr.at(0);
        QChar rightChar = rightStr.at(0);

        // 4. Assign categories
        int leftCategory = 2; // default: symbols
        if (leftChar.isDigit()) {
            leftCategory = 0; // digits
        } else if ((leftChar >= 'A' && leftChar <= 'Z') || (leftChar >= 'a' && leftChar <= 'z')) {
            leftCategory = 1; // Latin letters
        }

        int rightCategory = 2;
        if (rightChar.isDigit()) {
            rightCategory = 0;
        } else if ((rightChar >= 'A' && rightChar <= 'Z') || (rightChar >= 'a' && rightChar <= 'z')) {
            rightCategory = 1;
        }

        // 5. Compare categories
        if (leftCategory != rightCategory) {
            return leftCategory < rightCategory;
        }

        // 6. Use locale-aware comparison for the same category
        return QString::localeAwareCompare(leftStr, rightStr) < 0;
    }

    // For other columns, use default sorting
    return QSortFilterProxyModel::lessThan(left, right);
}