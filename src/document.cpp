#include "document.h"

#include "config.h"
#include "unicode_util.h"

#include <QFile>
#include <QSaveFile>
#include <QStringConverter>
#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <qcoreapplication_platform.h>
#endif

namespace {

// Byte-order marks. The UTF-32 LE mark begins with the UTF-16 LE mark, so
// decodeDocument() tests the longer marks first.
constexpr uchar kBomUtf8[] = {0xEF, 0xBB, 0xBF};
constexpr uchar kBomUtf16Le[] = {0xFF, 0xFE};
constexpr uchar kBomUtf16Be[] = {0xFE, 0xFF};
constexpr uchar kBomUtf32Le[] = {0xFF, 0xFE, 0x00, 0x00};
constexpr uchar kBomUtf32Be[] = {0x00, 0x00, 0xFE, 0xFF};

bool startsWith(const QByteArray &data, const uchar *mark, int length)
{
    if (data.size() < length) {
        return false;
    }
    for (int index = 0; index < length; ++index) {
        if (static_cast<uchar>(data.at(index)) != mark[index]) {
            return false;
        }
    }
    return true;
}

template <int Size>
bool startsWith(const QByteArray &data, const uchar (&mark)[Size])
{
    return startsWith(data, mark, Size);
}

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

QString decodeUtf32(const QByteArray &data, int offset, bool bigEndian, QString &error)
{
    const int remaining = data.size() - offset;
    if (remaining % 4 != 0) {
        error = QStringLiteral("UTF-32 file has an incomplete code unit");
        return QString();
    }

    QString result;
    for (int index = offset; index < data.size(); index += 4) {
        const auto byte = [&data](int i) {
            return static_cast<quint32>(static_cast<uchar>(data.at(i)));
        };
        quint32 scalar = 0;
        if (bigEndian) {
            scalar = (byte(index) << 24) | (byte(index + 1) << 16)
                | (byte(index + 2) << 8) | byte(index + 3);
        } else {
            scalar = byte(index) | (byte(index + 1) << 8)
                | (byte(index + 2) << 16) | (byte(index + 3) << 24);
        }
        if (!isValidCodePoint(scalar)) {
            error = QStringLiteral("UTF-32 file contains an invalid Unicode scalar at byte %1")
                        .arg(index - offset);
            return QString();
        }
        const char32_t value = static_cast<char32_t>(scalar);
        result.append(QString::fromUcs4(&value, 1));
    }
    return result;
}

QByteArray encodeUtf32(const QString &text, bool bigEndian)
{
    const QVector<uint> scalars = text.toUcs4();
    QByteArray data;
    data.reserve(scalars.size() * 4 + 4);
    const uchar *bom = bigEndian ? kBomUtf32Be : kBomUtf32Le;
    data.append(reinterpret_cast<const char *>(bom), 4);
    for (uint scalar : scalars) {
        for (int byte = 0; byte < 4; ++byte) {
            const int shift = bigEndian ? (3 - byte) * 8 : byte * 8;
            data.append(static_cast<char>((scalar >> shift) & 0xFF));
        }
    }
    return data;
}

QByteArray encodeUtf16(const QString &text, bool bigEndian)
{
    QByteArray data;
    data.reserve(text.size() * 2 + 2);
    const uchar *bom = bigEndian ? kBomUtf16Be : kBomUtf16Le;
    data.append(reinterpret_cast<const char *>(bom), 2);
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
    case TextEncoding::Utf32LE:
        return config.string(QStringLiteral("encoding_utf32_le"));
    case TextEncoding::Utf32BE:
        return config.string(QStringLiteral("encoding_utf32_be"));
    case TextEncoding::Utf8:
    default:
        return config.string(QStringLiteral("encoding_utf8"));
    }
}

bool decodeDocument(const QByteArray &data, QString &text, TextEncoding &encoding, QString &error)
{
    error.clear();
    text.clear();
    const Config &config = Config::instance();
    if (startsWith(data, kBomUtf32Le)) {
        text = decodeUtf32(data, 4, false, error);
        if (!error.isEmpty()) {
            return false;
        }
        encoding = TextEncoding::Utf32LE;
        return true;
    }
    if (startsWith(data, kBomUtf32Be)) {
        text = decodeUtf32(data, 4, true, error);
        if (!error.isEmpty()) {
            return false;
        }
        encoding = TextEncoding::Utf32BE;
        return true;
    }

    if (startsWith(data, kBomUtf8)) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        text = decoder.decode(data.mid(3));
        if (decoder.hasError()) {
            error = config.string(QStringLiteral("document_invalid_utf8_bom"));
            return false;
        }
        encoding = TextEncoding::Utf8Bom;
        return true;
    }

    if (startsWith(data, kBomUtf16Le)) {
        text = decodeUtf16(data, 2, false, error);
        if (!error.isEmpty()) {
            return false;
        }
        encoding = TextEncoding::Utf16LE;
        return true;
    }
    if (startsWith(data, kBomUtf16Be)) {
        text = decodeUtf16(data, 2, true, error);
        if (!error.isEmpty()) {
            return false;
        }
        encoding = TextEncoding::Utf16BE;
        return true;
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
        QByteArray data(reinterpret_cast<const char *>(kBomUtf8), sizeof(kBomUtf8));
        data.append(text.toUtf8());
        return data;
    }
    case TextEncoding::Utf16LE:
        return encodeUtf16(text, false);
    case TextEncoding::Utf16BE:
        return encodeUtf16(text, true);
    case TextEncoding::Utf32LE:
        return encodeUtf32(text, false);
    case TextEncoding::Utf32BE:
        return encodeUtf32(text, true);
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
    const QByteArray data = encodeDocument(text, encoding);
#ifdef Q_OS_ANDROID
    if (path.startsWith(QStringLiteral("content://"))) {
        QJniEnvironment env;
        const QJniObject context = QNativeInterface::QAndroidApplication::context();
        const QJniObject resolver = context.callObjectMethod(
            "getContentResolver", "()Landroid/content/ContentResolver;");
        const QJniObject javaPath = QJniObject::fromString(path);
        const QJniObject uri = QJniObject::callStaticObjectMethod(
            "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
            javaPath.object<jstring>());
        const QJniObject mode = QJniObject::fromString("wt");
        const QJniObject stream = resolver.callObjectMethod(
            "openOutputStream",
            "(Landroid/net/Uri;Ljava/lang/String;)Ljava/io/OutputStream;",
            uri.object<jobject>(), mode.object<jstring>());
        if (!env.isValid() || !stream.isValid()) {
            env.checkAndClearExceptions();
            error = QStringLiteral("Android could not open the selected document for writing.");
            return false;
        }
        jbyteArray bytes = env->NewByteArray(data.size());
        env->SetByteArrayRegion(bytes, 0, data.size(),
                                reinterpret_cast<const jbyte *>(data.constData()));
        stream.callMethod<void>("write", "([B)V", bytes);
        stream.callMethod<void>("flush", "()V");
        stream.callMethod<void>("close", "()V");
        env->DeleteLocalRef(bytes);
        if (env.checkAndClearExceptions()) {
            error = QStringLiteral("Android could not write the selected document.");
            return false;
        }
        return true;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || file.write(data) != data.size() || !file.flush()) {
        error = file.errorString();
        return false;
    }
    return true;
#else
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()
        || !file.commit()) {
        error = file.errorString();
        return false;
    }
    return true;
#endif
}
