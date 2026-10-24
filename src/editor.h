#ifndef AVESTAN_EDITOR_H
#define AVESTAN_EDITOR_H

#include <QPlainTextEdit>

// Editor is a thin wrapper around the native Qt multiline widget. It only
// changes paragraph alignment (right for RTL) and inserts text at the current
// caret; typing, selection, clipboard and undo remain Qt's native behaviour.
class Editor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr);

    void setRightToLeft(bool rightToLeft);
    bool isRightToLeft() const { return m_rightToLeft; }

    // Insert keyboard text at the native caret and restore focus so the
    // on-screen keyboard does not steal editing behaviour.
    void insertTextAtCaret(const QString &text);

private:
    bool m_rightToLeft = true;
};

#endif // AVESTAN_EDITOR_H
