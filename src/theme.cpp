#include "theme.h"

#include "config.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

namespace {

QString colorName(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QString buildStyleSheet(const ThemePalette &palette)
{
    const QString background = colorName(palette.background);
    const QString input = colorName(palette.input);
    const QString button = colorName(palette.button);
    const QString primary = colorName(palette.primary);
    const QString foreground = colorName(palette.foreground);
    const QString hover = palette.hover.isValid() ? colorName(palette.hover) : primary;
    const QString border = palette.border.isValid() ? colorName(palette.border) : button;

    return QStringLiteral(
               "QMainWindow, QDialog { background: %1; color: %6; }\n"
               "QWidget { color: %6; }\n"
               "QWidget#TopBar { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 %3, stop:0.45 %1, stop:1 %3); }\n"
               "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 %3, stop:1 %1); color: %6; border: 1px solid %7; "
               "border-radius: 5px; padding: 5px 12px; }\n"
               "QPushButton:hover { background: %5; }\n"
               "QPushButton:pressed { background: %1; }\n"
               "QComboBox { background: %2; color: %6; border: 1px solid %7; "
               "border-radius: 5px; padding: 4px 8px; }\n"
               "QComboBox QAbstractItemView { background: %2; color: %6; "
               "selection-background-color: %4; }\n"
               "QPlainTextEdit { background: %2; color: %6; selection-background-color: %4; "
               "selection-color: %6; }\n"
               "QListView { background: %1; border: none; }\n"
               "QCheckBox { color: %6; }\n"
               "QLabel { color: %6; }\n"
               "QToolTip { background: %2; color: %6; border: 1px solid %7; }\n")
        .arg(background, input, button, primary, hover, foreground, border);
}

QPalette buildPalette(const ThemePalette &palette)
{
    QPalette result;
    result.setColor(QPalette::Window, palette.background);
    result.setColor(QPalette::WindowText, palette.foreground);
    result.setColor(QPalette::Base, palette.input);
    result.setColor(QPalette::AlternateBase, palette.background);
    result.setColor(QPalette::Text, palette.foreground);
    result.setColor(QPalette::Button, palette.button);
    result.setColor(QPalette::ButtonText, palette.foreground);
    result.setColor(QPalette::Highlight, palette.primary);
    result.setColor(QPalette::HighlightedText, palette.foreground);
    result.setColor(QPalette::ToolTipBase, palette.input);
    result.setColor(QPalette::ToolTipText, palette.foreground);
    result.setColor(QPalette::PlaceholderText, palette.foreground);
    return result;
}

} // namespace

void Theme::apply(QApplication &app, const Config &config, const QString &themeName)
{
    const ThemePalette *palette = config.themePalette(themeName);
    if (palette == nullptr || !palette->valid || palette->variant == QLatin1String("system")) {
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        app.setPalette(app.style()->standardPalette());
        app.setStyleSheet(QString());
        return;
    }

    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setPalette(buildPalette(*palette));
    app.setStyleSheet(buildStyleSheet(*palette));
}
