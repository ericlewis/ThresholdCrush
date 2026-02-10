# CLAUDE.md

## Project Overview

ThresholdCrush is a macOS audio plugin (AU + VST3) that applies progressive bitcrushing to audio signals exceeding a user-defined threshold. Below the threshold, audio passes through clean; above it, overshoot drives bit-depth reduction, sample-rate decimation, and optional clipping. Built with JUCE 8.0.12, C++17, and CMake.

**Company:** Yellowballoon
**Bundle ID:** `com.yellowballoon.thresholdcrush`
**Version:** 0.2.0 (defined in `CMakeLists.txt` via `THRESHOLDCRUSH_VERSION`)

## Build Commands

```sh
# Release build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8

# Build + run tests
cmake -S . -B build -DTHRESHOLDCRUSH_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 8
./build/ThresholdCrushTests_artefacts/ThresholdCrushTests

# Universal macOS binary (arm64 + x86_64)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build -j 8

# Package installers (.pkg, .dmg, .zip)
bash scripts/package_macos.sh
```

Test binary path varies between single-config (Ninja/Make) and multi-config (Xcode) generators. Check all possible locations:
- `build/ThresholdCrushTests_artefacts/ThresholdCrushTests` (single-config)
- `build/ThresholdCrushTests_artefacts/Debug/ThresholdCrushTests` (multi-config, Debug)
- `build/ThresholdCrushTests_artefacts/Release/ThresholdCrushTests` (multi-config, Release)

## Project Structure

```
Source/
  ThresholdCrushDSP.h      # Core DSP algorithm (header-only, no JUCE processor dependency)
  PluginProcessor.h/.cpp   # JUCE AudioProcessor: parameter layout, state, presets, metering
  PluginEditor.h/.cpp      # JUCE AudioProcessorEditor: UI layout, knobs, labels, attachments
Tests/
  ThresholdCrushDSPTests.cpp      # 10 DSP unit tests (JUCE UnitTest framework)
  ThresholdCrushLayoutTests.cpp   # UI layout regression tests
scripts/
  package_macos.sh         # Builds + packages AU+VST3 for distribution
  package_macos_vst3.sh    # VST3-only packaging (deprecated)
.github/workflows/
  release.yml              # CI: build, test, package, release on tag/main push
```

## Architecture

**Signal flow:** Input -> stereo-linked peak detector (attack/release) -> overshoot calculation -> downsample-hold -> bitcrush quantization -> optional clip (soft tanh / hard clamp) -> wet/dry mix -> Output.

**Key design decisions:**
- `ThresholdCrushDSP` is a standalone header with no JUCE AudioProcessor coupling. It can be tested and reused independently.
- Stereo-linked detection: `max(|L|, |R|)` drives a shared envelope, so a quiet channel still gets crushed if the other is loud.
- Per-sample processing with no block latency. Envelope uses exponential smoothing (attack/release time constants).
- Metering uses `std::atomic<float>` with `memory_order_relaxed` for lock-free audio-to-UI communication.
- Factory presets are defined as static data in `PluginProcessor.cpp` and exposed via `getNumPrograms()`/`getProgramName()`.

**Plugin parameters** (IDs used in `AudioProcessorValueTreeState`):

| ID | Range | Default |
|----|-------|---------|
| `threshold_db` | -60 to 0 dB | -18 |
| `attack_ms` | 0.5 to 200 ms | 10 |
| `release_ms` | 5 to 1000 ms | 120 |
| `crush_range_db` | 6 to 48 dB | 24 |
| `min_bit_depth` | 2 to 16 | 4 |
| `downsample_max` | 1 to 64 | 1 |
| `clip_enabled` | bool | false |
| `clip_drive_db` | 0 to 24 dB | 12 |
| `clip_style` | 0 to 100% | 50 |
| `mix` | 0 to 100% | 100 |

## Testing

Tests use the **JUCE UnitTest** framework (not Catch2 or Google Test). Test classes inherit from `juce::UnitTest` and register via static construction. The test binary runs all registered tests and returns exit code 0 on success, 1 on any failure.

**DSP tests** verify: clean passthrough under threshold, crush scaling with overshoot, stereo linking, attack/release envelope behavior, bit-depth range effects, mix blending, downsample-hold blocking, clip toggle, soft vs. hard clip differences.

**Layout tests** verify: knob sizing, no overlapping components, all controls within editor bounds.

**Testing patterns:**
- Feed a steady-state signal for ~2000 samples to charge the envelope before asserting on transient behavior.
- Compare error magnitudes (not exact float equality) for crush-amount assertions.
- Use `expectEquals` for bit-identical passthrough tests, `expect` for inequality/magnitude checks.

## Coding Conventions

- **C++17**, `#pragma once` header guards
- **Naming:** `camelCase` for variables/methods, `PascalCase` for types/classes
- **Includes:** `<JuceHeader.h>` first, then local headers
- **Audio types:** `float` for sample data, `double` for sample rate
- **JUCE macros:** `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` on all processor/editor classes
- **Attachments:** `std::unique_ptr<SliderAttachment>` / `std::unique_ptr<ButtonAttachment>`, stored as class members
- **Test seams:** Public `debug*()` accessor methods on the editor (e.g., `debugThresholdSlider()`) for testing without brittle child-walking
- **Anonymous namespaces** for test classes and file-local helpers
- **Knob style:** `RotaryHorizontalVerticalDrag`, text box below
- **No MIDI, no web browser, no curl** (`JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0`)
- **Denormal protection:** `juce::ScopedNoDenormals` in `processBlock`

## CI/CD

The GitHub Actions workflow (`.github/workflows/release.yml`) triggers on:
- Tag push (`v*.*.*` or `*.*.*`)
- Push to `main` (auto-bumps patch version)
- Release publication
- Manual `workflow_dispatch`

Pipeline: checkout -> compute version -> create/push tag -> cmake configure (universal, tests on) -> build -> run tests -> package -> verify artifacts -> auval (best-effort) -> upload artifacts -> create GitHub release.

## Common Pitfalls

- Build artifacts land in different subdirectories depending on the CMake generator. Always check both `build/ThresholdCrush_artefacts/VST3/` and `build/ThresholdCrush_artefacts/Release/VST3/`.
- JUCE is fetched via `FetchContent` at configure time. The first `cmake -S . -B build` will clone JUCE 8.0.12, which requires network access.
- The `auval` validation step in CI is best-effort (`continue-on-error: true`) since it depends on macOS audio infrastructure.
- Parameter IDs use underscores (e.g., `threshold_db`), not camelCase. These IDs are serialized in user presets, so they must not change.
