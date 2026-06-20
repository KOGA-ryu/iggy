# 102 - Native Static Mesh Package Directory Structured Manifest Diagnostics Status Sync

## Goal

Sync planning and API docs after source commit
`c7839fed Expose native static mesh package report diagnostics`.

## Integrated Surface

- Extended `NativeStaticMeshExportPackageDirectoryReport` with structured
  diagnostics:
  - `manifestReadAttempted`
  - `manifestRead`
  - `manifestComparison`
- The report builder now populates those fields instead of keeping nested
  manifest read and comparison diagnostics as local-only variables.
- `manifestReadAttempted` is true only when package directory read succeeds and
  a projected nested manifest path is available.
- `manifestRead` stores
  `ReadNativeStaticMeshExportManifestFile(report.read.manifestPath)` when
  attempted.
- `manifestComparison` is populated only when
  `manifestReadAttempted && manifestRead.read()`; otherwise it remains
  default/empty.

## Preserved Behavior

- Report text output is preserved exactly: summary fields, row order/content,
  missing/malformed nested manifest behavior, package read failure behavior,
  `readOk()`, CLI exit behavior, `issues=`, verification behavior, package
  acceptance semantics, and export behavior are unchanged.
- Valid CLI smoke still contains
  `manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0`, no
  `manifestComparison` rows, and existing `manifestAsset=` plus package `asset=`
  rows.
- Combined mismatch output remains
  `manifestMatches=1 manifestMismatches=3 manifestComparisonIssues=3` with row
  order `FilenameMismatch`, `MissingFromManifest`, `MissingFromPackage`.
- Missing or malformed nested manifest output remains diagnostic-only:
  `manifestRead=invalid manifestReadIssues=1`, no `manifestAsset=`, no
  `manifestComparison`, and no comparison summary fields.

## Verification

- `git show --stat --oneline --name-only c7839fed` showed only
  `NativeStaticMeshExportPackageDirectoryReport.hpp` and focused report tests.
- `git diff --check c7839fed^ c7839fed` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Focused assertions cover valid export, combined mismatch, missing/malformed
  nested manifest, and missing/malformed package sidecar paths.

## Boundaries Preserved

- No report text changes, new rows, summary fields, statuses, `readOk()` changes,
  CLI exit changes, or core `issues=` changes.
- No package directory reader/status changes, manifest reader changes,
  verification/export behavior changes, package acceptance/semantic verification,
  scanning/discovery, `.igmesh` loading, geometry validation, policy/built-in
  reconstruction, renderer changes, fixtures/generated assets, or glTF/glb/JSON
  parser work.

## Finisher Verification

- `git merge-base --is-ancestor c7839fed HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
