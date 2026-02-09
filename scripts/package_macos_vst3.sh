#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Deprecated: prefer bash scripts/package_macos.sh (AU+VST3)." >&2
echo "Packaging VST3-only..." >&2

PKG_VERSION="${1:-${PKG_VERSION:-}}"
PKG_VERSION="${PKG_VERSION#v}"
bash "$ROOT_DIR/scripts/package_macos.sh" "${PKG_VERSION:-}"

# Drop AU files from the dist output to keep this script \"VST3-only\" for compatibility.
# We repackage from the combined dist directory to VST3-only names.
VERSION="${PKG_VERSION}"
if [[ -z "${VERSION}" ]]; then
  VERSION="$(perl -ne 'print $1 and exit if /project\(ThresholdCrush\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\)/' "$ROOT_DIR/CMakeLists.txt" 2>/dev/null || true)"
fi
if [[ -z "${VERSION}" ]]; then
  echo "Could not determine version for VST3-only compatibility packaging." >&2
  exit 1
fi

DIST="$ROOT_DIR/dist"

VST3_SRC="$ROOT_DIR/build/ThresholdCrush_artefacts/VST3/ThresholdCrush.vst3"
VST3_SRC_ALT="$ROOT_DIR/build/ThresholdCrush_artefacts/Release/VST3/ThresholdCrush.vst3"
if [[ ! -d "$VST3_SRC" && -d "$VST3_SRC_ALT" ]]; then VST3_SRC="$VST3_SRC_ALT"; fi
if [[ ! -d "$VST3_SRC" ]]; then
  echo "Missing VST3 artefact: $VST3_SRC" >&2
  exit 1
fi

ZIP_DIR="$DIST/ThresholdCrush-${VERSION}-macOS-VST3"
rm -rf "$ZIP_DIR"
mkdir -p "$ZIP_DIR"
cp -R "$VST3_SRC" "$ZIP_DIR/"
cat > "$ZIP_DIR/INSTALL.txt" <<'TXT'
ThresholdCrush.vst3

Install (macOS):
- System-wide: copy ThresholdCrush.vst3 to /Library/Audio/Plug-Ins/VST3/
- Per-user:    copy ThresholdCrush.vst3 to ~/Library/Audio/Plug-Ins/VST3/

Then rescan plugins in your DAW.

If a DAW refuses to load due to quarantine:
  xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/ThresholdCrush.vst3
(or the same path under ~/Library)
TXT

( cd "$DIST" && /usr/bin/zip -r "ThresholdCrush-${VERSION}-macOS-VST3.zip" "ThresholdCrush-${VERSION}-macOS-VST3" ) >/dev/null
hdiutil create -volname "ThresholdCrush ${VERSION} (VST3)" -srcfolder "$ZIP_DIR" -ov -format UDZO "$DIST/ThresholdCrush-${VERSION}-macOS-VST3.dmg" >/dev/null

# Create VST3-only pkg by staging into /Library/Audio/Plug-Ins/VST3 only.
PKGROOT="$DIST/pkgroot_vst3only"
TARGET_DIR="$PKGROOT/Library/Audio/Plug-Ins/VST3"
rm -rf "$PKGROOT"
mkdir -p "$TARGET_DIR"
cp -R "$VST3_SRC" "$TARGET_DIR/"
codesign --force --deep --sign - "$TARGET_DIR/ThresholdCrush.vst3" || true

pkgbuild \
  --root "$PKGROOT" \
  --identifier "com.jayjay.thresholdcrush.vst3" \
  --version "$VERSION" \
  --install-location / \
  "$DIST/ThresholdCrush-${VERSION}-macOS-VST3.pkg"

# Clean up combined outputs produced by package_macos.sh; keep this script VST3-only.
rm -rf \
  "$DIST/ThresholdCrush-${VERSION}-macOS-AU+VST3"* \
  "$DIST/dmgsrc" \
  "$DIST/pkgroot"

echo "Wrote: $DIST/ThresholdCrush-${VERSION}-macOS-VST3.pkg"
echo "Wrote: $DIST/ThresholdCrush-${VERSION}-macOS-VST3.zip"
echo "Wrote: $DIST/ThresholdCrush-${VERSION}-macOS-VST3.dmg"
