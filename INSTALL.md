# ThresholdCrush v0.2.0 (VST3) Install Guide

Recommended direction: use the **macOS system VST3 folder** so every VST3-capable DAW finds the same plugin.

## What You Get

The build produces a single VST3 bundle:
- `ThresholdCrush.vst3`

This VST3 is what FL Studio, Ableton Live, REAPER, Bitwig, Cubase/Nuendo, Studio One, etc. will load on macOS.

## Where The Built Plugin Is

After a Release build:
- `/Users/ericlewis/Developer/JayJay300/build/ThresholdCrush_artefacts/VST3/ThresholdCrush.vst3`

## Install (macOS)

VST3 install locations:
- System-wide: `/Library/Audio/Plug-Ins/VST3/`
- Per-user: `~/Library/Audio/Plug-Ins/VST3/`

Manual install example:
```sh
cp -R /Users/ericlewis/Developer/JayJay300/build/ThresholdCrush_artefacts/VST3/ThresholdCrush.vst3 \
  ~/Library/Audio/Plug-Ins/VST3/
```

If the DAW refuses to load it due to quarantine/Gatekeeper:
```sh
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/ThresholdCrush.vst3
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/ThresholdCrush.vst3
```

## Installers Already Generated (macOS)

These were created in:
- `/Users/ericlewis/Developer/JayJay300/dist/`

Files:
- `ThresholdCrush-0.2.0-macOS-VST3.pkg`
  - Installs to: `/Library/Audio/Plug-Ins/VST3/`
- `ThresholdCrush-0.2.0-macOS-VST3.dmg`
  - Drag `ThresholdCrush.vst3` into either VST3 folder above
- `ThresholdCrush-0.2.0-macOS-VST3.zip`
  - Contains `ThresholdCrush.vst3` for manual copy

## How To Generate The Installers (macOS)

Build Release first:
```sh
cd /Users/ericlewis/Developer/JayJay300
cmake -S . -B build
cmake --build build --config Release -j 8
```

Then package (creates `.pkg`, `.zip`, `.dmg` in `dist/`):
```sh
cd /Users/ericlewis/Developer/JayJay300
bash scripts/package_macos_vst3.sh
```

Notes:
- The script auto-detects the version from `CMakeLists.txt` and will build Release if needed.
- You can also override: `bash scripts/package_macos_vst3.sh 0.2.0`

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
- **Logic Pro**: requires **AU**, and this project currently builds **VST3 only**.
