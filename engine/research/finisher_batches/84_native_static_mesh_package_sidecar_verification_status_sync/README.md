# 84 - Native Static Mesh Package Sidecar Verification Status Sync

## Goal

Sync planning and API docs after source commit
`358b6512 Verify native static mesh package sidecar`.

## Integrated Surface

- `VerifyNativeStaticMeshExportDirectory(...)` now requires the package sidecar
  `static-mesh-export-package-manifest.txt`.
- Verification still checks the output directory and exact mesh manifest text
  first.
- Package sidecar verification happens after mesh manifest equality and before
  asset geometry checks.
- Expected package sidecar text is built read-only with
  `BuildNativeStaticMeshExportPackageManifestText(...)` from a package policy
  composed from the verification mesh policy.
- Added package-specific statuses: `MissingPackageManifest`,
  `PackageManifestMismatch`, and `PackageManifestBuildFailed`.
- Status-to-text mappings were updated for the verification report helper and
  CLI error text.
- No new CLI flag was added. Existing `--verify-static-mesh-export` and
  `--dump-static-mesh-export-verification-report` now fail/report missing or
  mismatched package sidecars.

Missing package sidecar verifier failure:

```text
iggy_native_play: static mesh export verification failed: MissingPackageManifest output=/tmp/iggy-native-package-verify-missing-84.RIkcIc/static-mesh-export-package-manifest.txt issues=0
```

Missing package sidecar report failure:

```text
static-mesh-export-verification-report status=MissingPackageManifest output=/tmp/iggy-native-package-verify-missing-84.RIkcIc verified=0 issues=0 problem=/tmp/iggy-native-package-verify-missing-84.RIkcIc/static-mesh-export-package-manifest.txt
iggy_native_play: static mesh export verification report failed: MissingPackageManifest output=/tmp/iggy-native-package-verify-missing-84.RIkcIc/static-mesh-export-package-manifest.txt issues=0
```

Mismatched package sidecar verifier failure:

```text
iggy_native_play: static mesh export verification failed: PackageManifestMismatch output=/tmp/iggy-native-package-verify-mismatch-84.vVqdH6/static-mesh-export-package-manifest.txt issues=1
```

Mismatched package sidecar report failure:

```text
static-mesh-export-verification-report status=PackageManifestMismatch output=/tmp/iggy-native-package-verify-mismatch-84.vVqdH6 verified=0 issues=1 problem=/tmp/iggy-native-package-verify-mismatch-84.vVqdH6/static-mesh-export-package-manifest.txt
iggy_native_play: static mesh export verification report failed: PackageManifestMismatch output=/tmp/iggy-native-package-verify-mismatch-84.vVqdH6/static-mesh-export-package-manifest.txt issues=1
```

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- Verification remains read-only and explicit-directory only.
- No package parser/reader syntax, semantic parsing of package manifest rows,
  discovery/scanning/catalog/registry, exact-extra-file rejection, repair/
  loading, export write behavior change, overwrite/create-dir policy, new CLI
  flags, single-export sidecars, fixture rewrites, renderer changes,
  `NativeVulkanRenderer.cpp`, `.igmesh` schema changes, glTF/glb/JSON
  dependencies, docs-in-source, native app CMake source registration,
  runtime/product/scene/server API changes, or gameplay/scripted/final-state
  changes.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified clean baseline at `94cc3e55` before source edits.
- Builder verified focused build for
  `native_static_mesh_export_directory_verification_tests`,
  `native_static_mesh_export_directory_verification_report_tests`,
  `native_static_mesh_file_export_tests`,
  `native_static_mesh_export_package_manifest_tests`, and `iggy_native_play`.
- Builder verified focused CTest for directory verification, verification
  report, file export, package manifest, package policy, and export policy tests
  passed 6/6.
- Builder verified valid export smoke at
  `/tmp/iggy-native-package-verify-ok-84.dskg9z`: batch export passed,
  `--verify-static-mesh-export` passed with `verified=3 manifest=ok`, and
  `--dump-static-mesh-export-verification-report` passed with all three assets
  verified.
- Builder verified missing package sidecar smoke at
  `/tmp/iggy-native-package-verify-missing-84.RIkcIc`: removing only
  `static-mesh-export-package-manifest.txt` made verifier and report fail
  nonzero with `MissingPackageManifest`.
- Builder verified mismatched package sidecar smoke at
  `/tmp/iggy-native-package-verify-mismatch-84.vVqdH6`: replacing only
  `static-mesh-export-package-manifest.txt` with `mismatch\n` made verifier and
  report fail nonzero with `PackageManifestMismatch` and `issues=1`.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  358b6512 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
