#ifndef AVESTAN_DOCUMENT_H
#define AVESTAN_DOCUMENT_H

#include <QByteArray>
#include <QString>

// Document encodings supported for open and save. UTF-16/UTF-32 byte-order
// marks are detected; unmarked files are treated as UTF-8.
enum class TextEncoding {
    Utf8,
    Utf8Bom,
    Utf16LE,
    Utf16BE,
    Utf32LE,
    Utf32BE,
};

QString encodingName(TextEncoding encoding);

// Decode raw file bytes into a QString, detecting the encoding from the
// byte-order mark and validating the result. Returns false on invalid input.
bool decodeDocument(const QByteArray &data, QString &text, TextEncoding &encoding, QString &error);

// Encode a QString using the given encoding (BOM included where applicable).
QByteArray encodeDocument(const QString &text, TextEncoding encoding);

bool readDocumentFile(const QString &path, QString &text, TextEncoding &encoding, QString &error);
bool writeDocumentFile(const QString &path, const QString &text, TextEncoding encoding, QString &error);

#endif // AVESTAN_DOCUMENT_H
