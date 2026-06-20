# 85 - Native Static Mesh Verification Summary Sidecar Diagnostics Status Sync

## Goal

Sync planning and API docs after source commit
`0e032380 Add native static mesh verification diagnostics`.

## Integrated Surface

- Added explicit read-only sidecar diagnostics to
  `NativeStaticMeshExportDirectoryVerificationResult`:
  `manifestPath`, `packageManifestPath`, `manifestVerified`, and
  `packageManifestVerified`.
- The verifier records the mesh manifest path before mesh manifest checks and
  records the package manifest path before package sidecar checks.
- `manifestVerified` is set only after exact mesh manifest text match.
- `packageManifestVerified` is set only after exact package manifest sidecar
  text match.
- Verification status ordering and failure behavior are unchanged.
- Verification report summary rows now include compact sidecar states:
  `manifest=... packageManifest=...`.
- `--verify-static-mesh-export` success output now includes
  `packageManifest=ok` alongside existing `manifest=ok`.

Verify success output:

```text
static-mesh-export-verify output=/tmp/iggy-native-verification-diag-ok-85.6tfWQS verified=3 manifest=ok packageManifest=ok
```

Verification report success summary:

```text
static-mesh-export-verification-report status=Verified output=/tmp/iggy-native-verification-diag-ok-85.6tfWQS verified=3 issues=0 manifest=ok packageManifest=ok
```

Missing package sidecar report summary:

```text
static-mesh-export-verification-report status=MissingPackageManifest output=/tmp/iggy-native-verification-diag-missing-pkg-85.27rr0Z verified=0 issues=0 manifest=ok packageManifest=missing problem=/tmp/iggy-native-verification-diag-missing-pkg-85.27rr0Z/static-mesh-export-package-manifest.txt
```

Missing mesh manifest report summary:

```text
static-mesh-export-verification-report status=MissingManifest output=/tmp/iggy-native-verification-diag-missing-mesh-85.IQHjfa verified=0 issues=0 manifest=missing packageManifest=not-checked problem=/tmp/iggy-native-verification-diag-missing-mesh-85.IQHjfa/static-mesh-export-manifest.txt
```

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No package manifest parser/reader semantics.
- No discovery/scanning/catalog/registry behavior.
- No export write behavior changes, CLI flags, or single-export sidecars.
- No renderer, shader, runtime/product/scene/server API, `.igmesh` schema,
  fixture, CMake, or docs-in-source changes.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified focused build for
  `native_static_mesh_export_directory_verification_tests`,
  `native_static_mesh_export_directory_verification_report_tests`,
  `native_static_mesh_file_export_tests`, and `iggy_native_play`.
- Builder verified focused CTest for directory verification, verification
  report, file export, package manifest, package policy, and export policy tests
  passed 6/6.
- Builder verified fresh batch export, verify, and verification report smoke.
- Builder verified missing package sidecar verifier/report smoke failed nonzero
  with `MissingPackageManifest`.
- Builder verified missing mesh manifest report smoke failed nonzero with
  `MissingManifest` and `packageManifest=not-checked`.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  0e032380 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
