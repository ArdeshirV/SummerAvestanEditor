#include "keyboard.h"

#include "editor.h"
#include "unicode_util.h"

#include <QComboBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QVBoxLayout>

QString keyLabelForCodePoint(uint codePoint)
{
    const Config &config = Config::instance();
    if (codePoint == ' ') {
        return config.string(QStringLiteral("space_key"));
    }
    if (QChar::isMark(codePoint)) {
        return config.string(QStringLiteral("combining_circle")) + codePointToString(codePoint);
    }
    return codePointToString(codePoint);
}

QString keyboardKeyText(const QString &languageName, uint codePoint)
{
    const Config &config = Config::instance();
    const QString reading = config.phoneticFor(languageName, codePoint).trimmed();
    return config.string(QStringLiteral("key_label_prefix")) + keyLabelForCodePoint(codePoint)
        + config.string(QStringLiteral("key_label_separator")) + reading;
}

CharacterModel::CharacterModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void CharacterModel::setCharacters(const QString &languageName, const QVector<uint> &characters)
{
    beginResetModel();
    m_languageName = languageName;
    m_characters = characters;
    endResetModel();
}

uint CharacterModel::codePointAt(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_characters.size()) {
        return 0;
    }
    return m_characters.at(index.row());
}

int CharacterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_characters.size();
}

QVariant CharacterModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid()) {
        return QVariant();
    }
    return keyboardKeyText(m_languageName, codePointAt(index));
}

KeyDelegate::KeyDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize KeyDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    const Config &config = Config::instance();
    return QSize(static_cast<int>(config.number(QStringLiteral("keyboard_key_width"))),
                 static_cast<int>(config.number(QStringLiteral("keyboard_key_height"))));
}

void KeyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
    painter->save();
    if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, option.palette.alternateBase());
    }

    const QString text = index.data(Qt::DisplayRole).toString();
    const QFontMetrics metrics(option.font);
    const QString elided = metrics.elidedText(text, Qt::ElideRight, option.rect.width() - 8);
    painter->setPen(option.palette.text().color());
    painter->drawText(option.rect.adjusted(4, 0, -4, 0), Qt::AlignCenter, elided);
    painter->restore();
}

KeyboardPanel::KeyboardPanel(Editor *editor, QWidget *parent)
    : QWidget(parent)
    , m_editor(editor)
{
    const Config &config = Config::instance();

    m_languageSelect = new QComboBox(this);
    for (const LanguageDefinition &language : config.languages()) {
        m_languageSelect->addItem(config.languageOptionName(language));
    }

    auto *languageRow = new QHBoxLayout();
    languageRow->setContentsMargins(0, 0, 0, 0);
    languageRow->addWidget(new QLabel(config.string(QStringLiteral("language")), this));
    languageRow->addWidget(m_languageSelect, 1);

    m_model = new CharacterModel(this);
    m_view = new QListView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new KeyDelegate(m_view));
    m_view->setViewMode(QListView::IconMode);
    m_view->setFlow(QListView::LeftToRight);
    m_view->setWrapping(true);
    m_view->setResizeMode(QListView::Adjust);
    m_view->setMovement(QListView::Static);
    m_view->setUniformItemSizes(true);
    m_view->setSelectionMode(QAbstractItemView::NoSelection);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_view->setGridSize(QSize(
        static_cast<int>(config.number(QStringLiteral("keyboard_key_width"))),
        static_cast<int>(config.number(QStringLiteral("keyboard_key_height")))));
    m_view->setSpacing(0);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addLayout(languageRow);
    layout->addWidget(m_view, 1);

    connect(m_view, &QListView::clicked, this, &KeyboardPanel::handleActivated);
    connect(m_languageSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &KeyboardPanel::setLanguage);

    if (m_languageSelect->count() > 0) {
        setLanguage(0);
    }
}

void KeyboardPanel::setLanguage(int index)
{
    const QVector<LanguageDefinition> &languages = Config::instance().languages();
    if (index < 0 || index >= languages.size()) {
        return;
    }
    if (m_languageSelect->currentIndex() != index) {
        m_languageSelect->setCurrentIndex(index);
    }
    const LanguageDefinition &language = languages.at(index);
    m_model->setCharacters(language.name, Config::instance().charactersFor(language));
    m_view->scrollToTop();
    emit languageChanged(index);
}

int KeyboardPanel::currentLanguage() const
{
    return m_languageSelect->currentIndex();
}

void KeyboardPanel::handleActivated(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    m_editor->insertTextAtCaret(codePointToString(m_model->codePointAt(index)));
}
