# ThresholdCrush (VST3)

A VST3 effect that stays clean under a threshold, then progressively bitcrushes the signal as it exceeds the threshold (like clipping, but with bit-depth reduction).

## Signal Flow
```text
Input
  -> stereo-linked detector (threshold, attack, release)
  -> overshoot drives: downsample-hold -> bitcrush -> optional clip
  -> wet/dry mix
Output
```

## Controls

- **Threshold (dB)**: below this level, the signal is clean.
- **Attack (ms)**: how quickly the detector reacts to overshoot (compressor-like attack). Faster attack = bitcrush engages sooner on transients.
- **Release (ms)**: how quickly the detector recovers after overshoot.
- **Crush Range (dB)**: how many dB above threshold it takes to reach maximum crush (smaller = more aggressive).
- **Min Bit Depth (bits)**: the lowest bit depth reached at maximum overshoot.
- **Downsample Max (x)**: at maximum overshoot, the wet path is sample-held for up to this many samples (1x = off).
- **Clip (toggle)**: optionally add clipping after the crush stages.
- **Clip Drive (dB)**: maximum clip drive at maximum overshoot (actual drive scales with overshoot).
- **Clip Style (%)**: 0% soft (tanh) → 100% hard (clamp).
- **Mix (%)**: dry/wet blend for the crushed signal.

## Build (macOS)

```sh
cmake -S . -B build
cmake --build build --config Release -j 8
```

VST3 output is under `build/ThresholdCrush_artefacts/VST3/`.

## Run Tests

```sh
cmake -S . -B build
cmake --build build --config Debug -j 8
./build/ThresholdCrushTests_artefacts/ThresholdCrushTests
```

## Install / DAWs

See `INSTALL.md`.

## Package (macOS)

```sh
bash scripts/package_macos_vst3.sh
```

The packager auto-detects the version from `CMakeLists.txt` and will build Release if needed.
