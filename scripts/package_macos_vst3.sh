#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="$ROOT_DIR/dist"
BUILD="$ROOT_DIR/build"

PKG_VERSION="${1:-${PKG_VERSION:-}}"
if [[ -z "${PKG_VERSION}" ]]; then
  # Best-effort fallback: parse from CMakeLists.txt (project(... VERSION X.Y.Z)).
  PKG_VERSION="$(perl -ne 'print $1 and exit if /project\\(ThresholdCrush\\s+VERSION\\s+([0-9]+\\.[0-9]+\\.[0-9]+)\\)/' \"$ROOT_DIR/CMakeLists.txt\" 2>/dev/null || true)"
fi
if [[ -z "${PKG_VERSION}" ]]; then
  echo "Missing version. Pass as arg: package_macos_vst3.sh 0.2.0 (or set PKG_VERSION env)." >&2
  exit 1
fi

VST3_SRC="$BUILD/ThresholdCrush_artefacts/VST3/ThresholdCrush.vst3"

if [[ ! -d "$VST3_SRC" ]]; then
  echo "Missing VST3 at: $VST3_SRC" >&2
  echo "Build first: cmake --build $BUILD --config Release" >&2
  exit 1
fi

rm -rf "$DIST"
mkdir -p "$DIST"

# Stage for pkg: install into the standard system VST3 folder.
PKGROOT="$DIST/pkgroot"
TARGET_DIR="$PKGROOT/Library/Audio/Plug-Ins/VST3"
mkdir -p "$TARGET_DIR"
cp -R "$VST3_SRC" "$TARGET_DIR/"

# Ad-hoc sign the staged bundle to reduce Gatekeeper friction (not notarized).
codesign --force --deep --sign - "$TARGET_DIR/ThresholdCrush.vst3" || true

PKG_ID="com.jayjay.thresholdcrush.vst3"
PKG_OUT="$DIST/ThresholdCrush-$PKG_VERSION-macOS-VST3.pkg"

pkgbuild \
  --root "$PKGROOT" \
  --identifier "$PKG_ID" \
  --version "$PKG_VERSION" \
  --install-location / \
  "$PKG_OUT"

# Zip for manual install.
ZIP_DIR="$DIST/ThresholdCrush-$PKG_VERSION-macOS-VST3"
mkdir -p "$ZIP_DIR"
cp -R "$VST3_SRC" "$ZIP_DIR/"
( cd "$DIST" && /usr/bin/zip -r "ThresholdCrush-$PKG_VERSION-macOS-VST3.zip" "ThresholdCrush-$PKG_VERSION-macOS-VST3" )

# Simple DMG.
DMG_SRC="$DIST/dmgsrc"
mkdir -p "$DMG_SRC"
cp -R "$VST3_SRC" "$DMG_SRC/"
cat > "$DMG_SRC/INSTALL.txt" <<'TXT'
ThresholdCrush.vst3

Install (macOS):
- System-wide: copy ThresholdCrush.vst3 to /Library/Audio/Plug-Ins/VST3/
- Per-user:    copy ThresholdCrush.vst3 to ~/Library/Audio/Plug-Ins/VST3/

Then rescan plugins in your DAW.

If a DAW refuses to load due to quarantine:
  xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/ThresholdCrush.vst3
(or the same path under ~/Library)
TXT

hdiutil create -volname "ThresholdCrush $PKG_VERSION" -srcfolder "$DMG_SRC" -ov -format UDZO "$DIST/ThresholdCrush-$PKG_VERSION-macOS-VST3.dmg" >/dev/null

echo "Wrote: $PKG_OUT"
echo "Wrote: $DIST/ThresholdCrush-$PKG_VERSION-macOS-VST3.zip"
echo "Wrote: $DIST/ThresholdCrush-$PKG_VERSION-macOS-VST3.dmg"
