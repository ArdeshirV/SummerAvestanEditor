#ifndef AVESTAN_DIALOGS_H
#define AVESTAN_DIALOGS_H

#include <QDialog>

class Config;
class QPushButton;

// A dialog that can also be dismissed with Escape, Return or the keypad Enter.
class DismissibleDialog : public QDialog {
public:
    using QDialog::QDialog;

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

// Widen a dialog's close button by the configured factor so its label is not
// clipped on narrow screens.
void applyCloseButtonWidth(QPushButton *button, const Config &config);

// Show the About dialog (icon, version, copyright, donation).
void showAboutDialog(QWidget *parent);

#endif // AVESTAN_DIALOGS_H
