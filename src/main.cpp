#include <QApplication>
#include <QIcon>
#include <QStyleFactory>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setStyle(QStyleFactory::create("Fusion"));

    const QString globalQss =
        "QTreeView, QListView {"
        "background: transparent;"
        "color: #CCCCCC;"
        "border: none;"
        "outline: 0;"
        "}"
        "QTreeView::item:selected, QListView::item:selected {"
        "background-color: #04395E;"
        "color: #FFFFFF;"
        "}"
        "QTreeView::item:hover, QListView::item:hover {"
        "background-color: rgba(255, 255, 255, 0.05);"
        "}";
    app.setStyleSheet(globalQss);

    // Apply the icon from the compiled resource file to the application
    app.setWindowIcon(QIcon(":/assets/app_icon.png"));

    MainWindow window;
    window.show();

    return app.exec();
}