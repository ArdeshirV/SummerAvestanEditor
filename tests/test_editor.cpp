#include <QtTest>

#include "editor.h"

class TestEditor : public QObject {
    Q_OBJECT

private slots:
    void insertTextAtCaretKeepsSelection();
    void rightToLeftAlignmentToggles();
    void editorWrapsAndScrollsVertically();
};

void TestEditor::insertTextAtCaretKeepsSelection()
{
    Editor editor;
    editor.setPlainText(QStringLiteral("hello world"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(5);
    editor.setTextCursor(cursor);
    editor.insertTextAtCaret(QStringLiteral("!"));
    QCOMPARE(editor.toPlainText(), QStringLiteral("hello! world"));
    QCOMPARE(editor.textCursor().position(), 6);
}

void TestEditor::rightToLeftAlignmentToggles()
{
    Editor editor;
    editor.setPlainText(QStringLiteral("hello"));
    editor.setRightToLeft(true);
    QCOMPARE(editor.document()->defaultTextOption().alignment(), Qt::AlignRight);
    editor.setRightToLeft(false);
    QCOMPARE(editor.document()->defaultTextOption().alignment(), Qt::AlignLeft);
}

void TestEditor::editorWrapsAndScrollsVertically()
{
    Editor editor;
    QCOMPARE(editor.lineWrapMode(), QPlainTextEdit::WidgetWidth);
    QCOMPARE(editor.horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    QCOMPARE(editor.verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
}

QTEST_MAIN(TestEditor)
#include "test_editor.moc"
