# 73 - Native Static Mesh Output Directory Export CLI Status Sync

## Goal

Sync planning/API docs after `7c4e586b Add native static mesh output directory export`.

## Integrated Surface

- `NativeStaticMeshFileExport.hpp` adds header-only app-local
  `ExportNativeStaticMeshAssetToDirectory(...)`.
- The helper validates policy lookup, existing output directory, directory type,
  target nonexistence, writer success, file open, and write success.
- Expected validation failures return status/result data and do not print or
  throw.
- `iggy_native_play --dump-static-mesh-asset NAME` now accepts optional
  `--output-dir DIR`.
- Without `--output-dir`, existing raw `.igmesh` stdout behavior is unchanged.
- With `--output-dir`, the CLI writes to `DIR/defaultFilename` from
  `NativeStaticMeshExportPolicy` and prints a compact status line such as
  `static-mesh-export name=cube output=/tmp/iggy-native-export-smoke/cube.igmesh bytes=523`.
- `--output-dir` requires `--dump-static-mesh-asset`.
- The existing `--dump-static-model-load-report` conflict remains.
- Production helper does not create directories and does not overwrite.

## Documented Errors

- Target exists:
  `iggy_native_play: static mesh export failed: TargetAlreadyExists output=/tmp/iggy-native-export-smoke/cube.igmesh issues=0`
- Unknown asset:
  `iggy_native_play: unknown static mesh asset: nope`
- Output dir without dump:
  `iggy_native_play: --output-dir requires --dump-static-mesh-asset`
- Static model report conflict:
  `iggy_native_play: --dump-static-model-load-report cannot be combined with --dump-static-mesh-asset`

## Boundaries Preserved

- No arbitrary `--output PATH`.
- No overwrite, `--force`, delete, rename, temp-file replacement, or in-place
  canonicalization.
- No writes to checked-in fixtures by default and no fixture rewrites.
- No production directory creation.
- No package discovery, directory scanning, registry/catalog, manifest
  expansion, or source mutation.
- No glTF/glb/JSON parser, dependency fetch, vendoring, package install, or web
  lookup.
- No `.igmesh` schema expansion.
- No materials, textures, descriptors, samplers, normals, UVs, skins, animation,
  transforms, scene graph, or metadata fields.
- No renderer behavior change, `NativeVulkanRenderer.cpp`, or public renderer
  API change.
- No runtime/product/scene/server/draw-list API changes.
- No gameplay/input/scripted-control/final-state semantic changes.

## Verification

- `git merge-base --is-ancestor 7c4e586b HEAD`
- `git diff --check`
- `git diff --cached --check`
- Docs-only changed file set.
