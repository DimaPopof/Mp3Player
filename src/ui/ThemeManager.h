#pragma once

#include <QObject>
#include <QColor>
#include <QString>

class QApplication;

struct CloudThemeConfig {
  int minRadius;
  int maxRadius;
  int minAlpha;
  int maxAlpha;
  QColor baseColor;
};

class ThemeManager final : public QObject {
    Q_OBJECT

public:
    enum class ThemeId {
        DarkGlassmorphism,
        PastelDream
    };
    Q_ENUM(ThemeId)

    static ThemeManager& instance();

    void initialize(QApplication* app);
    void setTheme(ThemeId theme);

    ThemeId currentTheme() const;
    CloudThemeConfig getCloudConfig() const;

    QColor getTextColor() const;
    QColor getIconTintColor() const;
    QColor getAccentColor() const;
    QColor getMutedIconColor() const;
    QColor getSeparatorColor() const;
    QColor getViewHighlightColor() const;
    QColor getHoverColor() const;
    QColor getSelectionColor() const;
    QColor getPlayingColor() const;
    QColor getMediaIconColor() const;
    QColor getPanelColor() const;
    QColor getPanelBorderColor() const;

signals:
    void themeChanged(ThemeId theme);

private:
    explicit ThemeManager(QObject* parent = nullptr);

    QString loadQss(ThemeId theme) const;

    ThemeId m_currentTheme { ThemeId::DarkGlassmorphism };
    QApplication* m_app { nullptr };
};