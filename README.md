# Avestan Editor

[فارسی](README.fa.md)

Avestan Editor is a cross-platform Unicode text editor for ancient scripts and
other writing systems, built with Qt Widgets. Its software keyboard starts with
Avesta, Pahlavi, Parthian, and Ancient Persian, followed by other historic
scripts, other Unicode scripts, general symbols, and an **All printable
Unicode** keyboard.

The editor runs on Linux, Windows, and Android. You can open, edit, and save
Unicode text documents in UTF-8 (with or without BOM), UTF-16 LE/BE, or BOM-marked
UTF-32 LE/BE. Invalid UTF data is rejected instead of silently replaced. All
settings, language data, readings, Unicode character names, and user-visible
strings are embedded from the `assets/` directory through Qt resources.

## Run and build

Requires a Qt 6 development environment with Qt Widgets and Qt Test (`qmake6`).
On Debian/Ubuntu install `qt6-base-dev` and `qt6-base-dev-tools`.

The `Makefile` builds native and cross-platform targets. Output files are named
`<AppName>-<OS>-<arch>`, for example `SummerAvestanEditor-Linux-amd64`,
`SummerAvestanEditor-Termux-arm64`, `SummerAvestanEditor-Windows-amd64.exe`, and
`SummerAvestanEditor-Android-arm64.apk`.

```sh
make                 # native build (Linux on a desktop, Termux in Termux)
make linux           # desktop Linux binary
make termux          # Termux (Android/bionic) binary
make windows         # Windows .exe
make android         # Android .apk
make test            # Qt Test regression suite
make run             # run the native build
make clean           # remove dist/ and test builds
```

Architecture-specific targets are `linux-amd64 linux-arm64 linux-386`,
`termux-arm64 termux-arm arm termux-amd64`, `windows-amd64 windows-arm64
windows-386`, and `android-arm64 android-arm android-amd64`. Cross builds need a
Qt kit built for the target, so point the relevant variable at that kit's qmake:

```sh
make windows WINDOWS_QMAKE=/opt/Qt/6.8.0/mingw_64/bin/qmake
make android ANDROID_QMAKE=$HOME/Qt/6.8.0/android_arm64_v8a/bin/qmake \
             ANDROID_NDK_ROOT=$HOME/Android/Sdk/ndk/27.0.12077973
```

The single-file qmake build is still available as `qmake6 AvestanEditor.pro &&
make` (or `make build`), and `bash tests/run-tests.sh` runs the tests. A CMake
project file is also provided for portability. Some Termux Qt packages ship
incomplete Qt plugin CMake metadata (a dangling GTK3 platform-theme target),
which makes `find_package(Qt6)` fail before compilation; the qmake project is
the canonical, working build path. Android packaging follows the usual Qt for
Android workflow (`androiddeployqt` with the Android SDK/NDK).

## Editing and shortcuts

The editor is Qt's native multiline widget: typing, caret movement, mouse and
touch selection, clipboard, undo/redo, and the mobile keyboard keep their native
behaviour. Long lines wrap to the editor width and the text scrolls vertically.
The app never changes the editor's layout direction, only its paragraph
alignment.

| Shortcut | Action |
|----------|--------|
| `Ctrl+O` | Open |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` | Save As |
| `Ctrl+Shift+U` | Insert a Unicode code point |
| `F1` | About |
| `Ctrl+X` | Close through the unsaved-changes flow |

Because `Ctrl+X` is reserved for closing per the application requirements, the
keyboard shortcut for the native cut command is not available; use the editor's
context menu (right-click or long-press) for Cut. `Ctrl+C`, `Ctrl+V`, `Ctrl+Z`,
`Ctrl+Y`, and `Ctrl+A` remain native.

Opening a file detects UTF-8, UTF-16, and UTF-32 byte-order marks; files without
a recognized BOM must be valid UTF-8. Existing files keep their detected
encoding on Save. Autosave writes an atomic recovery draft after edits; on the
next launch the app offers to restore it. The last opened file can also be
reopened as a session. Autosave can be disabled in Settings. Save As uses the
standard overwrite confirmation
before replacing an existing file. Exit, the window close button, and `Ctrl+X`
all go through the same unsaved-changes dialog with **Yes** (save first),
**No** (discard), and **Cancel** (stay open) choices.

## Keyboard and scripts

The language selector sits above the text editor; the editor remains above the
software keyboard. The first four language entries are Avesta, Pahlavi,
Parthian, and Ancient Persian; the list then continues with
other ancient Iranian and historic scripts, other Unicode scripts/languages,
general symbols, and **All printable Unicode**. Each entry shows the language
name with a sample in its own script where one is available.

Every printable character of the selected language is shown. The search field
filters by the displayed character, its reading, or its `U+` code point. For
example, search for `U+10B00` to find an Avestan letter. Each key label has the
exact layout **space, character, space, reading** — for example ` 𐬀 a`.
Combining marks are previewed with a dotted circle (`◌́`) but insert only the
mark itself. Tapping a key inserts at the editor's current caret and returns
focus to the editor, so native editing behaviour is preserved. The keyboard
list is virtualised, so large ranges such as CJK, Egyptian Hieroglyphs, or all
of Unicode remain responsive. `Ctrl+Shift+U` also inserts a character from a
hexadecimal code point.

Character enumeration and printability use the Unicode tables built into the
Qt toolchain (`QChar`). Unicode names used for keyboard search are bundled from
UCD 16.0.0 (`assets/unicode_names.tsv`); newly assigned names from later Unicode
versions may be absent. The Unicode data version shown in the status bar is
stored in `assets/app.json`.

## Readings

Verified, source-backed transliterations are bundled for the four requested
scripts in `assets/readings.json`:

| Script | Code points | Verified readings |
|--------|-------------|-------------------|
| Avesta | U+10B00–U+10B35 | 54 letters |
| Ancient Persian | U+103A0–U+103C3 | 36 signs |
| Parthian | U+10B40–U+10B55 | 22 letters |
| Pahlavi | U+10B60–U+10B72 | 19 letters |

Logograms, numbers, and punctuation have named entries (for example Ancient
Persian `U+103D1` = “one”). Any character without a verified reading is shown
with its hexadecimal code point (`U+XXXX`) instead of an invented
pronunciation. See [READINGS.md](READINGS.md) for sources, coverage, and
limitations.

## Settings and appearance

**Settings** contains autosave recovery, the metallic dark/light/system theme,
the text size, and font selection in one dialog:

- the **Autosave recovery draft** option controls whether unsaved drafts are
  saved locally for crash recovery;
- the **Theme** label and selector are on one horizontal row;
- the **Text size** label and selector are on another row;
- a **Font** label sits above a horizontal row with the **Font file...** and
  **Use bundled font** buttons;
- one blank line separates that row from the **Close** button, which is twice
  its default width.

Changes apply immediately and persist between runs. **Font file...** validates
the selected `.ttf`/`.otf` file (size limit and font parsing) before using it.
The bundled **Unicode Script Fallback** is a renamed, merged font containing
Noto Avestan, Old Persian, Parthian, Pahlavi, and 40 other Noto script faces,
plus Noto Naskh Arabic shaping. Glyph coverage does not guarantee full shaping
support for every ancient script; Avestan shaping quality depends on the font
and Qt's shaping backend. The app accepts user-supplied fonts in Settings. The
bundled font is distributed under SIL OFL 1.1; face names and attribution are
in `assets/fonts/FONT-SOURCES.txt` and the license is in
`assets/fonts/LICENSE.txt`.

The **Right to left** checkbox follows **About** in the top bar, defaults to on,
and is remembered. When checked, editor paragraphs align to the right; when
unchecked they align to the left. Keyboard direction is never changed. The top
bar also holds Open, Save, Save As, Settings, and About, with Exit at the top
right.

The editor remains above the keyboard in every orientation. The keyboard
occupies 70% of the window by default; dragging the splitter changes the
proportions, which are saved for the next run. The top-bar keyboard control can
hide the keyboard temporarily to make more room for editing.

## Desktop and portable Linux builds

After `make linux-amd64`, install a per-user desktop launcher with
`./scripts/install-desktop-launcher.sh`. It copies the executable into
`~/.local/opt` and installs the icon and `.desktop` entry. The target machine
still needs compatible Qt 6 runtime libraries. To create a bundled AppImage,
run `./scripts/package-portable-appimage.sh`; it downloads linuxdeploy and its Qt
plugin and bundles the runtime. Review downloaded tools and dependencies before
using them on production systems.

## About dialog

The About dialog shows the app icon next to **Avestan Editor**, the version
`1.0` below the name, the copyright
`Copyright (c) 2026 @ArdeshirV, Licenced under GPLv3+`, and the project link
<https://github.com/ArdeshirV/SummerAvestanEditor>. The `@ArdeshirV` handle in the
copyright line is itself a link to <https://ardeshirv.github.io/>. It can be
dismissed with its Close button or with Escape, Return, or the keypad Enter key.

## Bundled configuration

Startup settings and labels are embedded from `assets/app.json`; the ordered
script catalogue, code-point ranges, aliases, and previews are in
`assets/languages.json`; transliterations are in `assets/readings.json`. These
files are compiled into the executable with Qt resources. Unicode
classification and character enumeration remain algorithmic and use the Qt
toolchain's Unicode tables.

Wistaspa, Avesta, and `ave_bl_l4` are not included because their redistribution
terms could not be verified. The bundled, freely licensed Noto Sans Avestan face
provides Avestan Unicode coverage.

## Please Donate!

We truly appreciate and thank you for any donation you can give to our project.

![Ethereum donation QR code](assets/ethereum-donation.png)

Ethereum `0x6E6465394D14975956cd1BD37ab4E35F2C60300E`

Copyright (c) 2026 @ArdeshirV
