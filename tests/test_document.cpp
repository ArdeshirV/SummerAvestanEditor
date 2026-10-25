#include <QtTest>

#include "config.h"
#include "document.h"
#include "unicode_util.h"

class TestDocument : public QObject {
    Q_OBJECT

private slots:
    void utf8PlainRoundTrip();
    void utf8BomRoundTrip();
    void utf16LeRoundTrip();
    void utf16BeRoundTrip();
    void utf32RoundTripsAndDetectsByteOrder();
    void invalidUtf32IsRejected();
    void utf16ByteOrderMarksAreDetected();
    void invalidUtf8IsRejected();
    void incompleteUtf16IsRejected();
};

void TestDocument::utf8PlainRoundTrip()
{
    const QString text = QStringLiteral("Avestan \u10B00 \u0641\u0627\u0631\u0633\u06CC");
    const QByteArray data = encodeDocument(text, TextEncoding::Utf8);
    QString decoded;
    TextEncoding encoding = TextEncoding::Utf16BE;
    QString error;
    QVERIFY(decodeDocument(data, decoded, encoding, error));
    QCOMPARE(decoded, text);
    QCOMPARE(encoding, TextEncoding::Utf8);
}

void TestDocument::utf8BomRoundTrip()
{
    const QString text = QStringLiteral("hello \u10B01");
    const QByteArray data = encodeDocument(text, TextEncoding::Utf8Bom);
    QCOMPARE(data.left(3), QByteArray("\xEF\xBB\xBF", 3));
    QString decoded;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    QVERIFY(decodeDocument(data, decoded, encoding, error));
    QCOMPARE(decoded, text);
    QCOMPARE(encoding, TextEncoding::Utf8Bom);
}

void TestDocument::utf16LeRoundTrip()
{
    const QString text = QStringLiteral("LE \u10B02");
    const QByteArray data = encodeDocument(text, TextEncoding::Utf16LE);
    QCOMPARE(data.left(2), QByteArray("\xFF\xFE", 2));
    QString decoded;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    QVERIFY(decodeDocument(data, decoded, encoding, error));
    QCOMPARE(decoded, text);
    QCOMPARE(encoding, TextEncoding::Utf16LE);
}

void TestDocument::utf16BeRoundTrip()
{
    const QString text = QStringLiteral("BE \u10B03");
    const QByteArray data = encodeDocument(text, TextEncoding::Utf16BE);
    QCOMPARE(data.left(2), QByteArray("\xFE\xFF", 2));
    QString decoded;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    QVERIFY(decodeDocument(data, decoded, encoding, error));
    QCOMPARE(decoded, text);
    QCOMPARE(encoding, TextEncoding::Utf16BE);
}

void TestDocument::utf32RoundTripsAndDetectsByteOrder()
{
    const QString text = QStringLiteral("A ") + codePointToString(0x10B00) + QStringLiteral(" ")
        + codePointToString(0x1F600);
    for (const TextEncoding expected : {TextEncoding::Utf32LE, TextEncoding::Utf32BE}) {
        const QByteArray bytes = encodeDocument(text, expected);
        QString decoded;
        TextEncoding detected = TextEncoding::Utf8;
        QString error;
        QVERIFY(decodeDocument(bytes, decoded, detected, error));
        QCOMPARE(decoded, text);
        QCOMPARE(detected, expected);
    }
}

void TestDocument::invalidUtf32IsRejected()
{
    QString decoded;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    const QByteArray surrogate("\xFF\xFE\x00\x00\x00\xD8\x00\x00", 8);
    QVERIFY(!decodeDocument(surrogate, decoded, encoding, error));
    QVERIFY(!error.isEmpty());
}

void TestDocument::utf16ByteOrderMarksAreDetected()
{
    QString decoded;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    QVERIFY(decodeDocument(QByteArray("\xFF\xFE\x41\x00", 4), decoded, encoding, error));
    QCOMPARE(decoded, QStringLiteral("A"));
    QCOMPARE(encoding, TextEncoding::Utf16LE);

    QVERIFY(decodeDocument(QByteArray("\xFE\xFF\x00\x41", 4), decoded, encoding, error));
    QCOMPARE(decoded, QStringLiteral("A"));
    QCOMPARE(encoding, TextEncoding::Utf16BE);
}

void TestDocument::invalidUtf8IsRejected()
{
    QString decoded;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    QVERIFY(!decodeDocument(QByteArray("\x41\xC3\x28", 3), decoded, encoding, error));
    QVERIFY(!error.isEmpty());
}

void TestDocument::incompleteUtf16IsRejected()
{
    QString decoded;
    TextEncoding encoding = TextEncoding::Utf8;
    QString error;
    QVERIFY(!decodeDocument(QByteArray("\xFF\xFE\x41", 3), decoded, encoding, error));
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(TestDocument)
#include "test_document.moc"
