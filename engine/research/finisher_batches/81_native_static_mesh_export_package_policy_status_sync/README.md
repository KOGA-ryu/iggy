# 81 - Native Static Mesh Export Package Policy Status Sync

## Goal

Sync planning and API docs after source commit
`9e8be8f7 Add native static mesh export package policy`.

## Integrated Surface

- Added header-only, app-local, value-only
  `NativeStaticMeshExportPackagePolicy`.
- Stable constants:
  - `NativeStaticMeshExportPackageFormatId =
    "iggy:native-static-mesh-export-package"`
  - `NativeStaticMeshExportPackageFormatVersion = 1`
  - `NativeStaticMeshExportPackageManifestFilename =
    "static-mesh-export-manifest.txt"`
- `DefaultNativeStaticMeshExportPackagePolicy()` wraps
  `DefaultNativeStaticMeshExportPolicy()`.
- `ValidateNativeStaticMeshExportPackagePolicy(...)` performs deterministic
  metadata validation only.
- Validation does not access the filesystem or mutate, export, or verify package
  directories.

Validation issue codes:

- `EmptyFormatId`
- `UnsupportedFormatId`
- `UnsupportedVersion`
- `EmptyManifestFilename`
- `ManifestFilenameContainsSeparator`
- `ManifestFilenameCollidesWithAssetFilename`
- `InvalidMeshExportPolicy` with nested issue count surfaced.

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No `IggyNativePlay.cpp` changes.
- No `NativeVulkanRenderer.cpp`, renderer behavior, shader, fixture,
  runtime/product/scene/server API, native app CMake source, CLI, parser,
  package discovery/scanning, package IO, overwrite/create-dir policy, `.igmesh`
  schema, material/texture/normal/UV/animation, or gameplay changes.
- Guard scan found no file IO in the package-policy header; only the test string
  asserting filesystem-free validation.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified clean baseline at `785e72a4` before source edits.
- Builder verified full configure.
- Builder verified focused build for
  `native_static_mesh_export_package_policy_tests`.
- Builder verified focused CTest for package policy, verification report,
  verification, file export, and export policy tests passed 5/5.
- Builder verified `iggy_native_play` build.
- Builder smoke-tested export, verify, and verification report against
  `/tmp/iggy-native-package-policy-smoke-81-final.hqCzsS`.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  9e8be8f7 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
