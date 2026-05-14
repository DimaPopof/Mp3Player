#include "ThemeManager.h"

#include <QApplication>
#include <QFile>
#include <QStyleFactory>

namespace {
struct ThemePalette {
  QColor textColor;
  QColor iconTintColor;
  QColor accentColor;
  QColor mutedIconColor;
  QColor separatorColor;
  QColor viewHighlightColor;
  QColor hoverColor;
  QColor selectionColor;
  QColor playingColor;
  QColor mediaIconColor;
  QColor panelColor;
  QColor panelBorderColor;
  CloudThemeConfig cloudConfig;
  QColor visualizerIdleColor;
};

ThemePalette paletteForTheme(ThemeManager::ThemeId theme) {
  switch (theme) {
  case ThemeManager::ThemeId::DarkGlassmorphism:
    return {
        QColor("#E9EDF5"), // textColor (звездный белый)
        QColor("#E9EDF5"), // iconTintColor (звездный белый)
        QColor("#79C0FF"), // accentColor (лунный голубой)
        QColor("#FF8FA3"), // mutedIconColor
        QColor(255, 255, 255, 56), // separatorColor
        QColor(4, 57, 94, 190),    // viewHighlightColor
        QColor(143, 183, 255, 40), // hoverColor (лунный голубой полупрозрачный)
        QColor(143, 183, 255, 60), // selectionColor
        QColor("#79C0FF"),         // playingColor (лунный голубой)
        QColor("#121926"), // mediaIconColor (полуночный синий)
        QColor(20, 26, 40, 240),   // panelColor (ночной туман)
        QColor(143, 183, 255, 30), // panelBorderColor
        {15, 45, 15, 50, QColor(255, 248, 210)}, // cloudConfig
        QColor(121, 192, 255, 60)                // visualizerIdleColor
    };
  case ThemeManager::ThemeId::PastelDream:
    return {
        QColor("#3B3542"),          // textColor
        QColor("#3B3542"),          // iconTintColor
        QColor("#FFA8B6"),          // accentColor
        QColor("#FF6666"),          // mutedIconColor
        QColor(200, 198, 250, 150), // separatorColor
        QColor(220, 208, 255, 170), // viewHighlightColor
        QColor(220, 208, 255, 120), // hoverColor
        QColor(255, 168, 182, 150), // selectionColor
        QColor("#FFA8B6"),          // playingColor
        QColor("#3B3542"),          // mediaIconColor
        QColor(255, 255, 255, 240), // panelColor (белый полупрозрачный)
        QColor(200, 198, 250, 200),                // panelBorderColor
        {30, 60, 100, 200, QColor(255, 255, 255)}, // cloudConfig
        QColor(255, 168, 182, 60)                  // visualizerIdleColor
    };
  }

  return {QColor("#E9EDF5"),
          QColor("#E9EDF5"),
          QColor("#79C0FF"),
          QColor("#FF8FA3"),
          QColor(255, 255, 255, 56),
          QColor(4, 57, 94, 190),
          QColor(143, 183, 255, 40),
          QColor(143, 183, 255, 60),
          QColor("#79C0FF"),
          QColor("#121926"),
          QColor(20, 26, 40, 240),
          QColor(143, 183, 255, 30),
          {15, 45, 15, 50, QColor(255, 248, 210)},
          QColor(121, 192, 255, 60)};
}
} // namespace

ThemeManager &ThemeManager::instance() {
  static ThemeManager manager;
  return manager;
}

ThemeManager::ThemeManager(QObject *parent) : QObject(parent) {}

#include <QSettings>

void ThemeManager::initialize(QApplication *app) {
  m_app = app;
  if (!m_app) {
    return;
  }

  m_app->setStyle(QStyleFactory::create("Fusion"));

  QSettings settings;
  int savedTheme =
      settings.value("appTheme", static_cast<int>(ThemeId::PastelDream))
          .toInt();
  setTheme(static_cast<ThemeId>(savedTheme));
}

QString ThemeManager::loadQss(ThemeId theme) const {
  const char *resourcePath = nullptr;

  switch (theme) {
  case ThemeId::DarkGlassmorphism:
    resourcePath = ":/themes/theme_dark.qss";
    break;
  case ThemeId::PastelDream:
    resourcePath = ":/themes/theme_pastel.qss";
    break;
  }

  QFile file(resourcePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  return QString::fromUtf8(file.readAll());
}

void ThemeManager::setTheme(ThemeId theme) {
  m_currentTheme = theme;

  QSettings settings;
  settings.setValue("appTheme", static_cast<int>(theme));

  if (m_app) {
    m_app->setStyleSheet(loadQss(theme));
  }

  emit themeChanged(theme);
}

ThemeManager::ThemeId ThemeManager::currentTheme() const {
  return m_currentTheme;
}

QColor ThemeManager::getTextColor() const {
  return paletteForTheme(m_currentTheme).textColor;
}

QColor ThemeManager::getIconTintColor() const {
  return paletteForTheme(m_currentTheme).iconTintColor;
}

QColor ThemeManager::getAccentColor() const {
  return paletteForTheme(m_currentTheme).accentColor;
}

QColor ThemeManager::getMutedIconColor() const {
  return paletteForTheme(m_currentTheme).mutedIconColor;
}

QColor ThemeManager::getSeparatorColor() const {
  return paletteForTheme(m_currentTheme).separatorColor;
}

QColor ThemeManager::getViewHighlightColor() const {
  return paletteForTheme(m_currentTheme).viewHighlightColor;
}

QColor ThemeManager::getHoverColor() const {
  return paletteForTheme(m_currentTheme).hoverColor;
}

QColor ThemeManager::getSelectionColor() const {
  return paletteForTheme(m_currentTheme).selectionColor;
}

QColor ThemeManager::getPlayingColor() const {
  return paletteForTheme(m_currentTheme).playingColor;
}

QColor ThemeManager::getMediaIconColor() const {
  return paletteForTheme(m_currentTheme).mediaIconColor;
}

QColor ThemeManager::getPanelColor() const {
  return paletteForTheme(m_currentTheme).panelColor;
}

QColor ThemeManager::getPanelBorderColor() const {
  return paletteForTheme(m_currentTheme).panelBorderColor;
}
CloudThemeConfig ThemeManager::getCloudConfig() const {
  return paletteForTheme(m_currentTheme).cloudConfig;
}

QColor ThemeManager::getVisualizerIdleColor() const {
  return paletteForTheme(m_currentTheme).visualizerIdleColor;
}
