#!/usr/bin/env bash
# Installs Avestan Editor on Linux and registers the .ave file type:
# MIME type, file icon, application icon, "Open with" entry and default handler.
set -euo pipefail

NAME="Avestan Editor"; ID=avestan-editor
MIME=application/x-ave; MIME_ICON=application-x-ave

usage() { cat <<USAGE
Usage: $0 --app-dir DIR --exe FILE --png ICON.png [--system]
       $0 --uninstall [--system]
  --app-dir  folder with your built app (executable + Qt libraries/plugins)
  --exe      executable file name inside that folder
  --png      square PNG icon (256x256 or 512x512 recommended)
  --system   install for all users (needs root); default is current user only
USAGE
}
die()  { echo "error: $*" >&2; exit 1; }
warn() { echo "warning: $*" >&2; }

app_dir=""; exe=""; png=""; system=0; uninstall=0
while [[ $# -gt 0 ]]; do
  case $1 in
    --system) system=1; shift ;;
    --uninstall) uninstall=1; shift ;;
    -h|--help) usage; exit 0 ;;
    --app-dir|--exe|--png)
      [[ $# -ge 2 ]] || die "$1 needs a value"
      case $1 in --app-dir) app_dir=$2;; --exe) exe=$2;; --png) png=$2;; esac
      shift 2 ;;
    *) usage; die "unknown option: $1" ;;
  esac
done

if (( system )); then
  [[ $EUID -eq 0 ]] || die "--system needs root (use sudo)"
  APP_DST=/opt/$ID; DATA=/usr/local/share
else
  DATA=${XDG_DATA_HOME:-$HOME/.local/share}; APP_DST=$DATA/$ID
fi

refresh_caches() {
  if command -v update-mime-database >/dev/null; then
    [[ -d $DATA/mime ]] && { update-mime-database "$DATA/mime" >/dev/null 2>&1 || warn "update-mime-database failed"; }
  else warn "update-mime-database not found (install shared-mime-info); the file type won't be recognised"; fi
  if command -v update-desktop-database >/dev/null; then update-desktop-database "$DATA/applications" 2>/dev/null || true; fi
  if command -v gtk-update-icon-cache >/dev/null; then gtk-update-icon-cache -qtf "$DATA/icons/hicolor" 2>/dev/null || true; fi
}

if (( uninstall )); then
  rm -rf "$APP_DST"
  rm -f "$DATA/applications/$ID.desktop" "$DATA/mime/packages/$ID.xml"
  find "$DATA/icons/hicolor" \( -name "$ID.png" -o -name "$MIME_ICON.png" \) -delete 2>/dev/null || true
  refresh_caches
  echo "Uninstalled $NAME."; exit 0
fi

[[ -d $app_dir ]]        || { usage; die "--app-dir is required and must be a folder"; }
[[ -f $app_dir/$exe ]]   || die "executable not found: $app_dir/$exe"
[[ -f $png ]]            || die "PNG icon not found: $png"
[[ $(od -An -tx1 -N4 "$png" | tr -d ' \n') == 89504e47 ]] || die "$png is not a PNG file"

# read the PNG width straight from its header (no ImageMagick needed)
set -- $(od -An -tu1 -j16 -N4 "$png")
size=$(( ($1 << 24) | ($2 << 16) | ($3 << 8) | $4 ))
case $size in 16|22|24|32|36|48|64|72|96|128|192|256|384|512) ;;
  *) warn "icon is ${size}px; a standard size (256 or 512) works best with icon themes" ;; esac

# 1. program files
rm -rf "$APP_DST"; mkdir -p "$APP_DST"
cp -a "$app_dir"/. "$APP_DST"/
chmod +x "$APP_DST/$exe"

# 2. icons: one for the application, one for .ave files
for kind in apps:$ID mimetypes:$MIME_ICON; do
  dir=$DATA/icons/hicolor/${size}x${size}/${kind%%:*}
  mkdir -p "$dir"; cp "$png" "$dir/${kind#*:}.png"
done

# 3. MIME type: *.ave -> application/x-ave
mkdir -p "$DATA/mime/packages"
cat > "$DATA/mime/packages/$ID.xml" <<XML
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="$MIME">
    <comment>$NAME document</comment>
    <glob pattern="*.ave"/>
    <icon name="$MIME_ICON"/>
  </mime-type>
</mime-info>
XML

# 4. launcher entry that claims the MIME type
mkdir -p "$DATA/applications"
cat > "$DATA/applications/$ID.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=$NAME
Comment=Editor for Avestan and Old Persian text
Exec="$APP_DST/$exe" %f
Icon=$ID
Terminal=false
Categories=Utility;TextEditor;
MimeType=$MIME;
DESKTOP

refresh_caches
if (( ! system )) && command -v xdg-mime >/dev/null; then
  xdg-mime default "$ID.desktop" "$MIME" || warn "could not set default application"
fi
echo "Installed $NAME to $APP_DST. Double-click a .ave file to test it."
