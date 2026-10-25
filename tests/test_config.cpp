#include <QtTest>

#include <QFontDatabase>

#include "config.h"

class TestConfig : public QObject {
    Q_OBJECT

private slots:
    void languagesAreOrdered();
    void verifiedReadingsAreUsed();
    void unknownReadingsFallBackToCodePoint();
    void charactersIncludeEveryPrintableAndSpace();
    void themePalettesLoad();
    void applicationMetadataLoads();
    void bundledFontLoads();
};

void TestConfig::languagesAreOrdered()
{
    const QVector<LanguageDefinition> &languages = Config::instance().languages();
    QVERIFY(languages.size() >= 4);
    QCOMPARE(languages.at(0).name, QStringLiteral("Avesta"));
    QCOMPARE(languages.at(1).name, QStringLiteral("Pahlavi"));
    QCOMPARE(languages.at(2).name, QStringLiteral("Parthian"));
    QCOMPARE(languages.at(3).name, QStringLiteral("Ancient Persian"));
}

void TestConfig::verifiedReadingsAreUsed()
{
    const Config &config = Config::instance();
    QCOMPARE(config.phoneticFor(QStringLiteral("Avesta"), 0x10B00), QStringLiteral("a"));
    QCOMPARE(config.phoneticFor(QStringLiteral("Avesta"), 0x10B01), QStringLiteral("aa"));
    QCOMPARE(config.phoneticFor(QStringLiteral("Ancient Persian"), 0x103A0), QStringLiteral("a"));
}

void TestConfig::unknownReadingsFallBackToCodePoint()
{
    QCOMPARE(Config::instance().phoneticFor(QStringLiteral("Avesta"), 0x0041),
             QStringLiteral("41"));
}

void TestConfig::charactersIncludeEveryPrintableAndSpace()
{
    const QVector<LanguageDefinition> &languages = Config::instance().languages();
    const QVector<uint> &characters = Config::instance().charactersFor(languages.at(0));
    QVERIFY(!characters.isEmpty());
    QVERIFY(characters.contains(' '));
    // Avestan letters are printable and belong to the first language.
    QVERIFY(characters.contains(static_cast<uint>(0x10B00)));
    // Control characters are not advertised as keyboard keys.
    QVERIFY(!characters.contains(static_cast<uint>(0x0000)));
    QVERIFY(!characters.contains(static_cast<uint>(0x000A)));
}

void TestConfig::themePalettesLoad()
{
    const ThemePalette *dark = Config::instance().themePalette(QStringLiteral("Metallic Dark"));
    QVERIFY(dark != nullptr);
    QVERIFY(dark->valid);
    QCOMPARE(dark->variant, QStringLiteral("dark"));
    const ThemePalette *light = Config::instance().themePalette(QStringLiteral("Metallic Light"));
    QVERIFY(light != nullptr);
    QVERIFY(light->valid);
}

void TestConfig::applicationMetadataLoads()
{
    const Config &config = Config::instance();
    QCOMPARE(config.appVersion(), QStringLiteral("1.0"));
    QCOMPARE(config.projectUrl(), QStringLiteral("https://github.com/ArdeshirV/SummerAvestanEditor"));
    QCOMPARE(config.authorUrl(), QStringLiteral("https://ardeshirv.github.io/"));
    QVERIFY(config.copyright().contains(QStringLiteral("@ArdeshirV")));
}

void TestConfig::bundledFontLoads()
{
    const int fontId =
        QFontDatabase::addApplicationFont(QStringLiteral(":/assets/fonts/UnicodeScriptFallback.ttf"));
    QVERIFY(fontId >= 0);
    QVERIFY(!QFontDatabase::applicationFontFamilies(fontId).isEmpty());
}

QTEST_MAIN(TestConfig)
#include "test_config.moc"
