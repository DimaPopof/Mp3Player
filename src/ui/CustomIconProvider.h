#pragma once
#include <QAbstractFileIconProvider>
#include <QFileInfo>
#include <QIcon>
#include <QString>

class CustomIconProvider : public QAbstractFileIconProvider {
public:
    CustomIconProvider() = default;

    // Override the icon method to provide custom icons based on file details
    QIcon icon(const QFileInfo &info) const override {
        // Use existing bundled PNG assets from the qrc file.
        if (info.isDir()) {
            return QIcon(":/assets/folder.png");
        }
        
        // Show a dedicated icon for mp3 files.
        if (info.isFile() && info.suffix().toLower() == "mp3") {
            return QIcon(":/assets/mp3.png");
        }
        return QIcon(); // Return an invalid icon for unsupported file types

    }

    // You can also optionally override the icon(IconType type) method 
    // if you want to handle standard system icon requests (like Computer, Trash, etc.)
    // QIcon icon(IconType type) const override { ... }
};
