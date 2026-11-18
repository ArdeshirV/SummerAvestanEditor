#!/bin/bash
# Registers the .ave file type (UTI, document icon, handler) for Avestan Editor on macOS.
# Works on a built, deployed .app bundle (run macdeployqt first).
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
PB=/usr/libexec/PlistBuddy
LSREGISTER=/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister

usage() { cat <<USAGE
Usage: $0 --app AvestanEditor.app --png icon.png [--dest DIR]
  --app   your built and deployed application bundle
  --png   square PNG for the .ave document icon (1024x1024 is best)
  --dest  where to install the app (default: ~/Applications; use /Applications with sudo)
USAGE
}
die() { echo "error: $*" >&2; exit 1; }

app=""; png=""; dest="$HOME/Applications"
while [[ $# -gt 0 ]]; do
  case $1 in
    -h|--help) usage; exit 0 ;;
    --app|--png|--dest) [[ $# -ge 2 ]] || die "$1 needs a value"
      case $1 in --app) app=$2;; --png) png=$2;; --dest) dest=$2;; esac; shift 2 ;;
    *) usage; die "unknown option: $1" ;;
  esac
done
[[ $(uname) == Darwin ]] || die "this script is for macOS"
[[ -f $app/Contents/Info.plist ]] || { usage; die "--app must be an .app bundle"; }
[[ -f $png ]] || die "PNG not found: $png"

mkdir -p "$dest"
target="$dest/$(basename "$app")"
if [[ $(cd "$app/.." && pwd -P) != $(cd "$dest" && pwd -P) ]]; then
  rm -rf "$target"; cp -R "$app" "$target"
fi

# 1. PNG -> .icns (document icon)
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
iconset="$tmp/AvestanDocument.iconset"; mkdir "$iconset"
for s in 16 32 128 256 512; do
  sips -z $s $s "$png" --out "$iconset/icon_${s}x${s}.png" >/dev/null
  sips -z $((s*2)) $((s*2)) "$png" --out "$iconset/icon_${s}x${s}@2x.png" >/dev/null
done
mkdir -p "$target/Contents/Resources"
iconutil -c icns "$iconset" -o "$target/Contents/Resources/AvestanDocument.icns"

# 2. declare the .ave type + make the app its handler in Info.plist
plist="$target/Contents/Info.plist"
for key in CFBundleDocumentTypes UTExportedTypeDeclarations; do
  "$PB" -c "Delete :$key" "$plist" 2>/dev/null || true
done
"$PB" -c "Merge $HERE/macos/avestan-document-types.plist" "$plist"

# 3. the edited bundle must be signed again (ad-hoc here), then tell Launch Services
codesign --force --deep --sign - "$target" 2>/dev/null || echo "warning: codesign failed" >&2
"$LSREGISTER" -f "$target"
touch "$target"
echo "Done. $target now opens .ave files. If Finder still shows old icons, run: killall Finder"
