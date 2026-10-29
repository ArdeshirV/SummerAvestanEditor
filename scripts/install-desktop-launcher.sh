#!/usr/bin/env bash
# Install the already-built Linux executable and a per-user desktop launcher.
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_BIN="${1:-$ROOT/dist/SummerAvestanEditor-Linux-amd64}"
if [[ ! -x "$SOURCE_BIN" ]]; then
    echo "error: executable not found: $SOURCE_BIN" >&2
    echo "Build it first with: make linux-amd64" >&2
    exit 1
fi

APP_DIR="$HOME/.local/opt/summer-avestan-editor"
BIN="$APP_DIR/SummerAvestanEditor"
ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"
MIME_ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/mimetypes"
MIME_DIR="$HOME/.local/share/mime/packages"
DESKTOP_DIR="$HOME/.local/share/applications"
DESKTOP_FILE="$DESKTOP_DIR/summer-avestan-editor.desktop"
mkdir -p "$APP_DIR" "$ICON_DIR" "$MIME_ICON_DIR" "$MIME_DIR" "$DESKTOP_DIR"
install -m 0755 "$SOURCE_BIN" "$BIN"
install -m 0644 "$ROOT/img/Icon.png" "$ICON_DIR/summer-avestan-editor.png"
install -m 0644 "$ROOT/img/Icon.png" "$MIME_ICON_DIR/application-x-avestan.png"
install -m 0644 "$ROOT/scripts/ave-mime.xml" "$MIME_DIR/avestan-editor.xml"
command -v update-mime-database >/dev/null 2>&1 && update-mime-database "$HOME/.local/share/mime" || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f "$HOME/.local/share/icons/hicolor" || true

# Desktop Entry Exec fields must quote paths containing whitespace and escape
# characters that have special meaning in the Desktop Entry specification.
desktop_quote() {
    local value="$1"
    value="${value//\\/\\\\}"
    value="${value//\"/\\\"}"
    value="${value//\`/\\\`}"
    value="${value//\$/\\\$}"
    printf '"%s"' "$value"
}

{
    printf '[Desktop Entry]\nType=Application\nName=Summer Avestan Editor\n'
    printf 'Comment=Write and edit Unicode texts in Avestan and historic scripts\n'
    printf 'Exec=%s\n' "$(desktop_quote "$BIN")"
    printf 'Icon=%s\n' "$ICON_DIR/summer-avestan-editor.png"
    printf 'Terminal=false\nCategories=Office;TextEditor;Utility;\n'
    printf 'MimeType=text/plain;application/x-avestan;\nStartupNotify=true\n'
} > "$DESKTOP_FILE"
chmod 0644 "$DESKTOP_FILE"
command -v update-desktop-database >/dev/null 2>&1 && \
    update-desktop-database "$DESKTOP_DIR" || true
printf 'Installed launcher: %s\n' "$DESKTOP_FILE"
printf 'Note: this installs the executable, not Qt runtime libraries; the target system must provide Qt 6 Widgets.\n'
