# 88 - Native Static Mesh Package Manifest Verification Reader Diagnostics Status Sync

## Goal

Sync planning and API docs after source commit
`e8e0a922 Add native static mesh package manifest verification diagnostics`.

## Integrated Surface

- `ReadNativeStaticMeshExportPackageManifestFile(...)` is now wired into
  explicit-directory verification after package sidecar existence and before
  exact deterministic text comparison.
- Added verifier status `PackageManifestReadFailed`.
- Added compact diagnostic field `packageManifestReadIssueCount` on
  `NativeStaticMeshExportDirectoryVerificationResult`.
- Malformed or unreadable package sidecars now return
  `PackageManifestReadFailed`, set `problemPath` to the package sidecar, keep
  `manifestVerified=true`, keep `packageManifestVerified=false`, and do not scan
  asset geometry.
- Parse-valid but non-exact package sidecars still return
  `PackageManifestMismatch`.
- Missing package sidecar still returns `MissingPackageManifest`.
- Package sidecar is still not checked when mesh manifest verification fails
  first.

Malformed package sidecar verify failure:

```text
iggy_native_play: static mesh export verification failed: PackageManifestReadFailed output=/tmp/iggy-native-verify-reader-88-bad.8iYAKS/static-mesh-export-package-manifest.txt issues=1
```

Malformed package sidecar report summary:

```text
static-mesh-export-verification-report status=PackageManifestReadFailed output=/tmp/iggy-native-verify-reader-88-bad.8iYAKS verified=0 issues=1 manifest=ok packageManifest=invalid problem=/tmp/iggy-native-verify-reader-88-bad.8iYAKS/static-mesh-export-package-manifest.txt
```

Parse-valid exact mismatch remains `PackageManifestMismatch` with
`packageManifest=mismatch`.

Valid export still reports:

```text
static-mesh-export-verify output=<tmpdir> verified=3 manifest=ok packageManifest=ok
```

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No package directory reader/report/loading/discovery/scanning/catalog/registry
  behavior.
- No export behavior changes, overwrite/create-directory/temp replacement/
  arbitrary output path behavior, exact-extra-file rejection, repair behavior,
  or CLI flags.
- No semantic package acceptance replacing exact deterministic text
  verification.
- No reconstruction of export policy or built-in ids from package rows.
- No renderer, shader, runtime/product/scene/server API, `.igmesh` schema,
  fixture, CMake, gameplay, or docs-in-source changes.
- No next source packet opened or mixed into this docs packet.

## Verification

- Builder verified focused build for
  `native_static_mesh_export_directory_verification_tests`,
  `native_static_mesh_export_directory_verification_report_tests`,
  `native_static_mesh_export_package_manifest_tests`, and `iggy_native_play`.
- Builder verified focused CTest for directory verification, verification
  report, package manifest, package policy, and file export tests passed 5/5.
- Builder verified fresh valid export, verify, and verification report smoke.
- Builder verified malformed package sidecar smoke failed nonzero with
  `PackageManifestReadFailed` and `packageManifest=invalid`.
- Builder verified parse-valid mismatch sidecar smoke failed nonzero with
  `PackageManifestMismatch` and `packageManifest=mismatch`.
- Builder verified `git diff --check` and `git diff --cached --check` before
  source commit.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  e8e0a922 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
