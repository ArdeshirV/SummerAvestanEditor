#include "config.h"

#include "unicode_util.h"

#include <algorithm>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>

namespace {

QString readEmbedded(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

QJsonObject parseObject(const QString &path)
{
    const QByteArray data = readEmbedded(path).toUtf8();
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return QJsonObject();
    }
    return document.object();
}

QColor colorFromString(const QString &value)
{
    // Assets use #RRGGBBAA (Qt hexadecimal colours use #AARRGGBB), so the
    // alpha component is moved to the front before constructing the colour.
    const QString trimmed = value.trimmed();
    if (trimmed.size() != 9 || !trimmed.startsWith(QLatin1Char('#'))) {
        return QColor(trimmed);
    }
    const QString reordered = QStringLiteral("#") + trimmed.mid(7, 2) + trimmed.mid(1, 6);
    return QColor(reordered);
}

} // namespace

const Config &Config::instance()
{
    static const Config config;
    return config;
}

Config::Config()
{
    loadApplication(parseObject(QStringLiteral(":/assets/app.json")));
    loadLanguages(parseObject(QStringLiteral(":/assets/languages.json")));
    loadReadings(parseObject(QStringLiteral(":/assets/readings.json")));
}

void Config::loadApplication(const QJsonObject &root)
{
    const QJsonObject app = root.value(QStringLiteral("application")).toObject();
    m_appId = app.value(QStringLiteral("id")).toString();
    m_appName = app.value(QStringLiteral("name")).toString();
    m_appVersion = app.value(QStringLiteral("version")).toString();
    m_iconName = app.value(QStringLiteral("icon")).toString();
    m_projectUrl = app.value(QStringLiteral("project_url")).toString();
    m_authorHandle = app.value(QStringLiteral("author_handle")).toString();
    m_authorUrl = app.value(QStringLiteral("author_url")).toString();
    m_copyright = app.value(QStringLiteral("copyright")).toString();

    const QJsonObject strings = root.value(QStringLiteral("strings")).toObject();
    for (auto it = strings.begin(); it != strings.end(); ++it) {
        m_strings.insert(it.key(), it.value().toString());
    }

    const QJsonObject numbers = root.value(QStringLiteral("numbers")).toObject();
    for (auto it = numbers.begin(); it != numbers.end(); ++it) {
        m_numbers.insert(it.key(), it.value().toDouble());
    }

    const QJsonObject preferences = root.value(QStringLiteral("preferences")).toObject();
    for (auto it = preferences.begin(); it != preferences.end(); ++it) {
        m_preferences.insert(it.key(), it.value().toString());
    }

    const QJsonArray themeOptions = root.value(QStringLiteral("theme_options")).toArray();
    for (const QJsonValue &value : themeOptions) {
        m_themeOptions.append(value.toString());
    }

    const QJsonArray sizeOptions = root.value(QStringLiteral("font_size_options")).toArray();
    for (const QJsonValue &value : sizeOptions) {
        m_fontSizeOptions.append(value.toString());
    }

    m_rtlDefault = root.value(QStringLiteral("right_to_left_default")).toBool(true);

    const QJsonObject palettes = root.value(QStringLiteral("theme_palettes")).toObject();
    for (auto it = palettes.begin(); it != palettes.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        ThemePalette palette;
        palette.variant = entry.value(QStringLiteral("variant")).toString();
        if (entry.contains(QStringLiteral("background"))) {
            palette.background = colorFromString(entry.value(QStringLiteral("background")).toString());
            palette.input = colorFromString(entry.value(QStringLiteral("input")).toString());
            palette.button = colorFromString(entry.value(QStringLiteral("button")).toString());
            palette.primary = colorFromString(entry.value(QStringLiteral("primary")).toString());
            palette.foreground = colorFromString(entry.value(QStringLiteral("foreground")).toString());
            palette.hover = colorFromString(entry.value(QStringLiteral("hover")).toString());
            palette.border = colorFromString(entry.value(QStringLiteral("border")).toString());
            palette.valid = true;
        }
        m_palettes.insert(it.key(), palette);
    }
}

void Config::loadLanguages(const QJsonObject &root)
{
    const QJsonArray languages = root.value(QStringLiteral("languages")).toArray();
    for (const QJsonValue &value : languages) {
        const QJsonObject entry = value.toObject();
        LanguageDefinition language;
        language.name = entry.value(QStringLiteral("name")).toString();
        language.sample = entry.value(QStringLiteral("sample")).toString();
        language.afterUnicodeScripts = entry.value(QStringLiteral("after_unicode_scripts")).toBool(false);
        const QJsonArray ranges = entry.value(QStringLiteral("ranges")).toArray();
        for (const QJsonValue &rangeValue : ranges) {
            const QJsonArray range = rangeValue.toArray();
            if (range.size() == 2) {
                language.ranges.append({static_cast<uint>(range.at(0).toDouble()),
                                        static_cast<uint>(range.at(1).toDouble())});
            }
        }
        m_languages.append(language);
    }
}

void Config::loadReadings(const QJsonObject &root)
{
    for (auto it = root.begin(); it != root.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        ReadingSet set;
        const QJsonArray ranges = entry.value(QStringLiteral("ranges")).toArray();
        for (const QJsonValue &rangeValue : ranges) {
            const QJsonObject rangeObject = rangeValue.toObject();
            ReadingRange range;
            bool okFirst = false;
            bool okLast = false;
            range.first = rangeObject.value(QStringLiteral("start")).toString().toUInt(&okFirst, 16);
            range.last = rangeObject.value(QStringLiteral("end")).toString().toUInt(&okLast, 16);
            const QJsonArray readings = rangeObject.value(QStringLiteral("readings")).toArray();
            for (const QJsonValue &reading : readings) {
                range.readings.append(reading.toString());
            }
            if (okFirst && okLast && range.last >= range.first
                && range.readings.size() == static_cast<int>(range.last - range.first + 1)) {
                set.ranges.append(range);
            }
        }
        const QJsonObject special = entry.value(QStringLiteral("special")).toObject();
        for (auto specialIt = special.begin(); specialIt != special.end(); ++specialIt) {
            bool ok = false;
            const uint codePoint = specialIt.key().toUInt(&ok, 16);
            if (ok) {
                set.special.insert(codePoint, specialIt.value().toString());
            }
        }
        m_readings.insert(it.key(), set);
    }
}

QString Config::string(const QString &key) const
{
    const auto it = m_strings.constFind(key);
    return it == m_strings.constEnd() ? QString() : it.value();
}

double Config::number(const QString &key) const
{
    const auto it = m_numbers.constFind(key);
    return it == m_numbers.constEnd() ? 0.0 : it.value();
}

QString Config::preference(const QString &key) const
{
    const auto it = m_preferences.constFind(key);
    return it == m_preferences.constEnd() ? QString() : it.value();
}

const ThemePalette *Config::themePalette(const QString &name) const
{
    const auto it = m_palettes.constFind(name);
    if (it == m_palettes.constEnd()) {
        return nullptr;
    }
    return &it.value();
}

QString Config::normalizedLanguageName(const QString &name)
{
    QString normalized = name.toLower();
    static const QVector<QPair<QString, QString>> replacements = {
        {QStringLiteral(" "), QString()},
        {QStringLiteral("-"), QString()},
        {QStringLiteral("_"), QString()},
        {QStringLiteral("'"), QString()},
    };
    for (const auto &pair : replacements) {
        normalized.replace(pair.first, pair.second);
    }
    return normalized;
}

QString Config::languageOptionName(const LanguageDefinition &language) const
{
    QString sample = language.sample;
    if (sample.isEmpty()) {
        const QVector<uint> &characters = charactersFor(language);
        QString preview;
        const int wanted = static_cast<int>(number(QStringLiteral("language_preview_characters")));
        for (uint codePoint : characters) {
            if (codePoint == ' ' || QChar::isMark(codePoint)) {
                continue;
            }
            preview.append(codePointToString(codePoint));
            if (preview.size() >= wanted) {
                break;
            }
        }
        sample = preview;
    }
    if (sample.isEmpty()) {
        return language.name;
    }
    const QByteArray format = string(QStringLiteral("language_sample_format")).toUtf8();
    return QString::asprintf(format.constData(), language.name.toUtf8().constData(),
                             sample.toUtf8().constData());
}

QString Config::phoneticFor(const QString &languageName, uint codePoint) const
{
    if (codePoint == ' ') {
        return string(QStringLiteral("phonetic_space"));
    }
    const auto setIt = m_readings.constFind(languageName);
    if (setIt != m_readings.constEnd()) {
        const ReadingSet &set = setIt.value();
        const auto specialIt = set.special.constFind(codePoint);
        if (specialIt != set.special.constEnd()) {
            return specialIt.value();
        }
        for (const ReadingRange &range : set.ranges) {
            if (codePoint >= range.first && codePoint <= range.last) {
                return range.readings.at(static_cast<int>(codePoint - range.first));
            }
        }
    }
    return QString::asprintf(string(QStringLiteral("phonetic_unknown_format")).toUtf8().constData(),
                             codePoint);
}

void Config::loadUnicodeNames() const
{
    if (m_unicodeNamesLoaded) {
        return;
    }
    m_unicodeNamesLoaded = true;
    QFile file(QStringLiteral(":/assets/unicode_names.tsv"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    m_unicodeNames.reserve(150000);
    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        const qsizetype tab = line.indexOf('\t');
        if (tab <= 0) {
            continue;
        }
        bool ok = false;
        const uint codePoint = line.left(tab).toUInt(&ok, 16);
        if (ok) {
            m_unicodeNames.insert(codePoint,
                                  QString::fromLatin1(line.constData() + tab + 1,
                                                      line.size() - tab - 1));
        }
    }
}

QString Config::unicodeCharacterName(uint codePoint) const
{
    loadUnicodeNames();
    return m_unicodeNames.value(codePoint);
}

const QVector<uint> &Config::charactersFor(const LanguageDefinition &language) const
{
    const auto cached = m_characterCache.constFind(language.name);
    if (cached != m_characterCache.constEnd()) {
        return cached.value();
    }

    QVector<uint> characters;
    QSet<uint> seen;
    bool hasSpace = false;
    const auto append = [&](uint codePoint) {
        if (!QChar::isPrint(codePoint) || seen.contains(codePoint)) {
            return;
        }
        seen.insert(codePoint);
        characters.append(codePoint);
        hasSpace = hasSpace || codePoint == ' ';
    };

    for (const auto &range : language.ranges) {
        for (uint codePoint = range.first; codePoint <= range.second; ++codePoint) {
            append(codePoint);
        }
    }
    std::sort(characters.begin(), characters.end());
    if (!hasSpace) {
        characters.append(' ');
    }
    return m_characterCache.insert(language.name, characters).value();
}
