# 78 - Native Static Mesh Batch Manifest Sidecar Export Status Sync

## Goal

Sync planning and API docs after source commit
`36b27365 Write native static mesh batch manifest sidecar`.

## Integrated Surface

- Batch export now writes policy mesh files plus one manifest sidecar.
- The sidecar filename is exactly `static-mesh-export-manifest.txt`.
- The sidecar content is exactly
  `BuildNativeStaticMeshExportManifestText(policy).text`.
- Batch export preflights the sidecar target before mesh writes and applies the
  same no-overwrite `TargetAlreadyExists` policy.
- `NativeStaticMeshFileExportBatchResult` now reports `manifestOutputPath` and
  `manifestByteCount`.
- CLI batch success output includes stable `manifest=...` and
  `manifestBytes=...` fields.
- Single-asset stdout and single-asset output-dir export remain unchanged.
- Single output-dir export writes no sidecar.

Sample success output:

```text
static-mesh-export-batch output=/tmp/iggy-native-sidecar-smoke-78 exported=3 bytes=33879 manifest=/tmp/iggy-native-sidecar-smoke-78/static-mesh-export-manifest.txt manifestBytes=272
```

Manifest sidecar content shape:

```text
static-mesh-export-manifest version=1 assets=3 bytes=33879
asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523
asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882
asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474
```

Sample sidecar-target failure:

```text
iggy_native_play: static mesh batch export failed: TargetAlreadyExists output=/tmp/iggy-native-sidecar-existing-78/static-mesh-export-manifest.txt issues=0
```

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No single-export sidecar behavior.
- No package discovery/scanning, registry/catalog expansion, package semantics,
  source mutation, overwrite/force/create-directory/temp replacement, checked-
  in fixture rewrites/canonicalization, renderer behavior changes,
  `NativeVulkanRenderer.cpp`, JSON/glTF/glb parser/dependency work, `.igmesh`
  schema expansion, materials/textures/normals/UVs/animation/schema work,
  gameplay/scripted/final-state changes, or next research/scout source
  implementation.

## Verification

- Builder verified focused build:
  `native_static_mesh_file_export_tests`,
  `native_static_mesh_export_manifest_tests`, and `iggy_native_play`.
- Builder verified focused CTest for file export, manifest, report, and policy
  tests passed 4/4.
- Builder smoke-tested fresh batch export at
  `/tmp/iggy-native-sidecar-smoke-78` and verified `cube.igmesh`,
  `bean.igmesh`, `npc-marker.igmesh`, and
  `static-mesh-export-manifest.txt` exist.
- Builder verified sidecar content matched the manifest dump shape.
- Builder verified `iggy_native_play --dump-static-mesh-export-manifest`
  printed the same manifest text.
- Builder verified single export wrote only `cube.igmesh` and no sidecar.
- Builder verified re-running batch into the same directory failed on
  `static-mesh-export-manifest.txt`.
- Builder verified a pre-existing sidecar target failed before mesh writes and
  preserved the existing sidecar content.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  36b27365 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
