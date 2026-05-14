#include "ui/MainWindow.h"
#include "ui/ThemeManager.h"
#include <QApplication>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
  QCoreApplication::setOrganizationName("MySoft");
  QCoreApplication::setApplicationName("Mp3Player");
  QApplication app(argc, argv);

  ThemeManager::instance().initialize(&app);

  // Apply the icon from the compiled resource file to the application
  app.setWindowIcon(QIcon(":/assets/app_icon.png"));

  MainWindow window;
  window.show();

  return app.exec();


  //test
}