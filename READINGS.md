# Readings: sources, coverage, and limitations

`assets/readings.json` holds the verified transliterations used for keyboard key
labels. The application never invents a pronunciation: a character that has no
verified reading is labelled with its hexadecimal code point (`U+XXXX`).

## Sources

The bundled readings are derived from:

- The Unicode Consortium, *The Unicode Standard, Version 17.0*, code charts and
  the Avestan, Old Persian, and Inscriptional Parthian/Pahlavi sections.
- The Unicode Character Database (Unicode 17.0), whose character names give the
  conventional letter/sign value for the ancient Iranian scripts (for example
  `U+10B00 AVESTAN LETTER A` → `a`, `U+103A0 OLD PERSIAN SIGN A` → `a`).
- Standard epigraphic transliteration conventions for the Aramaic-derived
  Parthian and Pahlavi letters.

The readings describe the conventional **transliteration** of a character, not a
pronunciation. This distinction matters for Pahlavi and Parthian, which are
abjads: a letter can carry context-dependent or heterographic values, so no
single universal pronunciation is claimed. Ancient Persian signs are logographic
in places (for example `U+103D1` = “one”), and those are listed by name.

## Coverage

| Script | Range | Verified readings | Special entries |
|--------|-------|-------------------|-----------------|
| Avesta | U+10B00–U+10B35 | 54 letters | U+10B39–U+10B3F abbreviation and punctuation |
| Ancient Persian | U+103A0–U+103C3 | 36 signs | U+103C8–U+103CF logograms, U+103D0–U+103D5 word divider and numbers |
| Parthian | U+10B40–U+10B55 | 22 letters | U+10B58–U+10B5F numbers |
| Pahlavi | U+10B60–U+10B72 | 19 letters | U+10B78–U+10B7F numbers |

All other scripts and symbols fall back to the hexadecimal code point. The
language catalogue itself is in `assets/languages.json`; the readings are keyed
by the same language names.

## Verification

`tests/test_config.cpp` checks that the first four languages are ordered as
required, that known readings resolve (for example Avesta `U+10B00` → `a`), and
that unknown characters fall back to the code point. The loading code in
`src/config.cpp` also validates that every reading range has exactly one reading
per code point.
