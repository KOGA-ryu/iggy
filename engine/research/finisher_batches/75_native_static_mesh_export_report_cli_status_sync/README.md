# 75 - Native Static Mesh Export Report CLI Status Sync

## Goal

Sync planning/API docs after `93ebc30c Add native static mesh export report CLI`.

## Integrated Surface

- `NativeStaticMeshExportReport.hpp` adds header-only app-local
  `BuildNativeStaticMeshExportReport(...)` over
  `DefaultNativeStaticMeshExportPolicy()` or supplied policies.
- Report entries include export name, default filename, built-in id, writable
  status, issue count, vertex count, index count, and writer byte count.
- Aggregates include asset count, writable count, total bytes, and total issues.
- `iggy_native_play --dump-static-mesh-export-report` prints the report and
  exits before `NativeVulkanApp` construction or SDL/Vulkan startup.
- The report path does not write files, validate output directories, touch
  renderer behavior, or inspect the filesystem.

## Sample Output

```text
static-mesh-export-report assets=3 writable=3 bytes=33879 issues=0
asset=cube filename=cube.igmesh status=Writable vertices=8 indices=36 bytes=523 issues=0
asset=bean filename=bean.igmesh status=Writable vertices=234 indices=1296 bytes=23882 issues=0
asset=npc-marker filename=npc-marker.igmesh status=Writable vertices=98 indices=504 bytes=9474 issues=0
```

## Documented Conflicts

- `iggy_native_play: --dump-static-mesh-export-report cannot be combined with --output-dir`
- `iggy_native_play: --dump-static-mesh-export-report cannot be combined with --export-static-mesh-assets`
- `iggy_native_play: --dump-static-mesh-export-report cannot be combined with --dump-static-mesh-asset`
- `iggy_native_play: --dump-static-mesh-export-report cannot be combined with --dump-static-model-load-report`

## Boundaries Preserved

- No file writes in the report path.
- No output-dir/path validation in the report helper.
- No arbitrary `--output PATH`.
- No overwrite, force, or create-directory policy.
- No checked-in fixture rewrites or canonicalization.
- No package discovery/scanning, registry/catalog, manifest expansion, or source
  mutation.
- No glTF/glb/JSON parser, dependency fetch, vendoring, package install, or web
  lookup.
- No `.igmesh` schema expansion.
- No material, texture, descriptor, sampler, normal, UV, animation, scene graph,
  or metadata fields.
- No renderer behavior, `NativeVulkanRenderer.cpp`, or public renderer API
  changes.
- No runtime/product/scene/server/draw-list API changes.
- No gameplay/input/scripted-control/final-state semantic changes.

## Verification

- `git merge-base --is-ancestor 93ebc30c HEAD`
- `git diff --check`
- `git diff --cached --check`
- Docs-only changed file set.
