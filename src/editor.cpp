#include "editor.h"

#include <QScrollBar>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextOption>

Editor::Editor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    // Fill the available width, wrap long lines and scroll vertically.
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTabChangesFocus(false);
    setAcceptDrops(true);

    // Use a compact baseline; block formatting applies to existing and future
    // paragraphs without modifying the document's contents.
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(50, QTextBlockFormat::ProportionalHeight);
    QTextCursor cursor(document());
    cursor.setPosition(0);
    cursor.setBlockFormat(blockFormat);
    document()->setDefaultStyleSheet(QStringLiteral("p { line-height: 50%; margin: 0; }"));
}

void Editor::setRightToLeft(bool rightToLeft)
{
    m_rightToLeft = rightToLeft;

    // Only the paragraph alignment changes. The widget layout direction is left
    // untouched so keyboard and caret direction keep their native behaviour.
    QTextOption option = document()->defaultTextOption();
    option.setAlignment(rightToLeft ? Qt::AlignRight : Qt::AlignLeft);
    document()->setDefaultTextOption(option);

    document()->markContentsDirty(0, document()->characterCount());
    viewport()->update();
}

void Editor::insertTextAtCaret(const QString &text)
{
    if (text.isEmpty()) {
        return;
    }
    QTextCursor cursor = textCursor();
    cursor.insertText(text);
    setTextCursor(cursor);
    setFocus(Qt::OtherFocusReason);
}
