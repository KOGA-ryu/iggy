# 132 - Native Static Mesh Export Package Directory Report Failure Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`d4d92ab2 Extract static mesh package report failure text`.

## Integrated Surface

- Added pure helper
  `BuildNativeStaticMeshExportPackageDirectoryReportFailureText(const NativeStaticMeshExportPackageDirectoryReport &report)`
  beside the package-directory report wrapper boundary in
  `NativeStaticMeshExportPackageDirectoryReport.hpp`.
- `PrintNativeStaticMeshExportPackageDirectoryReport(...)` still prints and
  flushes `report.text` before failure handling.
- After report text is printed, the non-`readOk()` failure body is delegated to
  the helper.
- App-level `iggy_native_play:` prefix and newline behavior remain owned by the
  existing catch path.

## Preserved Failure Format

```text
static mesh export package directory report failed: <Status> output=<directory> issues=<N>
```

The helper emits the failure body without an embedded app prefix and without an
embedded trailing newline.

## Output Selection

- Uses `NativeStaticMeshExportPackageDirectoryReadStatusText(report.read.status)`.
- Uses `report.read.directory.string()` only.
- Uses `report.read.issueCount`.
- Does not substitute a problem path or sidecar path.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed for missing-directory package report stdout summary and
  stderr compact failure body.
- CLI smoke passed for successful package report summary output.
- CLI smoke passed for missing package sidecar compact failure body using the
  package directory output and issue count.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No package-directory report text row, report text renderer, report data
  builder, full builder, reader behavior, read result shape, read status
  semantic, `readOk()` semantic,
  `NativeStaticMeshExportPackageDirectoryReadStatusText(...)`, verification/
  report code, exact verification semantic, CLI parser/help/dispatch/conflict/
  exit behavior, app-level error prefix/newline behavior, or built-in asset dump
  writer changes.
- No docs/source mixing, CMake changes, renderer changes, assets/fixtures,
  sidecar generation, export write policy, schema, parser/dependency work,
  package loading/discovery/acceptance, extra-file policy, or static model
  changes.

## Finisher Verification

- `git merge-base --is-ancestor d4d92ab2 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
