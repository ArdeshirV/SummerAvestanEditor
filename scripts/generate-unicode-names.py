#!/usr/bin/env python3
"""Generate the bundled character-name index from Python's Unicode 16.0 UCD."""
import unicodedata
from pathlib import Path

EXPECTED_VERSION = "16.0.0"
if unicodedata.unidata_version != EXPECTED_VERSION:
    raise SystemExit(
        f"Expected Unicode {EXPECTED_VERSION}, got {unicodedata.unidata_version}; "
        "use CPython built with the matching Unicode database."
    )

root = Path(__file__).resolve().parents[1]
output = root / "assets" / "unicode_names.tsv"
with output.open("w", encoding="utf-8", newline="\n") as names:
    names.write(f"# Unicode Character Database {EXPECTED_VERSION}\n")
    for codepoint in range(0x110000):
        try:
            name = unicodedata.name(chr(codepoint))
        except ValueError:
            continue
        names.write(f"{codepoint:X}\t{name}\n")
print(f"Wrote {output}")
