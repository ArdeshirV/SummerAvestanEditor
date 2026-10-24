#ifndef AVESTAN_CONFIG_H
#define AVESTAN_CONFIG_H

#include <QColor>
#include <QHash>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

class QJsonObject;

// A single contiguous block of code points with a verified reading per code
// point, e.g. the Avestan letters U+10B00..U+10B35.
struct ReadingRange {
    uint first = 0;
    uint last = 0;
    QStringList readings;
};

// All verified readings for one language. `special` covers code points whose
// reading is not a simple transliteration (numbers, punctuation, logograms).
struct ReadingSet {
    QVector<ReadingRange> ranges;
    QMap<uint, QString> special;
};

struct LanguageDefinition {
    QString name;
    QString sample;
    QVector<QPair<uint, uint>> ranges;
    bool afterUnicodeScripts = false;
};

struct ThemePalette {
    bool valid = false;
    QString variant;
    QColor background;
    QColor input;
    QColor button;
    QColor primary;
    QColor foreground;
    QColor hover;
    QColor border;
};

// Config loads the embedded JSON assets (app.json, languages.json,
// readings.json) once and exposes typed accessors. Keeping the strings and
// configurable numbers in assets means the C++ code has no scattered UI text
// or magic numbers.
class Config {
public:
    static const Config &instance();

    QString string(const QString &key) const;
    double number(const QString &key) const;
    QString preference(const QString &key) const;

    QStringList themeOptions() const { return m_themeOptions; }
    QStringList fontSizeOptions() const { return m_fontSizeOptions; }
    const ThemePalette *themePalette(const QString &name) const;
    bool rtlDefault() const { return m_rtlDefault; }

    const QVector<LanguageDefinition> &languages() const { return m_languages; }

    // Human readable option name including a script sample when available.
    QString languageOptionName(const LanguageDefinition &language) const;

    // Verified reading for a character, or an uppercase hexadecimal code point
    // when no source-backed reading exists.
    QString phoneticFor(const QString &languageName, uint codePoint) const;

    // Every printable code point of a language, sorted, with a trailing space.
    const QVector<uint> &charactersFor(const LanguageDefinition &language) const;

    // Application metadata.
    const QString &appId() const { return m_appId; }
    const QString &appName() const { return m_appName; }
    const QString &appVersion() const { return m_appVersion; }
    const QString &projectUrl() const { return m_projectUrl; }
    const QString &authorHandle() const { return m_authorHandle; }
    const QString &authorUrl() const { return m_authorUrl; }
    const QString &copyright() const { return m_copyright; }
    const QString &iconName() const { return m_iconName; }

    static QString normalizedLanguageName(const QString &name);

private:
    Config();

    void loadApplication(const QJsonObject &root);
    void loadLanguages(const QJsonObject &root);
    void loadReadings(const QJsonObject &root);

    QMap<QString, QString> m_strings;
    QMap<QString, double> m_numbers;
    QMap<QString, QString> m_preferences;
    QStringList m_themeOptions;
    QStringList m_fontSizeOptions;
    QMap<QString, ThemePalette> m_palettes;
    bool m_rtlDefault = true;
    QVector<LanguageDefinition> m_languages;
    QMap<QString, ReadingSet> m_readings;

    mutable QHash<QString, QVector<uint>> m_characterCache;

    QString m_appId;
    QString m_appName;
    QString m_appVersion;
    QString m_iconName;
    QString m_projectUrl;
    QString m_authorHandle;
    QString m_authorUrl;
    QString m_copyright;
};

#endif // AVESTAN_CONFIG_H
