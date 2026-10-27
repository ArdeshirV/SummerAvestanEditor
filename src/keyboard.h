#ifndef AVESTAN_KEYBOARD_H
#define AVESTAN_KEYBOARD_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QWidget>

#include "config.h"

class Editor;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListView;
class QSortFilterProxyModel;

// The exact label layout required for every key: one space, the character
// (combining marks shown with a dotted circle), one space, then the reading.
QString keyLabelForCodePoint(uint codePoint);
QString keyboardKeyText(const QString &languageName, uint codePoint);

class CharacterModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit CharacterModel(QObject *parent = nullptr);

    void setCharacters(const QString &languageName, const QVector<uint> &characters);
    uint codePointAt(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QString m_languageName;
    QVector<uint> m_characters;
};

class KeyDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit KeyDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

// KeyboardPanel shows the language selector above the software keyboard. The
// keyboard lists every printable character of the selected language and inserts
// it at the native editor caret.
class KeyboardPanel : public QWidget {
    Q_OBJECT

public:
    explicit KeyboardPanel(Editor *editor, QWidget *parent = nullptr);

    void setLanguage(int index);
    int currentLanguage() const;
    QComboBox *languageSelector() const { return m_languageSelect; }
    QComboBox *takeLanguageSelector();
    QListView *characterView() const { return m_view; }
    QLineEdit *searchField() const { return m_searchField; }
    void setSearchTrailingWidget(QWidget *widget);

signals:
    void languageChanged(int index);

private slots:
    void handleActivated(const QModelIndex &index);

private:
    Editor *m_editor;
    QComboBox *m_languageSelect;
    QLabel *m_languageLabel = nullptr;
    QHBoxLayout *m_languageRow = nullptr;
    QLineEdit *m_searchField;
    QHBoxLayout *m_searchRow = nullptr;
    QListView *m_view;
    QSortFilterProxyModel *m_filterModel;
    CharacterModel *m_model;
};

#endif // AVESTAN_KEYBOARD_H
