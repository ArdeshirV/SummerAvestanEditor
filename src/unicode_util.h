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
