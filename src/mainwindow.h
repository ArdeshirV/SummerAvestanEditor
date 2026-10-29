#ifndef AVESTAN_MAINWINDOW_H
#define AVESTAN_MAINWINDOW_H

#include <QMainWindow>

#include "document.h"

class Editor;
class KeyboardPanel;
class QCheckBox;
class QLabel;
class QSplitter;
class QTimer;
class QToolButton;
#ifdef Q_OS_ANDROID
class SafBridge;
#endif

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void openDocument();
    void saveDocument();
    void saveDocumentAs();
#ifdef Q_OS_ANDROID
    void saveAsInSafFolder(const QString &folderUri);
#endif
    void showSettingsDialog();
    void showAboutDialog();
    void toggleRightToLeft(bool enabled);
    void updateStatus();
    void insertUnicodeCodePoint();
    void saveRecoveryDraft();
    void offerSessionRestore();
    void toggleKeyboard();

private:
    void buildUi();
    void applyPreferences();
    void applyTheme();
    void applyEditorFont();
    void setDirty(bool dirty);
    void setCurrentDocument(const QString &path, TextEncoding encoding);
    void clearRecoveryDraft();
    bool writeToPath(const QString &path);
    void requestExit();

    Editor *m_editor = nullptr;
    KeyboardPanel *m_keyboard = nullptr;
    QSplitter *m_splitter = nullptr;
    QCheckBox *m_rtlCheck = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_fileNameLabel = nullptr;
    QToolButton *m_keyboardToggle = nullptr;
    QTimer *m_recoveryTimer = nullptr;
#ifdef Q_OS_ANDROID
    SafBridge *m_safBridge = nullptr;
    bool m_safOverwritePending = false;
#endif

    QString m_currentPath;
    TextEncoding m_encoding = TextEncoding::Utf8;
    bool m_dirty = false;
    bool m_loading = false;
    bool m_restorePromptShown = false;
    QString m_recoveryPath;

    QString m_bundledFamily;
    QString m_customFamily;
    double m_editorFraction = 0.3;
    bool m_splitterReady = false;
};

#endif // AVESTAN_MAINWINDOW_H
