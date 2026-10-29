#!/usr/bin/env bash
# Bundle the Linux build and its Qt runtime into an AppImage using linuxdeploy.
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="${1:-$ROOT/dist/SummerAvestanEditor-Linux-amd64}"
TOOLS_DIR="${TOOLS_DIR:-$HOME/.local/bin}"
APPDIR="${APPDIR:-$ROOT/dist/AppDir}"
QMAKE="${QMAKE:-qmake6}"

if [[ ! -x "$BINARY" ]]; then
    echo "error: Linux executable not found: $BINARY" >&2
    echo "Build it first with make linux-amd64." >&2
    exit 1
fi
mkdir -p "$TOOLS_DIR"

LINUXDEPLOY="$TOOLS_DIR/linuxdeploy"
QT_PLUGIN="$TOOLS_DIR/linuxdeploy-plugin-qt"
if [[ ! -x "$LINUXDEPLOY" ]]; then
    curl -fL --retry 3 \
        https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
        -o "$LINUXDEPLOY"
    chmod 0755 "$LINUXDEPLOY"
fi
if [[ ! -x "$QT_PLUGIN" ]]; then
    curl -fL --retry 3 \
        https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \
        -o "$QT_PLUGIN"
    chmod 0755 "$QT_PLUGIN"
fi

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/256x256/apps"
install -m 0755 "$BINARY" "$APPDIR/usr/bin/avestan-editor"
install -m 0644 "$ROOT/img/Icon.png" \
    "$APPDIR/usr/share/icons/hicolor/256x256/apps/summer-avestan-editor.png"
cat > "$APPDIR/usr/share/applications/summer-avestan-editor.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Summer Avestan Editor
Comment=Write and edit Unicode texts in Avestan and historic scripts
Exec=avestan-editor
Icon=summer-avestan-editor
Terminal=false
Categories=Office;TextEditor;Utility;
MimeType=text/plain;application/x-avestan;
EOF

export PATH="$TOOLS_DIR:$PATH"
export QMAKE
"$LINUXDEPLOY" --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/avestan-editor" \
    --desktop-file "$APPDIR/usr/share/applications/summer-avestan-editor.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/summer-avestan-editor.png" \
    --plugin qt --output appimage

echo "Portable AppImage output is in the project directory (or as reported by linuxdeploy)."
