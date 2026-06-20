# 74 - Native Static Mesh Built-In Batch Export CLI Status Sync

## Goal

Sync planning/API docs after `00a8362e Add native static mesh batch export CLI`.

## Integrated Surface

- `ExportNativeStaticMeshPolicyToDirectory(...)` exports every
  `DefaultNativeStaticMeshExportPolicy()` asset to `DIR/defaultFilename`.
- The batch helper preflights output directory existence/type and all target
  filenames before writing, so common validation failures write no files.
- Batch result/entry structs carry aggregate status, output directory, exported
  count, total byte count, issue count, and per-asset file export results.
- `iggy_native_play --export-static-mesh-assets --output-dir DIR` writes every
  default built-in asset:
  - `cube.igmesh`
  - `bean.igmesh`
  - `npc-marker.igmesh`
- Batch success prints compact status only, for example
  `static-mesh-export-batch output=/tmp/iggy-native-export-batch-smoke exported=3 bytes=33879`.
- Existing single-asset stdout and single-asset output-dir behavior remains
  unchanged.

## Documented Errors

- Target exists:
  `iggy_native_play: static mesh batch export failed: TargetAlreadyExists output=/tmp/iggy-native-export-batch-smoke/cube.igmesh issues=0`
- Missing output dir flag:
  `iggy_native_play: --export-static-mesh-assets requires --output-dir`
- Single/batch conflict:
  `iggy_native_play: --export-static-mesh-assets cannot be combined with --dump-static-mesh-asset`
- Existing report/single conflict:
  `iggy_native_play: --dump-static-model-load-report cannot be combined with --dump-static-mesh-asset`
- Report/batch conflict:
  `iggy_native_play: --dump-static-model-load-report cannot be combined with --export-static-mesh-assets`

## Boundaries Preserved

- No arbitrary `--output PATH`.
- No overwrite, force, delete, rename, temp-file replacement, or in-place
  canonicalization.
- No checked-in fixture rewrite.
- No production directory creation.
- No package discovery/scanning, registry/catalog, manifest expansion, or source
  mutation.
- No glTF/glb/JSON parser or dependency.
- No `.igmesh` schema expansion.
- No material, texture, descriptor, sampler, normal, UV, animation, scene graph,
  or metadata fields.
- No renderer behavior, `NativeVulkanRenderer.cpp`, or public renderer API
  changes.
- No runtime/product/scene/server/draw-list API changes.
- No gameplay/scripted/final-state semantic changes.

## Verification

- `git merge-base --is-ancestor 00a8362e HEAD`
- `git diff --check`
- `git diff --cached --check`
- Docs-only changed file set.
