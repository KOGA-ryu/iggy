# 104 - Native Static Mesh Package Directory Report Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`554b746d Extract native static mesh package report renderer`.

## Integrated Surface

- Added header-only renderer helper:
  `BuildNativeStaticMeshExportPackageDirectoryReportText(const NativeStaticMeshExportPackageDirectoryReport &report)`.
- Moved existing package directory report text serialization out of
  `BuildNativeStaticMeshExportPackageDirectoryReport(...)` into the helper.
- `BuildNativeStaticMeshExportPackageDirectoryReport(...)` remains responsible
  for collecting structured report data and then setting
  `report.text = BuildNativeStaticMeshExportPackageDirectoryReportText(report)`.
- The renderer uses existing structured report fields only:
  - `read`
  - `packageManifestFacts`
  - `manifestFacts`
  - package manifest read issues
  - `manifestRead`
  - `manifestComparison`
  - `assetFacts`

## Preserved Text Contract

- Report text is preserved byte-for-byte for current cases.
- Preserved output includes summary row, field order, issue rows, manifest asset
  rows, comparison rows, package asset rows, counts, tokens, paths, and newlines.
- Focused tests compare renderer output to `report.text` for valid export,
  missing package sidecar, malformed package sidecar, missing nested manifest,
  combined comparison mismatch, and missing declared asset.

## Verification

- `git show --stat --oneline --name-only 554b746d` showed only
  `NativeStaticMeshExportPackageDirectoryReport.hpp` and focused report tests.
- `git diff --check 554b746d^ 554b746d` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid package directory report smoke passed with unchanged package manifest
  facts, manifest read/comparison facts, and cube asset row output.
- Missing package sidecar smoke passed with existing nonzero CLI behavior after
  report text and rows matching `PackageManifestReadFailed`, missing package
  manifest file facts, and `FileOpenFailed` issue row.
- Missing nested manifest smoke passed with unchanged missing nested manifest
  file facts, `manifestRead=invalid manifestReadIssues=1`, and package asset
  rows.

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

- `git merge-base --is-ancestor 554b746d HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
