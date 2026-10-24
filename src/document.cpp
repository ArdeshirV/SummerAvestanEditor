#include "document.h"

#include "config.h"

#include <QFile>
#include <QSaveFile>
#include <QStringConverter>

namespace {

constexpr uchar kBomUtf8Byte = 0xEF;
constexpr uchar kBomUtf16LeLow = 0xFF;
constexpr uchar kBomUtf16LeHigh = 0xFE;
constexpr uchar kBomUtf16BeLow = 0xFE;
constexpr uchar kBomUtf16BeHigh = 0xFF;

QString codeUnitError(const QString &key, int index)
{
    const QByteArray format = Config::instance().string(key).toUtf8();
    return QString::asprintf(format.constData(), index);
}

QString decodeUtf16(const QByteArray &data, int offset, bool bigEndian, QString &error)
{
    const int remaining = data.size() - offset;
    if (remaining % 2 != 0) {
        error = Config::instance().string(QStringLiteral("document_incomplete_utf16_unit"));
        return QString();
    }

    QString result;
    result.reserve(remaining / 2);
    for (int index = offset; index < data.size(); index += 2) {
        const uchar first = static_cast<uchar>(data.at(index));
        const uchar second = static_cast<uchar>(data.at(index + 1));
        const ushort unit = bigEndian ? static_cast<ushort>((first << 8) | second)
                                      : static_cast<ushort>((second << 8) | first);
        result.append(QChar(unit));
    }

    // QString already stores UTF-16, so surrogate pairs need no extra work, but
    // validate that the sequence forms well-formed UTF-16.
    for (int index = 0; index < result.size(); ++index) {
        const ushort unit = result.at(index).unicode();
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (index + 1 >= result.size()) {
                error = codeUnitError(QStringLiteral("document_invalid_utf16_surrogate"), index);
                return QString();
            }
            const ushort next = result.at(index + 1).unicode();
            if (next < 0xDC00 || next > 0xDFFF) {
                error = codeUnitError(QStringLiteral("document_invalid_utf16_surrogate"), index);
                return QString();
            }
            ++index;
        } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
            error = codeUnitError(QStringLiteral("document_unpaired_utf16_surrogate"), index);
            return QString();
        }
    }
    return result;
}

QByteArray encodeUtf16(const QString &text, bool bigEndian)
{
    QByteArray data;
    data.reserve(text.size() * 2 + 2);
    data.append(static_cast<char>(bigEndian ? kBomUtf16BeLow : kBomUtf16LeLow));
    data.append(static_cast<char>(bigEndian ? kBomUtf16BeHigh : kBomUtf16LeHigh));
    for (const QChar character : text) {
        const ushort unit = character.unicode();
        if (bigEndian) {
            data.append(static_cast<char>(unit >> 8));
            data.append(static_cast<char>(unit & 0xFF));
        } else {
            data.append(static_cast<char>(unit & 0xFF));
            data.append(static_cast<char>(unit >> 8));
        }
    }
    return data;
}

} // namespace

QString encodingName(TextEncoding encoding)
{
    const Config &config = Config::instance();
    switch (encoding) {
    case TextEncoding::Utf8Bom:
        return config.string(QStringLiteral("encoding_utf8_bom"));
    case TextEncoding::Utf16LE:
        return config.string(QStringLiteral("encoding_utf16_le"));
    case TextEncoding::Utf16BE:
        return config.string(QStringLiteral("encoding_utf16_be"));
    case TextEncoding::Utf8:
    default:
        return config.string(QStringLiteral("encoding_utf8"));
    }
}

bool decodeDocument(const QByteArray &data, QString &text, TextEncoding &encoding, QString &error)
{
    const Config &config = Config::instance();
    if (data.size() >= 3 && static_cast<uchar>(data.at(0)) == kBomUtf8Byte
        && static_cast<uchar>(data.at(1)) == 0xBB && static_cast<uchar>(data.at(2)) == 0xBF) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        text = decoder.decode(data.mid(3));
        if (decoder.hasError()) {
            error = config.string(QStringLiteral("document_invalid_utf8_bom"));
            return false;
        }
        encoding = TextEncoding::Utf8Bom;
        return true;
    }

    if (data.size() >= 2) {
        const uchar first = static_cast<uchar>(data.at(0));
        const uchar second = static_cast<uchar>(data.at(1));
        if (first == kBomUtf16LeLow && second == kBomUtf16LeHigh) {
            text = decodeUtf16(data, 2, false, error);
            if (!error.isEmpty()) {
                return false;
            }
            encoding = TextEncoding::Utf16LE;
            return true;
        }
        if (first == kBomUtf16BeLow && second == kBomUtf16BeHigh) {
            text = decodeUtf16(data, 2, true, error);
            if (!error.isEmpty()) {
                return false;
            }
            encoding = TextEncoding::Utf16BE;
            return true;
        }
    }

    QStringDecoder decoder(QStringDecoder::Utf8);
    text = decoder.decode(data);
    if (decoder.hasError()) {
        error = config.string(QStringLiteral("document_invalid_utf8"));
        return false;
    }
    encoding = TextEncoding::Utf8;
    return true;
}

QByteArray encodeDocument(const QString &text, TextEncoding encoding)
{
    switch (encoding) {
    case TextEncoding::Utf8Bom: {
        QByteArray data;
        data.append(static_cast<char>(kBomUtf8Byte));
        data.append(static_cast<char>(0xBB));
        data.append(static_cast<char>(0xBF));
        data.append(text.toUtf8());
        return data;
    }
    case TextEncoding::Utf16LE:
        return encodeUtf16(text, false);
    case TextEncoding::Utf16BE:
        return encodeUtf16(text, true);
    case TextEncoding::Utf8:
    default:
        return text.toUtf8();
    }
}

bool readDocumentFile(const QString &path, QString &text, TextEncoding &encoding, QString &error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();
    return decodeDocument(data, text, encoding, error);
}

bool writeDocumentFile(const QString &path, const QString &text, TextEncoding encoding, QString &error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }
    const QByteArray data = encodeDocument(text, encoding);
    if (file.write(data) != data.size() || !file.commit()) {
        error = file.errorString();
        return false;
    }
    return true;
}
