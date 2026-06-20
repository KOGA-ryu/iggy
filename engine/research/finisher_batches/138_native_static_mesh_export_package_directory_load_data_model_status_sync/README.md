# 138 - Native Static Mesh Export Package Directory Load Data Model Status Sync

## Goal

Sync planning and API docs after source commit
`4ed4906d Add static mesh package directory load model`.

## Integrated Surface

- Added header-only explicit-directory load API
  `LoadNativeStaticMeshExportPackageDirectory(...)` in
  `NativeStaticMeshExportPackageDirectoryLoad.hpp`.
- This is the first explicit package-directory consumption API:
  exported package directory -> package sidecar -> nested mesh manifest ->
  loaded CPU `NativeStaticMeshAsset` rows.
- This is consumption-oriented data-model work, not helper cleanup.

## Load Flow

- Reads the package directory with `ReadNativeStaticMeshExportPackageDirectory(...)`.
- Reads the projected nested mesh manifest with
  `ReadNativeStaticMeshExportManifestFile(...)`.
- Compares package rows to parsed mesh manifest rows with
  `CompareNativeStaticMeshExportPackageDirectoryManifestRows(...)`.
- Loads package-declared `.igmesh` files with the existing asset loader.

## Result Data Model

- `NativeStaticMeshExportPackageDirectoryLoadResult` carries:
  - top-level status;
  - supplied directory;
  - package directory read result;
  - nested mesh manifest read result;
  - package-vs-manifest comparison result;
  - loaded asset rows;
  - loaded count;
  - issue count;
  - `loaded()` predicate.
- Per-asset rows carry:
  - package asset identity and path;
  - manifest row;
  - expected vertex/index counts;
  - actual loaded vertex/index counts;
  - load issues;
  - issue count;
  - loaded `NativeStaticMeshAsset`;
  - per-row status;
  - `loaded()` predicate.

## Status Boundaries

- Top-level statuses cover `Loaded`, `PackageDirectoryReadFailed`,
  `ManifestReadFailed`, `ManifestComparisonFailed`, `MissingAsset`,
  `AssetLoadFailed`, and `GeometryMismatch`.
- Per-asset statuses cover `Loaded`, `MissingAsset`, `AssetLoadFailed`, and
  `GeometryMismatch`.

## Boundaries Preserved

- Loading is explicit-directory only.
- Loading follows package-declared files only after package/manifest rows compare
  cleanly.
- Unrelated extra files are ignored.
- No directory scanning, package discovery, catalog, or registry behavior.
- No printing or throwing for expected failures.
- No file mutation, package repair, renderer behavior, CLI/parser/help/dispatch,
  static model policy conversion, export write policy, sidecar generation, exact
  verifier/report behavior, package-directory reader/report behavior, CMake,
  assets/fixtures, schema/material/texture/normal/UV/animation scope, package
  acceptance beyond this data-model API, or glTF/glb/JSON parser/dependency
  changes.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_reader_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_reader_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_file_export_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_file_export_tests --output-on-failure` passed.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Stop Note

- Stop after this docs sync.
- Do not assume an automatic next packet should be opened unless the user
  resumes.

## Finisher Verification

- `git merge-base --is-ancestor 4ed4906d HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
