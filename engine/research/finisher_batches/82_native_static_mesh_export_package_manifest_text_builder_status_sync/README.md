# 82 - Native Static Mesh Export Package Manifest Text Builder Status Sync

## Goal

Sync planning and API docs after source commit
`d8fcec89 Add native static mesh export package manifest`.

## Integrated Surface

- Added header-only, app-local `NativeStaticMeshExportPackageManifest.hpp`.
- Added `NativeStaticMeshExportPackageManifestStatus { Built, InvalidPolicy }`.
- Added `NativeStaticMeshExportPackageManifestResult { status, text,
  issueCount, written() }`.
- Added `BuildNativeStaticMeshExportPackageManifestText(const
  NativeStaticMeshExportPackagePolicy &)`.
- The builder validates package policy first with
  `ValidateNativeStaticMeshExportPackagePolicy(...)`.
- Invalid policy returns `InvalidPolicy`, issue count, and no text.
- The builder is deterministic and no-write/no-filesystem.
- Added `iggy_native_play --dump-static-mesh-export-package-manifest`, which
  exits before `NativeVulkanApp`, SDL, or Vulkan startup.

Manifest output sample:

```text
static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=3
asset=cube filename=cube.igmesh
asset=bean filename=bean.igmesh
asset=npc-marker filename=npc-marker.igmesh
```

The CLI conflicts with:

- `--output-dir`
- `--export-static-mesh-assets`
- `--verify-static-mesh-export`
- `--dump-static-mesh-export-verification-report`
- `--dump-static-mesh-export-manifest`
- `--dump-static-mesh-export-report`
- `--dump-static-mesh-asset`
- `--dump-static-model-load-report`

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No package file IO, reader/parser syntax, package verification integration,
  package discovery/scanning/catalog/registry, write/repair behavior,
  overwrite/create-dir policy, renderer changes, model-slot binding, glTF/JSON
  dependencies, `.igmesh` schema changes, fixture rewrites, docs-in-source, or
  gameplay/scripted/final-state changes.
- No `NativeVulkanRenderer.cpp`, shader, checked-in fixture,
  runtime/product/scene/server API, renderer loading, or native app CMake source
  registration changes.
- Guard scan found only existing app-shell SDL/Vulkan/filesystem references in
  `IggyNativePlay.cpp` and test assertion text; the package manifest builder
  itself has no filesystem/file IO.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified clean baseline at `ded572d2` before source edits.
- Builder verified full configure.
- Builder verified focused build for
  `native_static_mesh_export_package_manifest_tests` and `iggy_native_play`.
- Builder verified focused CTest for package manifest, package policy,
  verification report, file export, and export policy tests passed 5/5.
- Builder verified `iggy_native_play --dump-static-mesh-export-package-manifest`
  printed the sample output.
- Builder verified conflict smokes for package manifest with all conflicting
  modes failed nonzero.
- Builder verified existing export loop smoke with export, verify, and
  verification report against
  `/tmp/iggy-native-package-manifest-smoke-82.gYxqxm`.
- Builder verified `iggy_native_play --help` shows the new flag.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  d8fcec89 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
