# 89 - Native Static Mesh Verification Package Read Issue Rows Status Sync

## Goal

Sync planning and API docs after source commit
`693fca5c Add native static mesh package read issue rows`.

## Integrated Surface

- `NativeStaticMeshExportDirectoryVerificationResult` now carries structured
  package manifest read issues in `packageManifestReadIssues` for
  `PackageManifestReadFailed` results.
- `VerifyNativeStaticMeshExportDirectory(...)` copies
  `readPackageManifest.issues`, sets `packageManifestReadIssueCount` from the
  vector size, and keeps `issueCount` equal to that count.
- Existing `PackageManifestReadFailed`, `packageManifest=invalid`,
  `manifestVerified=true`, `packageManifestVerified=false`, empty asset entries,
  and verification order are preserved.
- Parse-valid but exact-mismatched package sidecars remain
  `PackageManifestMismatch` with no read issue rows.
- Missing package sidecars remain `MissingPackageManifest` with no read issue
  rows.
- Mesh manifest failures still prevent package sidecar read rows.
- `BuildNativeStaticMeshExportDirectoryVerificationReport(...)` now emits
  deterministic package manifest reader issue rows after the summary and before
  asset rows when `packageManifestReadIssues` is non-empty.

Report row shape:

```text
packageManifestReadIssue code=MalformedHeader line=1 token=static-mesh-export-package
```

## Verification

- Builder verified focused build for
  `native_static_mesh_export_directory_verification_tests`,
  `native_static_mesh_export_directory_verification_report_tests`,
  `native_static_mesh_export_package_manifest_tests`, and `iggy_native_play`.
- Builder verified focused CTest for directory verification, verification
  report, package manifest, package policy, and file export tests passed 5/5.
- Builder verified fresh valid export, verify, and verification report smoke
  passed with no read issue rows.
- Builder verified malformed package sidecar report smoke failed nonzero as
  expected and emitted both the `PackageManifestReadFailed` summary and the
  `packageManifestReadIssue` row.
- Builder verified malformed package sidecar verify smoke failed nonzero with
  unchanged compact `PackageManifestReadFailed ... issues=1` output.
- Builder verified parse-valid exact mismatch smoke remained
  `PackageManifestMismatch` / `packageManifest=mismatch` and emitted no read
  issue rows.
- Builder verified `git diff --check` and `git diff --cached --check` before
  source commit.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  693fca5c HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No new CLI flag, ParseArgs, or usage text changes.
- No package directory reader/report/loading/discovery/scanning/catalog/
  registry/source mutation/repair/exact-extra-file rejection.
- No export behavior changes, overwrite/create-directory/temp replacement/
  arbitrary output path policy.
- No semantic package acceptance replacing exact deterministic text
  verification.
- No reconstruction of export policy or built-in ids from package rows.
- No renderer, runtime/product/scene/server API, gameplay, `.igmesh` schema,
  fixture, dependency, CMake, or docs-in-source changes.
- No next source packet opened or mixed into this docs packet.
