# ==============================================================================
# Avestan Editor — cross-platform Qt 6 build system (qmake)
# ==============================================================================
#
# Targets:
#   make                 build for the current host (Termux or Linux)
#   make linux           build the desktop Linux binary
#   make termux          build the Termux (Android/bionic) binary
#   make windows         build the Windows executable
#   make android         build the Android APK
#   make test            run the Qt Test regression suite
#   make clean           remove build output
#   make help            show this help
#
# Output names use the form <AppName>-<OS>-<arch>:
#   SummerAvestanEditor-Linux-amd64
#   SummerAvestanEditor-Termux-arm64
#   SummerAvestanEditor-Windows-amd64.exe
#   SummerAvestanEditor-Android-arm64.apk
#
# Cross builds need a Qt kit built for the target platform, so point the
# relevant *_QMAKE variable at that kit's qmake, for example:
#   make windows WINDOWS_QMAKE=/opt/Qt/6.8.0/mingw_64/bin/qmake
#   make android ANDROID_QMAKE=$HOME/Qt/6.8.0/android_arm64_v8a/bin/qmake \
#        ANDROID_NDK_ROOT=$HOME/Android/Sdk/ndk/27.0.12077973
# ==============================================================================

APP     ?= SummerAvestanEditor
VERSION ?= 1.0
PROJECT ?= AvestanEditor.pro
DIST    ?= dist
JOBS    ?= $(shell nproc 2>/dev/null || echo 2)

# Host Qt (native builds).
QMAKE ?= qmake6
MAKE  ?= make

# Per-platform Qt kits. Defaults fall back to the host qmake so native builds
# work out of the box; override them for real cross builds.
LINUX_QMAKE   ?= $(QMAKE)
TERMUX_QMAKE  ?= $(QMAKE)
WINDOWS_QMAKE ?= $(QMAKE)
ANDROID_QMAKE ?= $(if $(wildcard $(HOME)/Documents/Apps/Qt/6.12/6.12.0/android_arm64_v8a/bin/qmake),$(HOME)/Documents/Apps/Qt/6.12/6.12.0/android_arm64_v8a/bin/qmake,$(QMAKE))

# Cross toolchain / packaging settings.
MINGW_PREFIX    ?= x86_64-w64-mingw32
WINDOWS_SPEC    ?= win32-g++
# Leave the native specs empty so qmake uses its configured default (for
# example linux-clang in Termux, linux-g++ on most desktops).
LINUX_SPEC      ?=
TERMUX_SPEC     ?=
ANDROID_SPEC    ?= android-clang
ANDROID_API     ?= 23
ANDROID_ABI     ?= arm64-v8a
ANDROID_ARCH    ?= arm64
ANDROIDDEPLOYQT ?= $(if $(wildcard $(HOME)/Documents/Apps/Qt/6.12/6.12.0/gcc_64/bin/androiddeployqt),$(HOME)/Documents/Apps/Qt/6.12/6.12.0/gcc_64/bin/androiddeployqt,androiddeployqt)

# Host detection.
HOST_MACHINE := $(shell uname -m)
HOST_ARCH := $(shell printf '%s' '$(HOST_MACHINE)' \
	| sed -e 's/aarch64/arm64/' -e 's/armv7l/arm/' \
	      -e 's/x86_64/amd64/' -e 's/i[3-6]86/386/')
IS_TERMUX := $(if $(wildcard /data/data/com.termux),yes,no)

ifeq ($(IS_TERMUX),yes)
NATIVE := termux
else
NATIVE := linux
endif

.PHONY: all $(NATIVE) linux termux windows android build test clean help \
        linux-amd64 linux-arm64 linux-386 \
        termux-arm64 termux-arm termux-amd64 \
        windows-amd64 windows-arm64 windows-386 \
        android-arm64 android-arm android-amd64

all: $(NATIVE)

# ------------------------------------------------------------------------------
# Generic desktop build: (1) OS label, (2) arch, (3) qmake, (4) qmake args,
# (5) extension. Produces dist/<APP>-<OS>-<arch><ext>.
# ------------------------------------------------------------------------------
define qt_build
	@mkdir -p "$(DIST)/build/$(1)-$(2)"
	@echo ">>> Building $(APP)-$(1)-$(2)$(5)"
	@cd "$(DIST)/build/$(1)-$(2)" && \
		$(3) "$(CURDIR)/$(PROJECT)" $(4) && \
		$(MAKE) -j"$(JOBS)"
	@test -f "$(DIST)/build/$(1)-$(2)/avestan-editor$(5)" \
		|| { echo "error: expected $(DIST)/build/$(1)-$(2)/avestan-editor$(5) was not produced"; exit 1; }
	@mkdir -p "$(DIST)"
	@cp "$(DIST)/build/$(1)-$(2)/avestan-editor$(5)" "$(DIST)/$(APP)-$(1)-$(2)$(5)"
	@echo ">>> $(DIST)/$(APP)-$(1)-$(2)$(5)"
endef

# Native desktop builds --------------------------------------------------------

linux: linux-$(HOST_ARCH)

linux-amd64 linux-arm64 linux-386:
	@if [ "$(IS_TERMUX)" = "yes" ] && [ "$(LINUX_QMAKE)" = "$(QMAKE)" ]; then \
		echo "error: desktop Linux build from Termux needs a Linux Qt kit."; \
		echo "       Set LINUX_QMAKE=/path/to/linux/Qt/bin/qmake (cross sysroot required)."; \
		exit 1; \
	fi
	$(call qt_build,Linux,$(patsubst linux-%,%,$@),$(LINUX_QMAKE),$(if $(LINUX_SPEC),-spec $(LINUX_SPEC),),)

# Termux builds ----------------------------------------------------------------

termux: termux-$(HOST_ARCH)

termux-arm64 termux-arm termux-amd64:
	$(call qt_build,Termux,$(patsubst termux-%,%,$@),$(TERMUX_QMAKE),$(if $(TERMUX_SPEC),-spec $(TERMUX_SPEC),),)

# Windows builds ---------------------------------------------------------------

windows: windows-amd64

windows-amd64 windows-arm64 windows-386:
	@command -v "$(WINDOWS_QMAKE)" >/dev/null 2>&1 \
		|| { echo "error: $(WINDOWS_QMAKE) not found. Set WINDOWS_QMAKE to a Qt kit for Windows."; exit 1; }
	@if [ "$(WINDOWS_SPEC)" = "win32-g++" ]; then \
		command -v "$(MINGW_PREFIX)-g++" >/dev/null 2>&1 \
			|| { echo "error: $(MINGW_PREFIX)-g++ not found; install mingw-w64 or set MINGW_PREFIX/WINDOWS_QMAKE."; exit 1; }; \
	fi
	$(call qt_build,Windows,$(patsubst windows-%,%,$@),$(WINDOWS_QMAKE),\
		-spec $(WINDOWS_SPEC) QMAKE_CC=$(MINGW_PREFIX)-gcc QMAKE_CXX=$(MINGW_PREFIX)-g++,.exe)

# Android builds ---------------------------------------------------------------

android: android-arm64

android-arm64:
	$(call android_build,arm64,arm64-v8a)

android-arm:
	$(call android_build,arm,armeabi-v7a)

android-amd64:
	$(call android_build,amd64,x86_64)

# (1) arch label, (2) Android ABI.
define android_build
	@test -n "$(ANDROID_NDK_ROOT)" \
		|| { echo "error: set ANDROID_NDK_ROOT to the Android NDK path."; exit 1; }
	@command -v "$(ANDROID_QMAKE)" >/dev/null 2>&1 \
		|| { echo "error: $(ANDROID_QMAKE) not found. Set ANDROID_QMAKE to a Qt kit for Android."; exit 1; }
	@mkdir -p "$(DIST)/build/Android-$(1)"
	@echo ">>> Building $(APP)-Android-$(1)"
	@cd "$(DIST)/build/Android-$(1)" && \
		$(ANDROID_QMAKE) "$(CURDIR)/$(PROJECT)" \
			-spec $(ANDROID_SPEC) ANDROID_ABIS="$(2)" \
			$(if $(ANDROID_NDK_ROOT),ANDROID_NDK_ROOT="$(ANDROID_NDK_ROOT)",) && \
		$(MAKE) -j"$(JOBS)"
	@if command -v "$(ANDROIDDEPLOYQT)" >/dev/null 2>&1; then \
		cd "$(DIST)/build/Android-$(1)" && \
		mkdir -p "android-build/libs/$(2)" && \
		cp "libavestan-editor_$(2).so" "android-build/libs/$(2)/" && \
		$(ANDROIDDEPLOYQT) --input "android-avestan-editor-deployment-settings.json" \
			--output "$(CURDIR)/$(DIST)/build/Android-$(1)/android-build" \
			--apk --android-platform "android-$(ANDROID_API)" \
			$(if $(ANDROID_NDK_ROOT),--ndk "$(ANDROID_NDK_ROOT)",); \
		apk=$$(find "$(CURDIR)/$(DIST)/build/Android-$(1)" -name '*.apk' | head -n 1); \
		test -n "$$apk" || { echo "error: androiddeployqt produced no APK"; exit 1; }; \
		cp "$$apk" "$(CURDIR)/$(DIST)/$(APP)-Android-$(1).apk"; \
		echo ">>> $(DIST)/$(APP)-Android-$(1).apk"; \
	else \
		echo "note: $(ANDROIDDEPLOYQT) not found; native libraries were built but no APK was packaged."; \
		find "$(DIST)/build/Android-$(1)" -name 'libavestan-editor_*.so' -exec cp {} "$(DIST)/" \; ; \
		echo ">>> $(DIST)/libavestan-editor_$(2).so"; \
	fi
endef

# Development helpers ----------------------------------------------------------

build:
	mkdir -p "$(DIST)/build/native"
	cd "$(DIST)/build/native" && $(QMAKE) "$(CURDIR)/$(PROJECT)" && $(MAKE) -j"$(JOBS)"
	@mkdir -p "$(DIST)"
	@cp "$(DIST)/build/native/avestan-editor" "$(DIST)/$(APP)-$(if $(filter yes,$(IS_TERMUX)),Termux,Linux)-$(HOST_ARCH)" 2>/dev/null \
		|| cp "$(DIST)/build/native/avestan-editor.exe" "$(DIST)/$(APP)-Windows-$(HOST_ARCH).exe"

test:
	bash ./tests/run-tests.sh

# Run the built binary for the current host.
# Set RUN_ARGS to pass extra arguments, e.g. make run RUN_ARGS="--help"
# Set QT_QPA_PLATFORM to force a platform plugin (xcb, wayland, offscreen).
# Set QT_LOGGING_RULES to control Qt debug output.
RUN_BIN ?= $(DIST)/$(APP)-$(if $(filter yes,$(IS_TERMUX)),Termux,Linux)-$(HOST_ARCH)

# Auto-detect the best Qt platform plugin for the host.
QT_QPA_PLATFORM = $(shell \
	if [ -n "$$WAYLAND_DISPLAY" ]; then \
		echo wayland; \
	elif [ -n "$$DISPLAY" ]; then \
		echo xcb; \
	else \
		echo offscreen; \
	fi)

run:
	@test -f "$(RUN_BIN)" \
		|| { echo "error: $(RUN_BIN) not found. Build first: make linux-amd64"; exit 1; }
	@echo ">>> Running $(RUN_BIN) [QT_QPA_PLATFORM=$(QT_QPA_PLATFORM)]"
	@QT_QPA_PLATFORM="$(QT_QPA_PLATFORM)" $(RUN_BIN) $(RUN_ARGS)

# Run with verbose Qt logging for troubleshooting.
run-debug:
	@test -f "$(RUN_BIN)" \
		|| { echo "error: $(RUN_BIN) not found. Build first: make linux-amd64"; exit 1; }
	@echo ">>> Running $(RUN_BIN) with debug logging"
	@QT_QPA_PLATFORM="$(QT_QPA_PLATFORM)" QT_LOGGING_RULES="*.debug=true" $(RUN_BIN) $(RUN_ARGS)

clean:
	rm -rf "$(DIST)" tests/build build-qmake

help:
	@echo "Avestan Editor build system"
	@echo ""
	@echo "  make                 build for the current host ($(NATIVE))"
	@echo "  make linux           Linux binary      ($(LINUX_QMAKE))"
	@echo "  make termux          Termux binary     ($(TERMUX_QMAKE))"
	@echo "  make windows         Windows .exe      ($(WINDOWS_QMAKE))"
	@echo "  make android         Android .apk      ($(ANDROID_QMAKE))"
	@echo "  make test            run regression tests"
	@echo "  make run             run the built binary"
	@echo "  make run-debug       run with verbose Qt logging"
	@echo "  make clean           remove dist/ and test builds"
	@echo ""
	@echo "  Architecture: linux-amd64 linux-arm64 linux-386;"
	@echo "                termux-arm64 termux-arm termux-amd64;"
	@echo "                windows-amd64 windows-arm64 windows-386;"
	@echo "                android-arm64 android-arm android-amd64."
	@echo ""
	@echo "  Output: $(DIST)/$(APP)-<OS>-<arch>[.exe|.apk]"
