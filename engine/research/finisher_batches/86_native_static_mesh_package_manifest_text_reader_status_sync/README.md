# 86 - Native Static Mesh Package Manifest Text Reader Status Sync

## Goal

Sync planning and API docs after source commit
`8cf216a0 Add native static mesh package manifest reader`.

## Integrated Surface

- Added a dependency-free, filesystem-free in-memory reader for the current
  generated package manifest text grammar.
- New document/data surface includes:
  `NativeStaticMeshExportPackageManifestDocument`,
  `NativeStaticMeshExportPackageManifestAssetRow`,
  `NativeStaticMeshExportPackageManifestReadIssueCode`,
  `NativeStaticMeshExportPackageManifestReadIssue`,
  `NativeStaticMeshExportPackageManifestReadResult`, and
  `ReadNativeStaticMeshExportPackageManifestText(...)`.
- Parser accepts only the current generated header grammar:
  `static-mesh-export-package-manifest format=... version=... manifest=... assets=N`.
- Parser accepts only generated asset rows:
  `asset=NAME filename=FILENAME`.
- Reader validates format id, version, nested manifest filename, asset count,
  basename-only filenames, nonempty names/filenames, duplicate asset names,
  duplicate filenames, unexpected lines, missing fields, extra tokens, and
  malformed rows.
- Reader does not reconstruct `NativeStaticMeshExportPolicy` or built-in ids
  from manifest rows.
- No CLI, verification, export, renderer, file IO, CMake, docs-in-source, or
  runtime behavior changes.

## Test Coverage

- Roundtrips the generated default package manifest text through the reader and
  asserts format, version, nested manifest filename, and asset row order.
- Malformed-input coverage includes empty input, bad header token, unsupported
  format, unsupported version, malformed asset count, missing header fields,
  missing asset fields, extra header tokens, extra asset tokens, malformed asset
  row, unexpected line, asset count mismatch, duplicate asset names, duplicate
  asset filenames, separator-containing asset filenames, and
  separator-containing nested manifest filename.

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No filesystem access or package sidecar file reading.
- No verification/report/export/CLI integration.
- No package directory reader, discovery/scanning/catalog/registry,
  repair/loading, exact-extra-file rejection, or policy reconstruction from
  manifest rows.
- No export write behavior changes, CLI flags, renderer changes, schema changes,
  fixture rewrites, CMake changes, or docs-in-source changes.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified focused build for
  `native_static_mesh_export_package_manifest_tests`,
  `native_static_mesh_export_package_policy_tests`,
  `native_static_mesh_export_directory_verification_tests`, and
  `native_static_mesh_export_directory_verification_report_tests`.
- Builder verified focused CTest for package manifest reader, package policy,
  directory verification, verification report, and file export tests passed 5/5.
- Builder verified `iggy_native_play` build.
- Builder verified fresh temp smoke for batch export, package manifest dump,
  verifier, and verification report.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  8cf216a0 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
