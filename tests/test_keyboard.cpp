#include <QtTest>

#include <QComboBox>
#include <QFontDatabase>
#include <QListView>

#include "config.h"
#include "editor.h"
#include "keyboard.h"
#include "unicode_util.h"

class TestKeyboard : public QObject {
    Q_OBJECT

private slots:
    void combiningMarkUsesDottedCircle();
    void spaceKeyUsesSpaceSymbol();
    void keyLabelHasExactLayout();
    void codePointCountCountsScalars();
    void keyboardPanelBuilds();
    void allPrintableLanguageBuilds();
    void keyboardTapInsertsAtCaret();
};

void TestKeyboard::combiningMarkUsesDottedCircle()
{
    const uint combiningMark = 0x0301;
    const QString expected =
        Config::instance().string(QStringLiteral("combining_circle")) + codePointToString(combiningMark);
    QCOMPARE(keyLabelForCodePoint(combiningMark), expected);
}

void TestKeyboard::spaceKeyUsesSpaceSymbol()
{
    QCOMPARE(keyLabelForCodePoint(' '), Config::instance().string(QStringLiteral("space_key")));
}

void TestKeyboard::keyLabelHasExactLayout()
{
    // One space, the character, one space, then the reading.
    const QString text = keyboardKeyText(QStringLiteral("Avesta"), 0x10B00);
    QCOMPARE(text, QStringLiteral(" ") + codePointToString(0x10B00) + QStringLiteral(" ") + QStringLiteral("a"));
}

void TestKeyboard::codePointCountCountsScalars()
{
    const QString text = QStringLiteral("A") + codePointToString(0x10B00) + QStringLiteral("B");
    QCOMPARE(codePointCount(text), 3);
}

void TestKeyboard::keyboardPanelBuilds()
{
    Editor editor;
    KeyboardPanel panel(&editor);
    QCOMPARE(panel.languageSelector()->count(), Config::instance().languages().size());
    QVERIFY(panel.languageSelector()->itemText(0).startsWith(QStringLiteral("Avesta")));
    QVERIFY(panel.characterView()->model() != nullptr);
    QVERIFY(panel.characterView()->model()->rowCount() > 0);
    QVERIFY(panel.characterView()->model()->data(panel.characterView()->model()->index(0, 0))
                .toString()
                .contains(QLatin1Char(' ')));
}

void TestKeyboard::allPrintableLanguageBuilds()
{
    Editor editor;
    KeyboardPanel panel(&editor);
    const int last = Config::instance().languages().size() - 1;
    panel.setLanguage(last);
    QCOMPARE(panel.currentLanguage(), last);
    QVERIFY(panel.characterView()->model()->rowCount() > 100000);
}

void TestKeyboard::keyboardTapInsertsAtCaret()
{
    Editor editor;
    editor.setPlainText(QStringLiteral("ab"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(1);
    editor.setTextCursor(cursor);

    const uint codePoint = 0x10B00;
    editor.insertTextAtCaret(codePointToString(codePoint));
    QCOMPARE(editor.toPlainText(),
             QStringLiteral("a") + codePointToString(codePoint) + QStringLiteral("b"));
}

QTEST_MAIN(TestKeyboard)
#include "test_keyboard.moc"
