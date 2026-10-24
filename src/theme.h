#ifndef AVESTAN_THEME_H
#define AVESTAN_THEME_H

#include <QString>

class QApplication;
class Config;
struct ThemePalette;

// Applies the metallic dark or light palette to the whole application, or
// leaves the platform/system colours in place for the "System" option.
namespace Theme {
void apply(QApplication &app, const Config &config, const QString &themeName);
}

#endif // AVESTAN_THEME_H
