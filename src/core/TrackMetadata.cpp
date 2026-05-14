#include "TrackMetadata.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <QFileInfo>
#include <QDir>

TrackInfo TrackMetadataExtractor::extract(const QString &filePath) {
    TrackInfo info;
    info.artist = "Unknown Artist";
    info.title = QFileInfo(filePath).fileName();

#ifdef _WIN32
    std::wstring wPath = QDir::toNativeSeparators(filePath).toStdWString();
    const wchar_t *encodedPath = wPath.c_str();
#else
    QByteArray pathBytes = filePath.toUtf8();
    const char *encodedPath = pathBytes.constData();
#endif

    TagLib::FileRef fileRef(encodedPath);
    if (!fileRef.isNull() && fileRef.tag()) {
        if (!fileRef.tag()->artist().isEmpty()) {
            info.artist = QString::fromUtf8(fileRef.tag()->artist().toCString(true));
        }
        if (!fileRef.tag()->title().isEmpty()) {
            info.title = QString::fromUtf8(fileRef.tag()->title().toCString(true));
        }
    }

    if (!fileRef.isNull()) {
        TagLib::MPEG::File *mpegFile = dynamic_cast<TagLib::MPEG::File *>(fileRef.file());
        if (mpegFile && mpegFile->isValid()) {
            TagLib::ID3v2::Tag *id3v2Tag = mpegFile->ID3v2Tag();
            if (id3v2Tag) {
                TagLib::ID3v2::FrameList frames = id3v2Tag->frameListMap()["APIC"];
                if (!frames.isEmpty()) {
                    auto *picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames.front());
                    if (picFrame) {
                        QByteArray imageData(picFrame->picture().data(), picFrame->picture().size());
                        info.cover.loadFromData(imageData);
                    }
                }
            }
        }
    }

    return info;
}
