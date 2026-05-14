#ifndef FILEBROWSERMODELFACTORY_H
#define FILEBROWSERMODELFACTORY_H

#include <QAbstractItemModel>
#include <QDir>

class FileBrowserModelFactory {
public:
    static QAbstractItemModel *createTreeModel(QObject *parent = nullptr);
    static QAbstractItemModel *createListModel(QObject *parent = nullptr);

private:
    static QAbstractItemModel *createBaseModel(QObject *parent, QDir::Filters filters);
};

#endif // FILEBROWSERMODELFACTORY_H
