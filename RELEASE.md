# Releasing ThresholdCrush

## Automatic: push to main

Pushing to `main` runs **Build + Release (macOS AU+VST3)** and publishes a Release automatically.

Versioning:
- `CMakeLists.txt` defines the base version via `THRESHOLDCRUSH_VERSION` (use this to bump major/minor).
- On `main` pushes, the workflow auto-tags the **next patch** version for that major/minor (e.g. `0.2.0` -> `0.2.1`), then builds/packages using that version.

## Recommended: tag-based release

1. Bump `THRESHOLDCRUSH_VERSION` in `CMakeLists.txt` if needed.
2. Commit.
3. Tag and push:
   ```sh
   # Either style is supported:
   #   vX.Y.Z  (recommended)
   #   X.Y.Z
   git tag vX.Y.Z
   git push --tags
   ```
4. GitHub Actions workflow **Build + Release (macOS AU+VST3)** will:
   - build + run tests
   - package AU+VST3 (`dist/*.pkg`, `dist/*.dmg`, `dist/*.zip`)
   - create/update the GitHub Release and attach the artifacts

Note: publishing a GitHub Release (from the web UI) will also trigger the workflow for that release tag.

## Manual: workflow_dispatch

Run the workflow manually from GitHub Actions:
- Optionally provide `version` (e.g. `0.2.0` or `v0.2.0`)
- Optionally enable `prerelease`

The workflow will create/push the tag `v<version>` and publish the Release with assets.
