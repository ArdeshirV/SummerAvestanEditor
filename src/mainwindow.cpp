#include "mainwindow.h"

#include "config.h"
#include "editor.h"
#include "keyboard.h"
#include "theme.h"
#include "unicode_util.h"

#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QCursor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QPushButton>
#include <QPixmap>
#include <QSaveFile>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>

namespace {

QString preference(const QString &key)
{
    return Config::instance().preference(key);
}

QSettings applicationSettings()
{
    return QSettings();
}

QString defaultEditorFontSize()
{
    return QStringLiteral("10");
}

// A dialog that can also be dismissed with Escape, Return or the keypad Enter.
class DismissibleDialog : public QDialog {
public:
    using QDialog::QDialog;

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter) {
            accept();
            return;
        }
        QDialog::keyPressEvent(event);
    }
};

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_recoveryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/recovery.txt");
    QDir().mkpath(QFileInfo(m_recoveryPath).absolutePath());
    buildUi();
    applyPreferences();
}

void MainWindow::buildUi()
{
    const Config &config = Config::instance();

    m_editor = new Editor(this);
    m_editor->setPlaceholderText(config.string(QStringLiteral("placeholder")));
    m_recoveryTimer = new QTimer(this);
    m_recoveryTimer->setSingleShot(true);
    m_recoveryTimer->setInterval(1500);
    connect(m_recoveryTimer, &QTimer::timeout, this, &MainWindow::saveRecoveryDraft);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatus);
    connect(m_editor, &QPlainTextEdit::textChanged, this, [this]() {
        if (!m_loading) {
            setDirty(true);
            updateStatus();
            if (applicationSettings().value(QStringLiteral("autosave_enabled"), true).toBool()) {
                m_recoveryTimer->start();
            }
        }
    });

    m_keyboard = new KeyboardPanel(m_editor, this);
    QComboBox *languageSelect = m_keyboard->takeLanguageSelector();

    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_keyboard);
    m_splitter->setChildrenCollapsible(false);

    // Top bar: actions on the left, the RTL checkbox right after About, Exit on
    // the far right.
    auto *topBar = new QWidget(this);
    topBar->setObjectName(QStringLiteral("TopBar"));
    auto *topLayout = new QGridLayout(topBar);
    topLayout->setContentsMargins(6, 4, 6, 4);
    topLayout->setSpacing(6);
    QFont topBarFont = topBar->font();
    topBarFont.setPointSizeF(topBarFont.pointSizeF() * 0.8);
    topBar->setFont(topBarFont);

    auto *openAction = new QAction(config.string(QStringLiteral("open")), this);
    openAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    auto *saveAction = new QAction(config.string(QStringLiteral("save")), this);
    saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    auto *saveAsAction = new QAction(config.string(QStringLiteral("save_as")), this);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    auto *insertCodePointAction = new QAction(config.string(QStringLiteral("insert_unicode")), this);
    insertCodePointAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_U));
    auto *settingsAction = new QAction(config.string(QStringLiteral("settings")), this);
    settingsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    auto *aboutAction = new QAction(config.string(QStringLiteral("about")), this);
    aboutAction->setShortcut(QKeySequence(Qt::Key_F1));
    auto *exitAction = new QAction(config.string(QStringLiteral("exit")), this);
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X));

    addAction(openAction);
    addAction(saveAction);
    addAction(saveAsAction);
    addAction(insertCodePointAction);
    addAction(settingsAction);
    addAction(aboutAction);
    addAction(exitAction);

    connect(openAction, &QAction::triggered, this, &MainWindow::openDocument);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveDocumentAs);
    connect(insertCodePointAction, &QAction::triggered, this, &MainWindow::insertUnicodeCodePoint);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettingsDialog);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(exitAction, &QAction::triggered, this, &MainWindow::requestExit);

    auto *menuButton = new QToolButton(topBar);
    menuButton->setText(QStringLiteral("☰"));
    menuButton->setToolTip(QStringLiteral("Menu"));
    auto *menu = new QMenu(menuButton);
    menu->addAction(openAction);
    menu->addAction(saveAction);
    menu->addAction(saveAsAction);
    menu->addSeparator();
    menu->addAction(settingsAction);
    menu->addAction(aboutAction);
    menu->addSeparator();
    menu->addAction(exitAction);
    menuButton->setMenu(menu);
    menuButton->setPopupMode(QToolButton::InstantPopup);
    topLayout->setColumnStretch(0, 1);
    topLayout->setColumnStretch(1, 1);
    topLayout->setColumnStretch(2, 1);
    topLayout->addWidget(menuButton, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    m_fileNameLabel = new QLabel(config.string(QStringLiteral("untitled")), topBar);
    m_fileNameLabel->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(m_fileNameLabel, 0, 1);
    topLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 2);

    m_rtlCheck = new QCheckBox(config.string(QStringLiteral("right_to_left")), this);
    m_rtlCheck->setToolTip(QStringLiteral("Right to left"));
    m_keyboardToggle = new QToolButton(this);
    m_keyboardToggle->setText(config.string(QStringLiteral("hide_keys")));
    m_keyboardToggle->setToolButtonStyle(Qt::ToolButtonTextOnly);
    connect(m_rtlCheck, &QCheckBox::toggled, this, &MainWindow::toggleRightToLeft);
    connect(languageSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [](int index) {
                applicationSettings().setValue(QStringLiteral("last_language"), index);
            });
    connect(m_keyboardToggle, &QToolButton::clicked, this, &MainWindow::toggleKeyboard);

    auto *central = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(topBar);
    auto *languageBar = new QWidget(central);
    auto *languageLayout = new QHBoxLayout(languageBar);
    languageLayout->setContentsMargins(8, 3, 8, 3);
    QFont languageBarFont = languageBar->font();
    languageBarFont.setPointSizeF(languageBarFont.pointSizeF() * 0.8);
    languageBar->setFont(languageBarFont);
    languageLayout->addWidget(new QLabel(config.string(QStringLiteral("language")), languageBar));
    languageLayout->addWidget(languageSelect, 1);
    m_keyboardToggle->setParent(languageBar);
    m_keyboardToggle->setToolTip(m_keyboardToggle->text());
    languageLayout->addWidget(m_keyboardToggle);
    m_rtlCheck->setParent(languageBar);
    languageLayout->addWidget(m_rtlCheck);
    centralLayout->addWidget(languageBar);
    centralLayout->addWidget(m_splitter, 1);
    setCentralWidget(central);

    m_status = new QLabel(config.string(QStringLiteral("empty")), this);
    m_status->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_status->setOpenExternalLinks(false);
    connect(m_status, &QLabel::linkActivated, this, [](const QString &code) {
        QApplication::clipboard()->setText(code);
    });
    statusBar()->addWidget(m_status);

    setWindowTitle(config.appName());
    setWindowIcon(QIcon(QStringLiteral(":/img/Icon.png")));
    setMinimumWidth(static_cast<int>(config.number(QStringLiteral("minimum_window_width"))));
    setMinimumHeight(static_cast<int>(config.number(QStringLiteral("minimum_window_height"))));
    const QSize defaultSize(static_cast<int>(config.number(QStringLiteral("initial_window_width"))),
                            static_cast<int>(config.number(QStringLiteral("initial_window_height"))));
    resize(applicationSettings().value(QStringLiteral("main_window_size"), defaultSize).toSize());
    languageSelect->setMaximumHeight(qMax(1, height() / 5));
    languageSelect->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    languageSelect->view()->setMaximumHeight(qMax(1, height() * 4 / 5));
    languageSelect->view()->setMinimumHeight(0);
    languageSelect->view()->setMaximumHeight(qMax(1, height() * 4 / 5));

    auto *unicodeButton = new QPushButton(insertCodePointAction->text(), m_keyboard);
    unicodeButton->setToolTip(QStringLiteral("Unicode+ code..."));
    connect(unicodeButton, &QPushButton::clicked, insertCodePointAction, &QAction::trigger);
    m_keyboard->setSearchTrailingWidget(unicodeButton);
}

void MainWindow::applyPreferences()
{
    const Config &config = Config::instance();
    QSettings settings = applicationSettings();

    // Bundled font is always registered; the custom font is restored from the
    // base64 data saved in preferences so it survives restarts.
    const int bundledId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/assets/fonts/UnicodeScriptFallback.ttf"));
    if (bundledId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(bundledId);
        if (!families.isEmpty()) {
            m_bundledFamily = families.first();
        }
    }
    const QString encoded = settings.value(preference(QStringLiteral("custom_font_data"))).toString();
    if (!encoded.isEmpty()) {
        const QByteArray data = QByteArray::fromBase64(encoded.toLatin1());
        const int customId = QFontDatabase::addApplicationFontFromData(data);
        if (customId >= 0) {
            const QStringList families = QFontDatabase::applicationFontFamilies(customId);
            if (!families.isEmpty()) {
                m_customFamily = families.first();
            }
        }
    }

    applyTheme();
    applyEditorFont();

    m_rtlCheck->setChecked(
        settings.value(preference(QStringLiteral("right_to_left")), config.rtlDefault()).toBool());
    m_editor->setRightToLeft(m_rtlCheck->isChecked());
    m_keyboard->setLanguage(settings.value(QStringLiteral("last_language"), 0).toInt());

    const double editorFraction =
        settings.value(preference(QStringLiteral("split_offset")),
                       config.number(QStringLiteral("default_editor_fraction")))
            .toDouble();
    m_editorFraction = editorFraction;
    m_splitterReady = false;

    setCurrentDocument(QString(), TextEncoding::Utf8);
    setDirty(false);
}

void MainWindow::applyTheme()
{
    const Config &config = Config::instance();
    QSettings settings = applicationSettings();
    const QString themeName =
        settings.value(preference(QStringLiteral("theme")), config.string(QStringLiteral("theme_dark")))
            .toString();
    Theme::apply(*qApp, config, themeName);
}

void MainWindow::applyEditorFont()
{
    QSettings settings = applicationSettings();
    bool sizeIsValid = false;
    double pointSize = settings.value(preference(QStringLiteral("font_size")), defaultEditorFontSize())
                           .toString().toDouble(&sizeIsValid);
    if (!sizeIsValid || pointSize <= 0) {
        pointSize = defaultEditorFontSize().toDouble();
    }

    QFont font = m_editor->font();
    const QString family = m_customFamily.isEmpty() ? m_bundledFamily : m_customFamily;
    if (!family.isEmpty()) {
        font.setFamily(family);
    }
    font.setPointSizeF(pointSize);
    m_editor->setFont(font);
}

void MainWindow::setCurrentDocument(const QString &path, TextEncoding encoding)
{
    m_currentPath = path;
    m_encoding = encoding;
    if (!path.isEmpty()) {
        applicationSettings().setValue(QStringLiteral("last_session_path"), path);
    }
    updateStatus();
}

void MainWindow::saveRecoveryDraft()
{
    if (!applicationSettings().value(QStringLiteral("autosave_enabled"), true).toBool()) {
        return;
    }
    QSaveFile file(m_recoveryPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    const QByteArray data = encodeDocument(m_editor->toPlainText(), TextEncoding::Utf8Bom);
    if (file.write(data) != data.size() || !file.commit()) {
        file.cancelWriting();
        return;
    }
    QSettings settings = applicationSettings();
    settings.setValue(QStringLiteral("recovery_path"), m_currentPath);
    settings.setValue(QStringLiteral("recovery_encoding"), static_cast<int>(m_encoding));
    settings.setValue(QStringLiteral("recovery_cursor"), m_editor->textCursor().position());
}

void MainWindow::clearRecoveryDraft()
{
    m_recoveryTimer->stop();
    QFile::remove(m_recoveryPath);
    QSettings settings = applicationSettings();
    settings.remove(QStringLiteral("recovery_path"));
    settings.remove(QStringLiteral("recovery_encoding"));
    settings.remove(QStringLiteral("recovery_cursor"));
}

void MainWindow::offerSessionRestore()
{
    if (m_restorePromptShown) {
        return;
    }
    m_restorePromptShown = true;
    QSettings settings = applicationSettings();

    if (QFile::exists(m_recoveryPath)) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Recover draft"),
            QStringLiteral("An autosaved draft is available. Restore it?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer == QMessageBox::Yes) {
            QString text;
            TextEncoding recoveryEncoding = TextEncoding::Utf8;
            QString error;
            if (readDocumentFile(m_recoveryPath, text, recoveryEncoding, error)) {
                const int storedEncoding = settings.value(QStringLiteral("recovery_encoding"),
                                                          static_cast<int>(TextEncoding::Utf8)).toInt();
                if (storedEncoding >= static_cast<int>(TextEncoding::Utf8)
                    && storedEncoding <= static_cast<int>(TextEncoding::Utf32BE)) {
                    recoveryEncoding = static_cast<TextEncoding>(storedEncoding);
                }
                m_loading = true;
                m_editor->setPlainText(text);
                m_loading = false;
                setCurrentDocument(settings.value(QStringLiteral("recovery_path")).toString(),
                                   recoveryEncoding);
                QTextCursor cursor = m_editor->textCursor();
                cursor.setPosition(qBound(0, settings.value(QStringLiteral("recovery_cursor")).toInt(),
                                          m_editor->document()->characterCount() - 1));
                m_editor->setTextCursor(cursor);
                setDirty(true);
                return;
            }
            QMessageBox::warning(this, QStringLiteral("Recovery failed"), error);
        }
        clearRecoveryDraft();
        return;
    }

    const QString lastPath = settings.value(QStringLiteral("last_session_path")).toString();
    if (lastPath.isEmpty() || !QFile::exists(lastPath)) {
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Restore session"),
                              QStringLiteral("Reopen the last document?\n%1").arg(lastPath),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
        != QMessageBox::Yes) {
        return;
    }
    QString text;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    if (!readDocumentFile(lastPath, text, encoding, error)) {
        QMessageBox::warning(this, QStringLiteral("Could not restore session"), error);
        return;
    }
    m_loading = true;
    m_editor->setPlainText(text);
    m_loading = false;
    setCurrentDocument(lastPath, encoding);
    QTextCursor cursor = m_editor->textCursor();
    cursor.setPosition(qBound(0, settings.value(QStringLiteral("last_session_cursor")).toInt(),
                              m_editor->document()->characterCount() - 1));
    m_editor->setTextCursor(cursor);
    setDirty(false);
}

void MainWindow::toggleKeyboard()
{
    const bool showKeyboard = !m_keyboard->isVisible();
    m_keyboard->setVisible(showKeyboard);
    m_keyboardToggle->setText(showKeyboard
                                  ? Config::instance().string(QStringLiteral("hide_keys"))
                                  : Config::instance().string(QStringLiteral("show_keys")));
    if (!showKeyboard) {
        m_editor->setFocus();
    }
}

void MainWindow::insertUnicodeCodePoint()
{
    bool accepted = false;
    QString input = QInputDialog::getText(
        this, QStringLiteral("Insert Unicode character"),
        QStringLiteral("Enter a code point (for example U+10B00):"),
        QLineEdit::Normal, QString(), &accepted).trimmed();
    if (!accepted || input.isEmpty()) {
        return;
    }
    if (input.startsWith(QStringLiteral("U+"), Qt::CaseInsensitive)) {
        input.remove(0, 2);
    } else if (input.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
        input.remove(0, 2);
    }
    bool valid = false;
    const uint scalar = input.toUInt(&valid, 16);
    if (!valid || scalar > 0x10FFFF || (scalar >= 0xD800 && scalar <= 0xDFFF)) {
        QMessageBox::warning(this, QStringLiteral("Invalid code point"),
                             QStringLiteral("Enter a valid Unicode scalar value in hexadecimal."));
        return;
    }
    m_editor->insertTextAtCaret(codePointToString(scalar));
}

void MainWindow::setDirty(bool dirty)
{
    if (m_dirty == dirty) {
        return;
    }
    m_dirty = dirty;
    m_editor->document()->setModified(dirty);
    updateStatus();
}

void MainWindow::updateStatus()
{
    const Config &config = Config::instance();
    QString name = config.string(QStringLiteral("untitled"));
    if (!m_currentPath.isEmpty()) {
        name = QFileInfo(m_currentPath).fileName();
    }
    const QString marker = m_dirty ? config.string(QStringLiteral("dirty_marker"))
                                   : config.string(QStringLiteral("empty"));
    m_fileNameLabel->setText(name + marker);
    const int codePoints = codePointCount(m_editor->toPlainText());
    const QByteArray format = config.string(QStringLiteral("status_format")).toUtf8();
    const QByteArray markerBytes = marker.toUtf8();
    const QByteArray encodingBytes = encodingName(m_encoding).toUtf8();
    QString currentCharacterInfo = QStringLiteral("—");
    QTextCursor cursor = m_editor->textCursor();
    const int documentEnd = m_editor->document()->characterCount() - 1;
    if (documentEnd > 0 && cursor.position() > 0) {
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        const QVector<uint> scalars = cursor.selectedText().toUcs4();
        if (!scalars.isEmpty()) {
            const uint codePoint = scalars.constLast();
            const QString code = QStringLiteral("U+") + QString::number(codePoint, 16).toUpper();
            const QString characterName = config.unicodeCharacterName(codePoint);
            currentCharacterInfo = QStringLiteral("<a href=\"%1\">%1</a> — %2")
                .arg(code.toHtmlEscaped(),
                     (characterName.isEmpty() ? QStringLiteral("unnamed character")
                                              : characterName).toHtmlEscaped());
        }
    }
    const QByteArray currentCharacterBytes = currentCharacterInfo.toUtf8();
    m_status->setText(QString::asprintf(format.constData(), markerBytes.constData(),
                                       encodingBytes.constData(), codePoints,
                                       currentCharacterBytes.constData()));
}

void MainWindow::toggleRightToLeft(bool enabled)
{
    QSettings settings = applicationSettings();
    settings.setValue(preference(QStringLiteral("right_to_left")), enabled);
    m_editor->setRightToLeft(enabled);
    m_editor->setFocus();
}

void MainWindow::openDocument()
{
    const Config &config = Config::instance();
    const QString path = QFileDialog::getOpenFileName(
        this, config.string(QStringLiteral("open")), m_currentPath,
        config.string(QStringLiteral("document_filter")));
    if (path.isEmpty()) {
        return;
    }

    QString text;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    if (!readDocumentFile(path, text, encoding, error)) {
        QMessageBox::critical(this, config.string(QStringLiteral("open")), error);
        return;
    }

    m_loading = true;
    m_editor->setPlainText(text);
    m_loading = false;
    setCurrentDocument(path, encoding);
    setDirty(false);
    clearRecoveryDraft();
    m_editor->setFocus();
}

void MainWindow::saveDocument()
{
    if (m_currentPath.isEmpty()) {
        saveDocumentAs();
        return;
    }
    writeToPath(m_currentPath);
}

void MainWindow::saveDocumentAs()
{
    const Config &config = Config::instance();
    const QString path = QFileDialog::getSaveFileName(
        this, config.string(QStringLiteral("save_as")),
        m_currentPath.isEmpty() ? config.string(QStringLiteral("untitled_filename")) : m_currentPath,
        config.string(QStringLiteral("document_filter")));
    if (path.isEmpty()) {
        return;
    }
    writeToPath(path);
}

bool MainWindow::writeToPath(const QString &path)
{
    const Config &config = Config::instance();
    QString error;
    if (!writeDocumentFile(path, m_editor->toPlainText(), m_encoding, error)) {
        QMessageBox::critical(this, config.string(QStringLiteral("save")), error);
        return false;
    }
    setCurrentDocument(path, m_encoding);
    setDirty(false);
    clearRecoveryDraft();
    return true;
}

void MainWindow::showSettingsDialog()
{
    const Config &config = Config::instance();
    QSettings settings = applicationSettings();

    DismissibleDialog dialog(this);
    dialog.setWindowTitle(config.string(QStringLiteral("settings")));
    auto *layout = new QVBoxLayout(&dialog);

    auto *autosaveCheck = new QCheckBox(QStringLiteral("Autosave recovery draft"), &dialog);
    autosaveCheck->setChecked(settings.value(QStringLiteral("autosave_enabled"), true).toBool());
    layout->addWidget(autosaveCheck);
    connect(autosaveCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        QSettings local = applicationSettings();
        local.setValue(QStringLiteral("autosave_enabled"), enabled);
        if (!enabled) {
            clearRecoveryDraft();
        }
    });

    // Theme label and selector on one horizontal row.
    auto *themeRow = new QHBoxLayout();
    themeRow->addWidget(new QLabel(config.string(QStringLiteral("theme_settings_label")), &dialog));
    auto *themeSelect = new QComboBox(&dialog);
    themeSelect->addItems(config.themeOptions());
    themeSelect->setCurrentText(
        settings.value(preference(QStringLiteral("theme")), config.string(QStringLiteral("theme_dark")))
            .toString());
    themeRow->addWidget(themeSelect, 1);
    layout->addLayout(themeRow);

    // Text size label and selector on another horizontal row.
    auto *sizeRow = new QHBoxLayout();
    sizeRow->addWidget(new QLabel(config.string(QStringLiteral("font_settings_label")), &dialog));
    auto *sizeSelect = new QComboBox(&dialog);
    sizeSelect->addItems(config.fontSizeOptions());
    sizeSelect->setCurrentText(
        settings.value(preference(QStringLiteral("font_size")), defaultEditorFontSize())
            .toString());
    sizeRow->addWidget(sizeSelect, 1);
    layout->addLayout(sizeRow);

    // Font label starts the row, before the font buttons.
    auto *fontRow = new QHBoxLayout();
    fontRow->addWidget(new QLabel(config.string(QStringLiteral("font")), &dialog));
    auto *fontFileButton = new QPushButton(config.string(QStringLiteral("font_file_button")), &dialog);
    auto *bundledButton = new QPushButton(config.string(QStringLiteral("font_bundled_button")), &dialog);
    fontRow->addWidget(fontFileButton);
    fontRow->addWidget(bundledButton);
    fontRow->addStretch(1);
    layout->addLayout(fontRow);

    auto *fontNote = new QLabel(
        QStringLiteral("The bundled font covers many historic-script glyphs; shaping quality varies. "
                       "For Avestan, choose a font with Avestan OpenType shaping support when available."),
        &dialog);
    fontNote->setWordWrap(true);
    layout->addWidget(fontNote);

    // One blank line before the Close button.
    const int lineHeight = QFontMetrics(dialog.font()).height();
    layout->addSpacing(lineHeight);

    auto *closeButton = new QPushButton(config.string(QStringLiteral("settings_close")), &dialog);
    closeButton->setMinimumWidth(static_cast<int>(closeButton->sizeHint().width()
                                                  * config.number(QStringLiteral("settings_close_button_width_multiplier"))));
    layout->addWidget(closeButton, 0, Qt::AlignRight);

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(themeSelect, &QComboBox::currentTextChanged, this, [this](const QString &value) {
        QSettings local = applicationSettings();
        local.setValue(preference(QStringLiteral("theme")), value);
        applyTheme();
    });
    connect(sizeSelect, &QComboBox::currentTextChanged, this, [this](const QString &value) {
        QSettings local = applicationSettings();
        local.setValue(preference(QStringLiteral("font_size")), value);
        applyEditorFont();
    });
    connect(fontFileButton, &QPushButton::clicked, this, [this]() {
        const Config &local = Config::instance();
        const QString path = QFileDialog::getOpenFileName(
            this, local.string(QStringLiteral("font")), QString(),
            local.string(QStringLiteral("font_filter")));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, local.string(QStringLiteral("font")), file.errorString());
            return;
        }
        const QByteArray data = file.readAll();
        const qint64 limit = static_cast<qint64>(local.number(QStringLiteral("default_font_file_limit_mib"))
                                                 * local.number(QStringLiteral("bytes_per_mib")));
        if (data.size() > limit) {
            const QByteArray format = local.string(QStringLiteral("font_file_too_large")).toUtf8();
            QMessageBox::critical(
                this, local.string(QStringLiteral("font")),
                QString::asprintf(format.constData(),
                                  static_cast<int>(local.number(QStringLiteral("default_font_file_limit_mib")))));
            return;
        }
        const int fontId = QFontDatabase::addApplicationFontFromData(data);
        if (fontId < 0) {
            QString message = local.string(QStringLiteral("font_file_invalid"));
            message.replace(QStringLiteral("%w"), QString());
            QMessageBox::critical(this, local.string(QStringLiteral("font")), message);
            return;
        }
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        m_customFamily = families.isEmpty() ? QString() : families.first();
        QSettings localSettings = applicationSettings();
        localSettings.setValue(preference(QStringLiteral("custom_font_name")), QFileInfo(path).fileName());
        localSettings.setValue(preference(QStringLiteral("custom_font_data")), data.toBase64());
        applyEditorFont();
    });
    connect(bundledButton, &QPushButton::clicked, this, [this]() {
        QSettings localSettings = applicationSettings();
        localSettings.setValue(preference(QStringLiteral("custom_font_name")), QString());
        localSettings.setValue(preference(QStringLiteral("custom_font_data")), QString());
        m_customFamily.clear();
        applyEditorFont();
    });

    dialog.exec();
}

void MainWindow::showAboutDialog()
{
    const Config &config = Config::instance();

    DismissibleDialog dialog(this);
    dialog.setWindowTitle(config.string(QStringLiteral("about_dialog_title")));
    auto *layout = new QVBoxLayout(&dialog);

    auto *icon = new QLabel(&dialog);
    const int iconSize = static_cast<int>(config.number(QStringLiteral("about_icon_size")));
    icon->setPixmap(QPixmap(QStringLiteral(":/img/Icon.png"))
                        .scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto *name = new QLabel(config.appName(), &dialog);
    QFont nameFont = name->font();
    nameFont.setPointSizeF(nameFont.pointSizeF() * config.number(QStringLiteral("about_name_font_multiplier")));
    nameFont.setBold(true);
    name->setFont(nameFont);

    auto *version = new QLabel(QStringLiteral("Version ") + config.appVersion(), &dialog);

    auto *headerText = new QVBoxLayout();
    headerText->addWidget(name);
    headerText->addWidget(version);

    auto *header = new QHBoxLayout();
    header->addWidget(icon);
    header->addLayout(headerText);
    header->addStretch(1);
    layout->addLayout(header);

    // Render the author handle inside the copyright line as a link to the
    // project's author page.
    QString copyright = config.copyright().toHtmlEscaped();
    QString handle = config.authorHandle();
    if (handle.startsWith(QLatin1Char('@'))) {
        handle.remove(0, 1);
    }
    if (!handle.isEmpty()) {
        const int handleIndex = copyright.indexOf(handle.toHtmlEscaped());
        if (handleIndex >= 0) {
            const QString anchor = QStringLiteral("<a href=\"%1\">%2</a>")
                                       .arg(config.authorUrl().toHtmlEscaped(),
                                            handle.toHtmlEscaped());
            copyright.replace(handleIndex, handle.toHtmlEscaped().size(), anchor);
        }
    }
    auto *copyrightLabel = new QLabel(copyright, &dialog);
    copyrightLabel->setOpenExternalLinks(true);
    copyrightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(copyrightLabel);
    layout->addSpacing(QFontMetrics(dialog.font()).height());

    auto *donationTitle = new QLabel(QStringLiteral("Please Donate!"), &dialog);
    QFont donationTitleFont = donationTitle->font();
    donationTitleFont.setBold(true);
    donationTitle->setFont(donationTitleFont);
    layout->addWidget(donationTitle);
    auto *donationDescription = new QLabel(
        QStringLiteral("We truly appreciate and thank you for any donation you can give to our project."),
        &dialog);
    donationDescription->setWordWrap(true);
    layout->addWidget(donationDescription);
    auto *donationImage = new QLabel(&dialog);
    const int donationSize = static_cast<int>(config.number(QStringLiteral("donation_image_size")));
    donationImage->setPixmap(QPixmap(QStringLiteral(":/assets/ethereum-donation.png"))
                                 .scaled(donationSize, donationSize, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation));
    donationImage->setAlignment(Qt::AlignCenter);
    layout->addWidget(donationImage);
    const QString donationAddress = QStringLiteral("0x6E6465394D14975956cd1BD37ab4E35F2C60300E");
    auto *donationAddressLabel = new QLabel(
        QStringLiteral("Ethereum %1").arg(donationAddress), &dialog);
    layout->addWidget(donationAddressLabel);
    auto *copyDonationButton = new QPushButton(QStringLiteral("Copy to clipboard"), &dialog);
    copyDonationButton->setFixedWidth(donationSize);
    layout->addWidget(copyDonationButton, 0, Qt::AlignHCenter);
    connect(copyDonationButton, &QPushButton::clicked, this, [&dialog, donationAddress]() {
        QApplication::clipboard()->setText(donationAddress);
        QToolTip::showText(QCursor::pos(), QStringLiteral("Address copied"), &dialog);
    });
    layout->addSpacing(QFontMetrics(dialog.font()).height());

    auto *link = new QLabel(
        QStringLiteral("<a href=\"%1\">%1</a>").arg(config.projectUrl()), &dialog);
    link->setOpenExternalLinks(true);
    link->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout->addWidget(link, 0, Qt::AlignHCenter);

    layout->addSpacing(QFontMetrics(dialog.font()).height());

    auto *closeButton = new QPushButton(config.string(QStringLiteral("close")), &dialog);
    closeButton->setMinimumWidth(static_cast<int>(closeButton->sizeHint().width()
                                                  * config.number(QStringLiteral("settings_close_button_width_multiplier"))));
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->setAlignment(closeButton, Qt::AlignHCenter);

    dialog.exec();
}

void MainWindow::requestExit()
{
    close();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!m_restorePromptShown) {
        QTimer::singleShot(0, this, &MainWindow::offerSessionRestore);
    }
    if (m_splitterReady) {
        return;
    }
    m_splitterReady = true;
    const int total = qMax(2, m_splitter->height());
    const int editorSize =
        qBound(1, static_cast<int>(total * m_editorFraction), total - 1);
    m_splitter->setSizes({editorSize, total - editorSize});
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_dirty) {
        QSettings settings = applicationSettings();
        const double fraction = static_cast<double>(m_splitter->sizes().value(0))
            / qMax(1, m_splitter->sizes().value(0) + m_splitter->sizes().value(1));
        settings.setValue(preference(QStringLiteral("split_offset")), fraction);
        settings.setValue(QStringLiteral("main_window_size"), size());
        if (!m_currentPath.isEmpty()) {
            settings.setValue(QStringLiteral("last_session_path"), m_currentPath);
            settings.setValue(QStringLiteral("last_session_cursor"), m_editor->textCursor().position());
        }
        clearRecoveryDraft();
        event->accept();
        return;
    }

    const Config &config = Config::instance();
    QMessageBox confirmation(this);
    confirmation.setWindowTitle(config.string(QStringLiteral("save_changes_title")));
    confirmation.setText(config.string(QStringLiteral("save_changes_message")));
    confirmation.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    confirmation.setDefaultButton(QMessageBox::Cancel);

    switch (confirmation.exec()) {
    case QMessageBox::Yes:
        if (m_currentPath.isEmpty()) {
            const QString path = QFileDialog::getSaveFileName(
                this, config.string(QStringLiteral("save_as")),
                config.string(QStringLiteral("untitled_filename")),
                config.string(QStringLiteral("document_filter")));
            if (path.isEmpty() || !writeToPath(path)) {
                event->ignore();
                return;
            }
        } else if (!writeToPath(m_currentPath)) {
            event->ignore();
            return;
        }
        break;
    case QMessageBox::No:
        break;
    case QMessageBox::Cancel:
    default:
        event->ignore();
        return;
    }

    QSettings settings = applicationSettings();
    const double fraction = static_cast<double>(m_splitter->sizes().value(0))
        / qMax(1, m_splitter->sizes().value(0) + m_splitter->sizes().value(1));
    settings.setValue(preference(QStringLiteral("split_offset")), fraction);
    settings.setValue(QStringLiteral("main_window_size"), size());
    if (!m_currentPath.isEmpty()) {
        settings.setValue(QStringLiteral("last_session_path"), m_currentPath);
        settings.setValue(QStringLiteral("last_session_cursor"), m_editor->textCursor().position());
    }
    clearRecoveryDraft();
    event->accept();
}
