#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="$ROOT_DIR/dist"
BUILD="$ROOT_DIR/build"

PKG_VERSION="${1:-${PKG_VERSION:-}}"
if [[ -z "${PKG_VERSION}" ]]; then
  # Parse from CMakeLists.txt: set(THRESHOLDCRUSH_VERSION "X.Y.Z" ...)
  PKG_VERSION="$(perl -ne 'print $1 and exit if /set\(THRESHOLDCRUSH_VERSION\s+\"?([0-9]+\.[0-9]+\.[0-9]+)\"?/' "$ROOT_DIR/CMakeLists.txt" 2>/dev/null || true)"
fi
PKG_VERSION="${PKG_VERSION#v}"
if [[ -z "${PKG_VERSION}" ]]; then
  echo "Missing version. Fix by:" >&2
  echo "1) Ensure CMakeLists.txt has: set(THRESHOLDCRUSH_VERSION \"X.Y.Z\" ...)" >&2
  echo "2) Or run: bash scripts/package_macos.sh X.Y.Z (or vX.Y.Z)" >&2
  exit 1
fi

VST3_SRC="$BUILD/ThresholdCrush_artefacts/VST3/ThresholdCrush.vst3"
AU_SRC="$BUILD/ThresholdCrush_artefacts/AU/ThresholdCrush.component"

# Some generators place artefacts under a configuration subdir.
VST3_SRC_ALT="$BUILD/ThresholdCrush_artefacts/Release/VST3/ThresholdCrush.vst3"
AU_SRC_ALT="$BUILD/ThresholdCrush_artefacts/Release/AU/ThresholdCrush.component"

if [[ ! -d "$VST3_SRC" && -d "$VST3_SRC_ALT" ]]; then VST3_SRC="$VST3_SRC_ALT"; fi
if [[ ! -d "$AU_SRC" && -d "$AU_SRC_ALT" ]]; then AU_SRC="$AU_SRC_ALT"; fi

need_release_build=0
if [[ ! -f "$BUILD/CMakeCache.txt" ]]; then
  need_release_build=1
else
  # For single-config generators, enforce Release via CMAKE_BUILD_TYPE.
  gen="$(perl -ne 'print $1 and exit if /^CMAKE_GENERATOR:INTERNAL=(.*)$/' "$BUILD/CMakeCache.txt" 2>/dev/null || true)"
  bt="$(perl -ne 'print $1 and exit if /^CMAKE_BUILD_TYPE:STRING=(.*)$/' "$BUILD/CMakeCache.txt" 2>/dev/null || true)"
  if [[ "${gen}" == "Unix Makefiles" || "${gen}" == "Ninja" ]]; then
    if [[ "${bt}" != "Release" ]]; then need_release_build=1; fi
  fi
fi
if [[ ! -d "$VST3_SRC" || ! -d "$AU_SRC" ]]; then need_release_build=1; fi

if [[ "${need_release_build}" -eq 1 ]]; then
  echo "Preparing Release build..." >&2
  cmake -S "$ROOT_DIR" -B "$BUILD" -DTHRESHOLDCRUSH_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
  cmake --build "$BUILD" --config Release -j 8
fi

if [[ ! -d "$VST3_SRC" && -d "$VST3_SRC_ALT" ]]; then VST3_SRC="$VST3_SRC_ALT"; fi
if [[ ! -d "$AU_SRC" && -d "$AU_SRC_ALT" ]]; then AU_SRC="$AU_SRC_ALT"; fi

if [[ ! -d "$VST3_SRC" || ! -d "$AU_SRC" ]]; then
  echo "Build completed, but artefacts are still missing." >&2
  echo "VST3: $VST3_SRC" >&2
  echo "AU:   $AU_SRC" >&2
  exit 1
fi

rm -rf "$DIST"
mkdir -p "$DIST"

PKGROOT="$DIST/pkgroot"
VST3_DST="$PKGROOT/Library/Audio/Plug-Ins/VST3"
AU_DST="$PKGROOT/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3_DST" "$AU_DST"

cp -R "$VST3_SRC" "$VST3_DST/"
cp -R "$AU_SRC" "$AU_DST/"

# Ad-hoc sign the staged bundles to reduce Gatekeeper friction (not notarized).
codesign --force --deep --sign - "$VST3_DST/ThresholdCrush.vst3" || true
codesign --force --deep --sign - "$AU_DST/ThresholdCrush.component" || true

VST3_STAGED="$VST3_DST/ThresholdCrush.vst3"
AU_STAGED="$AU_DST/ThresholdCrush.component"

PKG_ID="com.yellowballoon.thresholdcrush.auv2vst3"
PKG_OUT="$DIST/ThresholdCrush-$PKG_VERSION-macOS-AU+VST3.pkg"

pkgbuild \
  --root "$PKGROOT" \
  --identifier "$PKG_ID" \
  --version "$PKG_VERSION" \
  --install-location / \
  "$PKG_OUT"

# Zip for manual install.
ZIP_DIR="$DIST/ThresholdCrush-$PKG_VERSION-macOS-AU+VST3"
mkdir -p "$ZIP_DIR"
cp -R "$VST3_STAGED" "$ZIP_DIR/"
cp -R "$AU_STAGED" "$ZIP_DIR/"
cat > "$ZIP_DIR/INSTALL.txt" <<'TXT'
ThresholdCrush (AU + VST3)

Install (macOS):
- VST3 (system-wide): /Library/Audio/Plug-Ins/VST3/
- AU   (system-wide): /Library/Audio/Plug-Ins/Components/

- VST3 (per-user):    ~/Library/Audio/Plug-Ins/VST3/
- AU   (per-user):    ~/Library/Audio/Plug-Ins/Components/

Then rescan plugins in your DAW.
Logic Pro scans AUs on launch; if it doesn't appear, restart Logic (or rescan Audio Units).

If a DAW refuses to load due to quarantine:
  xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/ThresholdCrush.vst3
  xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/Components/ThresholdCrush.component
(or the same paths under ~/Library)
TXT
( cd "$DIST" && /usr/bin/zip -r "ThresholdCrush-$PKG_VERSION-macOS-AU+VST3.zip" "ThresholdCrush-$PKG_VERSION-macOS-AU+VST3" )

# Simple DMG.
DMG_SRC="$DIST/dmgsrc"
mkdir -p "$DMG_SRC"
cp -R "$VST3_STAGED" "$DMG_SRC/"
cp -R "$AU_STAGED" "$DMG_SRC/"
cat > "$DMG_SRC/INSTALL.txt" <<'TXT'
ThresholdCrush (AU + VST3)

Install (macOS):
- VST3 (system-wide): /Library/Audio/Plug-Ins/VST3/
- AU   (system-wide): /Library/Audio/Plug-Ins/Components/

- VST3 (per-user):    ~/Library/Audio/Plug-Ins/VST3/
- AU   (per-user):    ~/Library/Audio/Plug-Ins/Components/

Then rescan plugins in your DAW.
Logic Pro scans AUs on launch; if it doesn't appear, restart Logic (or rescan Audio Units).

If a DAW refuses to load due to quarantine:
  xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/ThresholdCrush.vst3
  xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/Components/ThresholdCrush.component
(or the same paths under ~/Library)
TXT

hdiutil create -volname "ThresholdCrush $PKG_VERSION" -srcfolder "$DMG_SRC" -ov -format UDZO "$DIST/ThresholdCrush-$PKG_VERSION-macOS-AU+VST3.dmg" >/dev/null

echo "Wrote: $PKG_OUT"
echo "Wrote: $DIST/ThresholdCrush-$PKG_VERSION-macOS-AU+VST3.zip"
echo "Wrote: $DIST/ThresholdCrush-$PKG_VERSION-macOS-AU+VST3.dmg"
