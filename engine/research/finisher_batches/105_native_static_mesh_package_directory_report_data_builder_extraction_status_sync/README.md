# 105 - Native Static Mesh Package Directory Report Data Builder Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`689988c0 Extract native static mesh package report data builder`.

## Integrated Surface

- Added no-text structured data builder:
  `BuildNativeStaticMeshExportPackageDirectoryReportData(const std::filesystem::path &directory)`.
- Moved existing structured report data collection into that helper:
  - package directory read
  - nested mesh manifest read attempt/result
  - package-vs-nested-manifest comparison
  - package sidecar facts
  - nested manifest facts
  - package-declared asset facts
- The data builder returns structured diagnostics with `text` empty.
- `BuildNativeStaticMeshExportPackageDirectoryReport(...)` remains the full
  report builder by calling the data builder and then assigning
  `report.text = BuildNativeStaticMeshExportPackageDirectoryReportText(report)`.

## Preserved Text Contract

- Full report text output and behavior are preserved exactly.
- Preserved output includes report text, rows, summary fields, row order,
  status/count/token values, and newlines.
- Full builder text is still checked against the text renderer helper.

## Verification

- `git show --stat --oneline --name-only 689988c0` showed only
  `NativeStaticMeshExportPackageDirectoryReport.hpp` and focused report tests.
- `git diff --check 689988c0^ 689988c0` passed.
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
- Focused tests compare no-text data-builder structured fields against the full
  builder for valid export, missing package sidecar, missing nested manifest,
  combined comparison mismatch, and missing declared asset.

## Boundaries Preserved

- No report text changes, rows, summary fields, row-order changes,
  status/count/token/newline changes, `readOk()` changes, CLI exit changes, or
  core `issues=` changes.
- No package directory reader status/data changes, exact verification/export/
  package acceptance/generated sidecar/write-policy changes, or nested
  mesh-manifest-only path facts.
- No `.igmesh` loading, geometry validation, policy/built-in reconstruction,
  discovery/scanning/catalog/registry, exact-extra-file rejection, renderer,
  model-slot, schema, gameplay, glTF/glb/JSON parser scope, source, CMake,
  renderer, fixture/asset, or Spark ledger changes.

## Finisher Verification

- `git merge-base --is-ancestor 689988c0 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
