#include "editor.h"

#include <QTextCursor>
#include <QTextOption>
#include <QScrollBar>

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
