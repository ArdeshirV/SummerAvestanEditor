#!/usr/bin/env bash
# Install the Qt Android ARM64 kit/SDK and build an MXE Qt6 Windows cross-kit.
# Supported host: Ubuntu/Debian x86_64. The Windows Qt build via MXE is lengthy
# and can consume tens of GB of disk space.
set -Eeuo pipefail

QT_VERSION="${QT_VERSION:-6.8.3}"
ANDROID_API="${ANDROID_API:-35}"
ANDROID_NDK_VERSION="${ANDROID_NDK_VERSION:-27.2.12479018}"
ANDROID_BUILD_TOOLS="${ANDROID_BUILD_TOOLS:-35.0.0}"
ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/Android/Sdk}"
QT_ROOT="${QT_ROOT:-$HOME/Qt}"
MXE_ROOT="${MXE_ROOT:-$HOME/src/mxe}"
MXE_TARGET="${MXE_TARGET:-x86_64-w64-mingw32.static}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"

if [[ "$(uname -m)" != "x86_64" ]]; then
    echo "error: this setup script currently supports x86_64 Linux hosts only." >&2
    exit 1
fi
if [[ ! -r /etc/os-release ]] || ! grep -Eq '^(ID|ID_LIKE)=.*(ubuntu|debian)' /etc/os-release; then
    echo "error: this setup script requires Ubuntu or Debian." >&2
    exit 1
fi

if (( EUID == 0 )); then
    SUDO=()
elif command -v sudo >/dev/null 2>&1; then
    SUDO=(sudo)
else
    echo "error: install system packages as root or install sudo first." >&2
    exit 1
fi

printf 'This installs Qt %s for Android and builds Qt6 for Windows with MXE.\n' "$QT_VERSION"
printf 'The Windows Qt cross-build can take a long time and use tens of GB of disk.\n\n'

"${SUDO[@]}" apt-get update
"${SUDO[@]}" apt-get install -y \
    autoconf automake autopoint bash bison build-essential bzip2 ca-certificates \
    cmake curl flex gettext git gperf intltool libffi-dev libgl-dev \
    libgdk-pixbuf-2.0-dev libltdl-dev libssl-dev libtool libtool-bin \
    libxml-parser-perl lzip make openjdk-17-jdk openssl patch perl \
    pkg-config python3 python3-mako python3-pip python3-pkgconfig python3-venv \
    python3-setuptools python3-yaml ruby sed unzip wget xz-utils

# Android SDK command-line tools and required SDK/NDK packages.
mkdir -p "$ANDROID_SDK_ROOT/cmdline-tools"
SDKMANAGER="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
if [[ ! -x "$SDKMANAGER" ]]; then
    tmpdir="$(mktemp -d)"
    trap 'rm -rf "$tmpdir"' EXIT
    curl -fL --retry 3 \
        "https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip" \
        -o "$tmpdir/commandlinetools.zip"
    unzip -q "$tmpdir/commandlinetools.zip" -d "$tmpdir"
    rm -rf "$ANDROID_SDK_ROOT/cmdline-tools/latest"
    mv "$tmpdir/cmdline-tools" "$ANDROID_SDK_ROOT/cmdline-tools/latest"
    rm -rf "$tmpdir"
    trap - EXIT
fi
export ANDROID_HOME="$ANDROID_SDK_ROOT"
export ANDROID_SDK_ROOT
export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"
export PATH="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/platform-tools:$PATH"

# Accept SDK licenses when possible, then install the selected platform, tools,
# and NDK. The following sdkmanager invocation reports any actual install error.
yes | "$SDKMANAGER" --sdk_root="$ANDROID_SDK_ROOT" --licenses >/dev/null 2>&1 || true
"$SDKMANAGER" --sdk_root="$ANDROID_SDK_ROOT" \
    "platform-tools" \
    "platforms;android-${ANDROID_API}" \
    "build-tools;${ANDROID_BUILD_TOOLS}" \
    "ndk;${ANDROID_NDK_VERSION}"

# aqtinstall provides the Linux-hosted qmake/androiddeployqt tools and the
# ARM64-v8a Qt Android libraries. Keep it isolated from distro Python packages.
AQT_VENV="${AQT_VENV:-$HOME/.local/venvs/aqtinstall}"
python3 -m venv "$AQT_VENV"
"$AQT_VENV/bin/python" -m pip install --upgrade pip aqtinstall
"$AQT_VENV/bin/aqt" install-qt linux android "$QT_VERSION" android_arm64_v8a -O "$QT_ROOT"

# MXE provides a Linux-hosted Qt/qmake cross-kit and matching Windows libraries.
# This is necessary: the ordinary Linux qmake and a standalone MinGW compiler
# are not sufficient to link a Qt Widgets Windows application.
mkdir -p "$(dirname "$MXE_ROOT")"
if [[ ! -d "$MXE_ROOT/.git" ]]; then
    git clone --depth 1 https://github.com/mxe/mxe.git "$MXE_ROOT"
fi
make -C "$MXE_ROOT" -j"$JOBS" MXE_TARGETS="$MXE_TARGET" qtbase
export PATH="$MXE_ROOT/usr/bin:$PATH"

ANDROID_QMAKE="$QT_ROOT/$QT_VERSION/android_arm64_v8a/bin/qmake"
ANDROIDDEPLOYQT="$QT_ROOT/$QT_VERSION/android_arm64_v8a/bin/androiddeployqt"
WINDOWS_QMAKE="$MXE_ROOT/usr/$MXE_TARGET/qt6/bin/qmake"

for tool in "$ANDROID_QMAKE" "$ANDROIDDEPLOYQT" "$WINDOWS_QMAKE" \
            "$MXE_ROOT/usr/bin/$MXE_TARGET-g++"; do
    if [[ ! -x "$tool" ]]; then
        echo "error: expected build tool not found: $tool" >&2
        echo "Check the Qt/MXE installation output and the MXE target layout." >&2
        exit 1
    fi
done

cat <<EOF

Setup complete. From the project directory, build Android ARM64 with:
  ANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \\
  ANDROID_QMAKE="$ANDROID_QMAKE" \\
  ANDROIDDEPLOYQT="$ANDROIDDEPLOYQT" \\
  ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk/$ANDROID_NDK_VERSION" \\
  ANDROID_API="$ANDROID_API" make android-arm64

Build Windows AMD64 with:
  PATH="$MXE_ROOT/usr/bin:\$PATH" \\
  WINDOWS_QMAKE="$WINDOWS_QMAKE" \\
  MINGW_PREFIX="$MXE_TARGET" make windows-amd64

You can change versions and locations by setting QT_VERSION, ANDROID_API,
ANDROID_NDK_VERSION, ANDROID_SDK_ROOT, QT_ROOT, MXE_ROOT, MXE_TARGET, and JOBS
before running this script.
EOF
