#include <QtTest>

#include <QComboBox>
#include <QFontDatabase>
#include <QLineEdit>
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
    void characterSearchFiltersByCodePointAndReading();
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

void TestKeyboard::characterSearchFiltersByCodePointAndReading()
{
    Editor editor;
    KeyboardPanel panel(&editor);
    QAbstractItemModel *model = panel.characterView()->model();
    QVERIFY(model->rowCount() > 1);

    panel.searchField()->setText(QStringLiteral("U+10B00"));
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, 0)).toString(), keyboardKeyText(QStringLiteral("Avesta"), 0x10B00));

    panel.searchField()->setText(QStringLiteral("AVESTAN LETTER A"));
    bool foundNamedCharacter = false;
    for (int row = 0; row < model->rowCount(); ++row) {
        foundNamedCharacter = foundNamedCharacter
            || model->data(model->index(row, 0), Qt::ToolTipRole).toString()
                   == QStringLiteral("U+10B00 — AVESTAN LETTER A");
    }
    QVERIFY(foundNamedCharacter);
    panel.searchField()->setText(QStringLiteral("a"));
    QVERIFY(model->rowCount() > 1);
    bool foundAvestanAByReading = false;
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex source = qobject_cast<QSortFilterProxyModel *>(model)
                                       ->mapToSource(model->index(row, 0));
        const auto *characters = qobject_cast<CharacterModel *>(
            qobject_cast<QSortFilterProxyModel *>(model)->sourceModel());
        if (characters->codePointAt(source) == 0x10B00) {
            foundAvestanAByReading = true;
            break;
        }
    }
    QVERIFY(foundAvestanAByReading);
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
