#include "keyboard.h"

#include "editor.h"
#include "unicode_util.h"

#include <QComboBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QPen>
#include <QSortFilterProxyModel>
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
    if (!index.isValid()) {
        return QVariant();
    }
    const uint codePoint = codePointAt(index);
    const QString label = keyboardKeyText(m_languageName, codePoint);
    if (role == Qt::DisplayRole) {
        return label;
    }
    const QString hexadecimal = QStringLiteral("U+") + QString::number(codePoint, 16).toUpper();
    const QString name = Config::instance().unicodeCharacterName(codePoint);
    if (role == Qt::UserRole) {
        return label + QLatin1Char(' ') + hexadecimal + QLatin1Char(' ') + name;
    }
    if (role == Qt::ToolTipRole) {
        return name.isEmpty() ? hexadecimal : hexadecimal + QStringLiteral(" — ") + name;
    }
    return QVariant();
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
    painter->setRenderHint(QPainter::Antialiasing);

    const QRect keyRect = option.rect.adjusted(3, 3, -3, -3);
    QColor keyColor = option.palette.color(QPalette::Button);
    QColor edgeColor = option.palette.color(QPalette::Mid);
    if (option.state & QStyle::State_MouseOver) {
        keyColor = option.palette.color(QPalette::Highlight);
        edgeColor = keyColor.lighter(125);
    }

    painter->setPen(QPen(edgeColor, 1));
    painter->setBrush(keyColor);
    painter->drawRoundedRect(keyRect, 5, 5);

    const QString text = index.data(Qt::DisplayRole).toString();
    const QFontMetrics metrics(option.font);
    const QRect textRect = keyRect.adjusted(5, 0, -5, 0);
    const QString elided = metrics.elidedText(text, Qt::ElideRight, textRect.width());
    painter->setPen(option.palette.color(QPalette::ButtonText));
    painter->drawText(textRect, Qt::AlignCenter, elided);
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

    m_languageRow = new QHBoxLayout();
    m_languageRow->setContentsMargins(0, 0, 0, 0);
    m_languageLabel = new QLabel(config.string(QStringLiteral("language")), this);
    m_languageRow->addWidget(m_languageLabel);
    m_languageRow->addWidget(m_languageSelect, 1);

    m_model = new CharacterModel(this);
    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText(config.string(QStringLiteral("keyboard_search_placeholder")));
    m_filterModel = new QSortFilterProxyModel(this);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(Qt::UserRole);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setDynamicSortFilter(true);

    m_view = new QListView(this);
    m_view->setModel(m_filterModel);
    m_view->setItemDelegate(new KeyDelegate(m_view));
    m_view->setViewMode(QListView::IconMode);
    m_view->setFlow(QListView::LeftToRight);
    m_view->setWrapping(true);
    m_view->setResizeMode(QListView::Adjust);
    m_view->setMovement(QListView::Static);
    m_view->setUniformItemSizes(true);
    m_view->setSelectionMode(QAbstractItemView::NoSelection);
    m_view->setMouseTracking(true);
    m_view->viewport()->setMouseTracking(true);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_view->setGridSize(QSize(
        static_cast<int>(config.number(QStringLiteral("keyboard_key_width"))),
        static_cast<int>(config.number(QStringLiteral("keyboard_key_height")))));
    m_view->setSpacing(4);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addLayout(m_languageRow);
    layout->addWidget(m_searchField);
    layout->addWidget(m_view, 1);

    connect(m_searchField, &QLineEdit::textChanged, m_filterModel,
            &QSortFilterProxyModel::setFilterFixedString);
    connect(m_view, &QListView::clicked, this, &KeyboardPanel::handleActivated);
    connect(m_languageSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &KeyboardPanel::setLanguage);

    if (m_languageSelect->count() > 0) {
        setLanguage(0);
    }
}

QComboBox *KeyboardPanel::takeLanguageSelector()
{
    if (m_languageRow) {
        layout()->removeItem(m_languageRow);
        delete m_languageRow;
        m_languageRow = nullptr;
    }
    delete m_languageLabel;
    m_languageLabel = nullptr;
    m_languageSelect->setParent(nullptr);
    return m_languageSelect;
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
    const QModelIndex sourceIndex = m_filterModel->mapToSource(index);
    m_editor->insertTextAtCaret(codePointToString(m_model->codePointAt(sourceIndex)));
}
