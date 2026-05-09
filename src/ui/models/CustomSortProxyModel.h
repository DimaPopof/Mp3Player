#ifndef CUSTOMSORTPROXYMODEL_H
#define CUSTOMSORTPROXYMODEL_H

#include <QSortFilterProxyModel>

class CustomSortProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit CustomSortProxyModel(QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

#endif // CUSTOMSORTPROXYMODEL_H