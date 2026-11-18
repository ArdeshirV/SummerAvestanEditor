# Avestan Editor: .ave file association

One file type everywhere: extension `.ave`, MIME type `application/x-ave`.

| System  | What you run | Result |
|---|---|---|
| Linux   | `./install-linux.sh --app-dir DIR --exe AvestanEditor --png icon.png` | MIME type, file icon, app icon, "Open with", default app |
| Windows | `powershell -ExecutionPolicy Bypass -File install-windows.ps1 -AppDir DIR -Exe AvestanEditor.exe -Icon icon.ico` | file icon, "Open with", Start menu, entry in Settings > Apps |
| macOS   | `./install-macos.sh --app AvestanEditor.app --png icon.png` | UTI, .icns document icon, default app |
| Android | paste `android/AndroidManifest-intent-filters.xml` into your manifest, then rebuild | app appears for .ave files (no custom file icon is possible on Android) |

Uninstall: `./install-linux.sh --uninstall` or `install-windows.ps1 -Uninstall` (or Settings > Apps).
Add `--system` / `-AllUsers` to install for all users (needs root / administrator).

## Before you run them
1. Deploy your Qt app so it is self-contained: `windeployqt` (Windows), `macdeployqt` (macOS), copy the Qt libs or use an AppImage (Linux).
2. Icons: a square PNG (512 or 1024 px) for Linux/macOS, and an .ico with 16/32/48/256 px for Windows.
3. In your app, handle the incoming file with `qt/open-file-support.h` (command-line argument on Windows/Linux, open event on macOS/Android).

## Good to know
- **Windows 10/11** don't let installers force the default app. If nothing else owns `.ave`, it just works; otherwise pick "Avestan Editor" and "Always" once.
- **macOS**: editing Info.plist breaks the code signature, so the script re-signs ad-hoc. For releases, put the keys from `macos/avestan-document-types.plist` into your CMake Info.plist template and sign normally.
- **Android**: files from some apps arrive as `application/octet-stream`; the file-name filter catches most of them. Save and share your own files as `application/x-ave`.
