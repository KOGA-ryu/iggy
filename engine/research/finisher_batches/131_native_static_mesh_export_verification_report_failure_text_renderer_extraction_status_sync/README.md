# 131 - Native Static Mesh Export Verification Report Failure Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`d28f70c9 Extract static mesh verification report failure text`.

## Integrated Surface

- Added pure helper
  `BuildNativeStaticMeshExportDirectoryVerificationReportFailureText(const NativeStaticMeshExportDirectoryVerificationReport &report)`
  beside the verification report wrapper boundary in
  `NativeStaticMeshExportDirectoryVerificationReport.hpp`.
- `PrintNativeStaticMeshExportDirectoryVerificationReport(...)` still prints
  and flushes `report.text` before failure handling.
- After report text is printed, the failure body is delegated to the helper.
- App-level `iggy_native_play:` prefix and newline behavior remain owned by the
  existing catch path.

## Preserved Failure Format

```text
static mesh export verification report failed: <Status> output=<path> issues=<N>
```

The helper emits the failure body without an embedded app prefix and without an
embedded trailing newline.

## Output Path Selection

- Uses `report.verification.problemPath` when present.
- Falls back to `report.verification.outputDirectory` otherwise.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_directory_verification_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_directory_verification_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed for missing-manifest report stdout summary and stderr compact
  failure body.
- CLI smoke passed for verified report summary output.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No report text row, report text renderer, report data builder, full builder,
  `verified()` semantic, direct verification success/failure helper,
  package-directory report behavior, verifier internal, result shape, status
  ordering, issue-count semantic, CLI parser/help/dispatch/conflict/exit
  behavior, or app-level error prefix/newline behavior changes.
- No docs/source mixing, CMake changes, renderer changes, assets/fixtures,
  sidecar generation, export write policy, schema, parser/dependency work,
  package loading/acceptance, or `NativeStaticMeshExportPackageDirectoryReport.hpp`
  changes.

## Finisher Verification

- `git merge-base --is-ancestor d28f70c9 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
