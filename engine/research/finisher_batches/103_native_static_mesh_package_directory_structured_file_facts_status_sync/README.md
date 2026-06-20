# 103 - Native Static Mesh Package Directory Structured File Facts Status Sync

## Goal

Sync planning and API docs after source commit
`43ac5cb6 Expose native static mesh package file facts`.

## Integrated Surface

- Added structured package-directory path facts:
  - `NativeStaticMeshExportPackageDirectoryPathFacts`
  - `ReadNativeStaticMeshExportPackageDirectoryPathFacts(...)`
  - `NativeStaticMeshExportPackageDirectoryAssetFacts`
- Extended `NativeStaticMeshExportPackageDirectoryReport` with:
  - `packageManifestFactsRecorded`
  - `packageManifestFacts`
  - `manifestFactsRecorded`
  - `manifestFacts`
  - `assetFacts`
- Replaced the report builder's local path-facts lambda with the structured
  helper and changed text rendering to consume the stored facts.

## Semantics

- `packageManifestFactsRecorded` / `packageManifestFacts` are populated when
  `report.read.packageManifestPath` is available, including missing or malformed
  package sidecar cases.
- `manifestFactsRecorded` / `manifestFacts` are populated when
  `report.read.manifestPath` is available, including missing or malformed nested
  manifest cases.
- `assetFacts` mirrors `report.read.assets` in package-declared row order and
  stores file facts for package-declared asset paths only.
- Nested mesh-manifest-declared-only rows do not receive file facts.
- Report text output is preserved exactly: no new rows, new summary fields,
  row-order changes, status/count/token changes, or newline changes.

## Verification

- `git show --stat --oneline --name-only 43ac5cb6` showed only
  `NativeStaticMeshExportPackageDirectoryReport.hpp` and focused report tests.
- `git diff --check 43ac5cb6^ 43ac5cb6` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid package directory report smoke passed with unchanged text including
  package manifest facts, manifest read/comparison facts, and package asset rows.
- Missing nested manifest smoke passed with unchanged invalid-read diagnostics
  and package asset rows.
- Missing declared asset smoke passed with `cube exists=0 regularFile=0 bytes=0`.
- Focused coverage includes valid export, missing package sidecar, malformed
  package sidecar, missing nested mesh manifest, missing declared package asset,
  and directory-at-declared-asset.

## Boundaries Preserved

- No report text changes, new rows, summary fields, row-order changes,
  statuses, counts, tokens, or newline changes.
- No `readOk()` changes, CLI exit changes, core `issues=` changes, package
  directory reader status/data changes, exact verification/export/package
  acceptance/generated sidecar/write-policy changes, or nested
  mesh-manifest-only path facts.
- No `.igmesh` loading, geometry validation, policy/built-in reconstruction,
  discovery/scanning/catalog/registry, exact-extra-file rejection, renderer,
  model-slot, schema, gameplay, glTF/glb/JSON parser scope, source, CMake,
  renderer, fixture/asset, or Spark ledger changes.

## Finisher Verification

- `git merge-base --is-ancestor 43ac5cb6 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
