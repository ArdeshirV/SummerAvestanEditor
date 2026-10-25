#!/usr/bin/env python3
"""Generate best-effort character labels from Unicode character names.

Unicode names are not true pronunciations for many scripts. These descriptive
fallbacks are overridden by verified readings in assets/readings.json.
"""
import re
import sys
from pathlib import Path


def label_from_name(name: str) -> str:
    name = name.lower()
    if " capital letter " in name:
        name = name.rsplit(" capital letter ", 1)[1]
    elif " small letter " in name:
        name = name.rsplit(" small letter ", 1)[1]
    else:
        for marker in (" letter ", " syllable "):
            if marker in name:
                name = name.rsplit(marker, 1)[1]
                break
    else:
        if " digit " in name:
            name = name.rsplit(" digit ", 1)[1]
        elif " sign " in name:
            name = name.rsplit(" sign ", 1)[1] + " sign"
        elif " ideograph " in name:
            name = "ideograph " + name.rsplit(" ideograph ", 1)[1]
    name = re.sub(r"[,_-]+", " ", name)
    return re.sub(r"\s+", " ", name).strip() or "unnamed character"


def main() -> int:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} UnicodeData.txt output.tsv", file=sys.stderr)
        return 2
    source, destination = map(Path, sys.argv[1:])
    if not source.is_file():
        print(f"UnicodeData.txt not found: {source}", file=sys.stderr)
        return 1
    with destination.open("w", encoding="utf-8", newline="\n") as output:
        output.write("# Best-effort fallback labels derived from Unicode names.\n")
        for line in source.read_text(encoding="utf-8").splitlines():
            fields = line.split(";")
            if len(fields) < 2 or not fields[1] or fields[1].startswith("<"):
                continue
            try:
                cp = int(fields[0], 16)
            except ValueError:
                continue
            output.write(f"{cp:04X}\t{label_from_name(fields[1])}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
