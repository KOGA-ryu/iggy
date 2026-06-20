# 107 - Native Static Mesh Export Verification Report Data Builder Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`9fa597bb Extract native static mesh verification report data builder`.

## Integrated Surface

- Added no-text structured verification report data builder:
  `BuildNativeStaticMeshExportDirectoryVerificationReportData(const NativeStaticMeshExportPolicy &policy, const std::filesystem::path &directory)`.
- The data builder constructs `NativeStaticMeshExportDirectoryVerificationReport`,
  assigns `report.verification = VerifyNativeStaticMeshExportDirectory(policy, directory)`,
  and returns with `report.text` empty.
- `BuildNativeStaticMeshExportDirectoryVerificationReport(policy, directory)`
  remains the full builder: it calls the data builder, assigns
  `report.text = BuildNativeStaticMeshExportDirectoryVerificationReportText(report)`,
  and returns the same full report behavior.

## Preserved Behavior Contract

- Report text and verification semantics are preserved.
- Preserved scope includes status, verified count, issue count, problem path,
  sidecar paths/states, package read issue count/rows, entries, entry
  statuses/counts, CLI behavior, and failure behavior.
- Existing text-renderer parity checks remain intact.

## Verification

- `git show --stat --oneline --name-only 9fa597bb` showed only
  `NativeStaticMeshExportDirectoryVerificationReport.hpp` and focused
  verification report tests.
- `git diff --check 9fa597bb^ 9fa597bb` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_directory_verification_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_directory_verification_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid verification report smoke passed with `status=Verified`,
  `manifest=ok packageManifest=ok`, and verified asset rows.
- Missing package sidecar smoke preserved nonzero CLI behavior and report rows
  for `MissingPackageManifest`, `manifest=ok packageManifest=missing`, and the
  package manifest problem path.
- Malformed package sidecar smoke preserved nonzero CLI behavior and report rows
  for `PackageManifestReadFailed`, `packageManifest=invalid`, and
  `packageManifestReadIssue code=MalformedHeader`.
- Focused tests compare data-builder structured fields against the full builder
  and assert `dataReport.text.empty()` for valid export, missing mesh manifest,
  missing package manifest, malformed package sidecar, missing asset, and
  geometry mismatch.

## Boundaries Preserved

- No verifier internals changes.
- No verification status/order/issue-count changes, report text changes, CLI
  behavior changes, exact sidecar matching changes, generated sidecar/export
  behavior changes, package acceptance semantics, package directory report/reader
  changes, package loading/discovery, `.igmesh` loading beyond existing verifier
  behavior, renderer/model-slot changes, CMake/assets/fixtures, or glTF/glb/JSON
  parser/dependency work.

## Finisher Verification

- `git merge-base --is-ancestor 9fa597bb HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
