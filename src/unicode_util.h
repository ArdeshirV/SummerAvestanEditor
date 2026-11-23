#ifndef AVESTAN_UNICODE_UTIL_H
#define AVESTAN_UNICODE_UTIL_H

#include <QString>

// Convert a Unicode scalar value to a QString. Qt6's fromUcs4 takes char32_t,
// while the rest of the application stores code points as uint.
inline QString codePointToString(uint codePoint)
{
    const char32_t value = static_cast<char32_t>(codePoint);
    return QString::fromUcs4(&value, 1);
}

// True for a Unicode scalar value: inside the valid code point range and
// outside the surrogate block that UTF-16 reserves for surrogate pairs.
inline bool isValidCodePoint(uint codePoint)
{
    return codePoint <= 0x10FFFF && (codePoint < 0xD800 || codePoint > 0xDFFF);
}

// The "U+XXXX" label used by the status bar and the keyboard tooltips.
inline QString codePointLabel(uint codePoint)
{
    return QStringLiteral("U+") + QString::number(codePoint, 16).toUpper();
}

// Count Unicode scalar values, treating each surrogate pair as one code point.
inline int codePointCount(const QString &text)
{
    int count = 0;
    for (int index = 0; index < text.size(); ++index) {
        if (text.at(index).isHighSurrogate() && index + 1 < text.size()
            && text.at(index + 1).isLowSurrogate()) {
            ++index;
        }
        ++count;
    }
    return count;
}

#endif // AVESTAN_UNICODE_UTIL_H
