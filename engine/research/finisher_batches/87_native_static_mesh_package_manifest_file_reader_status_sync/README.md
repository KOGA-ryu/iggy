# 87 - Native Static Mesh Package Manifest File Reader Status Sync

## Goal

Sync planning and API docs after source commit
`6bf5c2fb Add native static mesh package manifest file reader`.

## Integrated Surface

- Added explicit-file package manifest reader API:
  `ReadNativeStaticMeshExportPackageManifestFile(const std::filesystem::path &path)`.
- The wrapper opens exactly the supplied path in binary mode, reads the full
  file, and delegates to `ReadNativeStaticMeshExportPackageManifestText(...)`.
- Added `NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed`.
- On file open failure, the wrapper returns one issue with line `0` and token
  set to `path.string()`.
- Existing text-reader grammar and malformed-input behavior are unchanged.
- No directory inference, companion mesh validation, CLI integration,
  verification/report integration, export behavior, renderer behavior, or
  docs-in-source changes.

## Test Coverage

- Generated default package manifest text written to a temp file reads
  successfully and preserves parsed format, version, nested manifest filename,
  and asset rows.
- Missing file reports exactly one `FileOpenFailed` issue with line `0` and
  supplied path token.
- Malformed readable file propagates text-reader issue codes and does not report
  file-open failure.

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No package directory reader, directory summary/report, loading,
  discovery/scanning/catalog/registry, or exact-extra-file rejection.
- No integration into verification/report/export/CLI.
- No policy reconstruction or built-in id reconstruction from manifest rows.
- No export write behavior changes, CLI flags, renderer changes, schema changes,
  fixture rewrites, CMake changes, or docs-in-source changes.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified focused build for
  `native_static_mesh_export_package_manifest_tests` and
  `native_static_mesh_export_package_policy_tests`.
- Builder verified focused CTest for package manifest, package policy,
  directory verification, verification report, and file export tests passed 5/5.
- Builder verified `iggy_native_play` build.
- Builder verified fresh temp smoke for batch export, package manifest dump,
  verifier, and verification report.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  6bf5c2fb HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
