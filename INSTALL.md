# ThresholdCrush v0.2.0 (AU + VST3) Install Guide

Recommended direction: install to the **macOS system plugin folders** so every DAW finds the same plugin.

## What You Get

The build produces:
- `ThresholdCrush.vst3` (VST3)
- `ThresholdCrush.component` (Audio Unit v2, for Logic Pro)

VST3 hosts (FL Studio, Ableton Live, REAPER, Bitwig, Cubase/Nuendo, Studio One, etc.) load the `.vst3`.
Logic Pro loads the `.component`.

## Where The Built Plugin Is

After a Release build:
- VST3:
  - `/Users/ericlewis/Developer/JayJay300/build/ThresholdCrush_artefacts/VST3/ThresholdCrush.vst3`
  - `/Users/ericlewis/Developer/JayJay300/build/ThresholdCrush_artefacts/Release/VST3/ThresholdCrush.vst3`
- AU:
  - `/Users/ericlewis/Developer/JayJay300/build/ThresholdCrush_artefacts/AU/ThresholdCrush.component`
  - `/Users/ericlewis/Developer/JayJay300/build/ThresholdCrush_artefacts/Release/AU/ThresholdCrush.component`

## Install (macOS)

VST3 install locations:
- System-wide: `/Library/Audio/Plug-Ins/VST3/`
- Per-user: `~/Library/Audio/Plug-Ins/VST3/`

AU install locations:
- System-wide: `/Library/Audio/Plug-Ins/Components/`
- Per-user: `~/Library/Audio/Plug-Ins/Components/`

Manual install example:
```sh
cp -R /Users/ericlewis/Developer/JayJay300/build/ThresholdCrush_artefacts/Release/VST3/ThresholdCrush.vst3 \
  ~/Library/Audio/Plug-Ins/VST3/

cp -R /Users/ericlewis/Developer/JayJay300/build/ThresholdCrush_artefacts/Release/AU/ThresholdCrush.component \
  ~/Library/Audio/Plug-Ins/Components/
```

If the DAW refuses to load it due to quarantine/Gatekeeper:
```sh
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/ThresholdCrush.vst3
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/ThresholdCrush.vst3

xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/ThresholdCrush.component
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/Components/ThresholdCrush.component
```

## Installers Already Generated (macOS)

These were created in:
- `/Users/ericlewis/Developer/JayJay300/dist/`

Files:
- `ThresholdCrush-0.2.0-macOS-AU+VST3.pkg`
  - Installs to:
    - `/Library/Audio/Plug-Ins/VST3/`
    - `/Library/Audio/Plug-Ins/Components/`
- `ThresholdCrush-0.2.0-macOS-AU+VST3.dmg`
  - Drag `ThresholdCrush.vst3` and `ThresholdCrush.component` into the folders above
- `ThresholdCrush-0.2.0-macOS-AU+VST3.zip`
  - Contains both bundles for manual copy

## How To Generate The Installers (macOS)

Package (creates `.pkg`, `.zip`, `.dmg` in `dist/`). The script will build Release if needed:
```sh
cd /Users/ericlewis/Developer/JayJay300
bash scripts/package_macos.sh
```

Notes:
- The script auto-detects the version from `CMakeLists.txt` and will build Release if needed.
- You can also override: `bash scripts/package_macos.sh 0.2.0` (or `v0.2.0`)
- GitHub Releases are built as **universal** (`arm64` + `x86_64`). To build universal locally:
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`

If you prefer to build manually first:
```sh
cd /Users/ericlewis/Developer/JayJay300
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
```

## DAW Rescan Instructions (macOS)

All of these DAWs will see the plugin once it’s in a VST3 folder above, but each has its own rescan UI.

### FL Studio (macOS)
1. `Options > Manage plugins`
2. Run a rescan (`Find installed plugins` / `Rescan`)
3. Look for `ThresholdCrush`

### Ableton Live (macOS)
1. `Settings/Preferences > Plug-Ins`
2. Enable **VST3 Plug-In System Folders**
3. Click **Rescan**

### REAPER (macOS)
1. `Preferences > Plug-ins > VST`
2. Add the VST3 path if needed (usually auto-detected)
3. Click **Re-scan**

### Bitwig Studio (macOS)
1. `Settings > Locations > Plug-ins`
2. Ensure VST3 locations are enabled
3. **Rescan**

### Cubase / Nuendo (macOS)
1. Install to the standard VST3 folder (Cubase uses system VST3 locations)
2. `Studio > VST Plug-in Manager`
3. Rescan if needed

### Studio One (macOS)
1. `Options/Preferences > Locations > VST Plug-ins`
2. Rescan
3. If it’s blacklisted, reset blacklist and rescan

## Windows / Logic Notes

- **Windows**: not packaged here yet; would require a Windows build (VST3 goes to `C:\\Program Files\\Common Files\\VST3\\`).
- **Logic Pro**: uses the AU (`.component`). Logic scans AUs on launch; if it doesn't appear, restart Logic (or rescan Audio Units).
