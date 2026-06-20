# 77 - Native Static Mesh Export Manifest Text Builder Status Sync

## Goal

Sync planning and API docs after source commit
`c6405f68 Add native static mesh export manifest CLI`.

## Integrated Surface

- Added app-local header-only `NativeStaticMeshExportManifest.hpp`.
- Added `BuildNativeStaticMeshExportManifestText(...)`.
- The manifest builder validates the supplied `NativeStaticMeshExportPolicy`,
  then uses `BuildNativeStaticMeshExportReport(...)` for stable asset counts and
  writer byte counts.
- Added `NativeStaticMeshExportManifestResult` with statuses `Built`,
  `InvalidPolicy`, and `WriterFailed`.
- Added `iggy_native_play --dump-static-mesh-export-manifest`, which prints
  deterministic manifest text to stdout and exits before `NativeVulkanApp`,
  SDL, or Vulkan startup.
- Added conflicts with `--output-dir`, `--export-static-mesh-assets`,
  `--dump-static-mesh-export-report`, `--dump-static-mesh-asset`, and
  `--dump-static-model-load-report`.
- Existing valid export report and batch export output remain unchanged.

Current manifest output shape:

```text
static-mesh-export-manifest version=1 assets=3 bytes=33879
asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523
asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882
asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474
```

## Boundaries Preserved

- No source, test, CMake, asset, shader, or runtime changes in this docs packet.
- No sidecar file writes or package/export manifest files on disk.
- No overwrite, create-directory, temp-file, arbitrary output path, or checked-in
  fixture canonicalization/rewrite policy.
- No package discovery, directory scanning, registry/catalog expansion, source
  mutation, material/texture/schema changes, JSON/glTF/glb parser/dependency
  work, or renderer behavior changes.
- No `NativeVulkanRenderer.cpp` changes, native app CMake source registration
  changes, gameplay/scripted/final-state changes, or docs mixed into source.

## Verification

- Planner verified `git diff --name-only 65809c2b..c6405f68` was exactly the
  source/test/CMake/app files for the manifest packet.
- Planner verified `git diff --check 65809c2b..c6405f68`.
- Planner verified focused build for `native_static_mesh_export_manifest_tests`
  and `iggy_native_play`.
- Planner verified focused CTest for manifest, report, policy, and file export
  tests.
- Planner smoke-tested `iggy_native_play --dump-static-mesh-export-manifest`,
  retained export report output, retained batch export output, and expected
  conflict failures.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  c6405f68 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
