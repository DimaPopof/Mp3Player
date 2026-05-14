#ifndef TRACKMETADATA_H
#define TRACKMETADATA_H

#include <QString>
#include <QPixmap>

struct TrackInfo {
    QString artist;
    QString title;
    QPixmap cover;
};

class TrackMetadataExtractor {
public:
    static TrackInfo extract(const QString &filePath);
};

#endif // TRACKMETADATA_H
