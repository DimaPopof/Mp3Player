#include "FileBrowserModelFactory.h"
#include "CustomSortProxyModel.h" 
#include <QFileSystemModel>

// The base method receives the specific filters as an argument
QAbstractItemModel *FileBrowserModelFactory::createBaseModel(QObject *parent, QDir::Filters filters) {
    
    QFileSystemModel *model = new QFileSystemModel(parent);
    model->setNameFilters(QStringList() << "*.mp3");
    model->setNameFilterDisables(false);
    
    // Apply the requested filters immediately
    model->setFilter(filters);
    
    CustomSortProxyModel *proxy = new CustomSortProxyModel(parent);
    proxy->setSourceModel(model);
    proxy->setDynamicSortFilter(true);
    proxy->sort(0);
    
    return proxy;
}

// Tree method passes its specific filters
QAbstractItemModel *FileBrowserModelFactory::createTreeModel(QObject *parent) {
    return createBaseModel(parent, QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
}

// List method passes its specific filters
QAbstractItemModel *FileBrowserModelFactory::createListModel(QObject *parent) {
    return createBaseModel(parent, QDir::AllDirs | QDir::Files | QDir::NoDot);
}