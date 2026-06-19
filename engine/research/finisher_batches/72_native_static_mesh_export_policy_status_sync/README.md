# 72 - Native Static Mesh Export Policy Status Sync

## Goal

Sync planning/API docs after `64ce4594 Add native static mesh export policy`.

## Integrated Surface

- `NativeStaticMeshExportPolicy.hpp` adds app-local value-only export metadata.
- `NativeStaticMeshBuiltInExportId` contains `Cube`, `Bean`, and `NpcMarker`.
- `NativeStaticMeshExportAssetRef` carries `id`, `name`, and
  `defaultFilename`.
- `NativeStaticMeshExportPolicy` carries `assets`.
- `DefaultNativeStaticMeshExportPolicy()` returns stable refs:
  - `Cube`, `cube`, `cube.igmesh`
  - `Bean`, `bean`, `bean.igmesh`
  - `NpcMarker`, `npc-marker`, `npc-marker.igmesh`
- `FindNativeStaticMeshExportAsset(policy, name)` performs first-match lookup.
- `BuiltInNativeStaticMeshExportAsset(id)` maps ids to existing built-in CPU
  mesh assets.
- Existing `--dump-static-mesh-asset NAME` now resolves through the policy
  before writing raw `.igmesh`, preserving accepted names, unknown-name error,
  conflict behavior, and raw stdout output.
- Tests cover stable default order, exact names/filenames, basename-only
  filenames, lookup, missing-name null, duplicate first-match, and writer-valid
  built-in meshes.

## Boundaries Preserved

- No file writes, `--output`, fixture rewrites/canonicalization, arbitrary asset
  path input, directory scanning, package discovery, registry/catalog/manifest
  expansion, or source mutation.
- No glTF/glb/JSON parser or dependency.
- No `.igmesh` schema expansion.
- No material, texture, descriptor, sampler, normal, UV, animation, scene graph,
  or metadata fields.
- No renderer behavior, `NativeVulkanRenderer.cpp`, or public renderer API
  changes.
- No runtime/product/scene/server/draw-list API changes.
- No gameplay/scripted/final-state semantic changes.
- No docs mixed into the source commit.

## Verification

- `git merge-base --is-ancestor 64ce4594 HEAD`
- `git diff --check`
- `git diff --cached --check`
- Docs-only changed file set.
