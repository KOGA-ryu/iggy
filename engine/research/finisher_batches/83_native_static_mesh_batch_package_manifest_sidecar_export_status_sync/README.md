# 83 - Native Static Mesh Batch Package Manifest Sidecar Export Status Sync

## Goal

Sync planning and API docs after source commit
`a1801e2d Write native static mesh package sidecar`.

## Integrated Surface

- Batch export now writes an additional package manifest sidecar:
  `static-mesh-export-package-manifest.txt`.
- Package sidecar content is exactly
  `BuildNativeStaticMeshExportPackageManifestText(packagePolicy).text`, with
  `packagePolicy.meshPolicy` composed from the mesh policy passed to batch
  export.
- `NativeStaticMeshExportPackagePolicy::manifestFilename` remains unchanged and
  continues to name the nested mesh export manifest
  `static-mesh-export-manifest.txt`.
- Batch export preflights the package sidecar target before asset and
  mesh-manifest writes.
- A pre-existing package sidecar returns `TargetAlreadyExists`, preserves that
  file, and writes no meshes or mesh manifest.
- Single export remains unchanged and writes only the selected `.igmesh` with no
  sidecars.
- `--dump-static-mesh-export-package-manifest` remains no-write and unchanged.
- Existing verify and verification-report paths remain unchanged and continue
  ignoring the extra package sidecar.

Batch success output now includes package sidecar fields:

```text
static-mesh-export-batch output=/tmp/iggy-native-package-sidecar-smoke-83.OV3EYV exported=3 bytes=33879 manifest=/tmp/iggy-native-package-sidecar-smoke-83.OV3EYV/static-mesh-export-manifest.txt manifestBytes=272 packageManifest=/tmp/iggy-native-package-sidecar-smoke-83.OV3EYV/static-mesh-export-package-manifest.txt packageManifestBytes=250
```

Package sidecar sample:

```text
static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=3
asset=cube filename=cube.igmesh
asset=bean filename=bean.igmesh
asset=npc-marker filename=npc-marker.igmesh
```

Package sidecar collision sample:

```text
iggy_native_play: static mesh batch export failed: TargetAlreadyExists output=/tmp/iggy-native-package-sidecar-collision-83.aanDWz/static-mesh-export-package-manifest.txt issues=0
```

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No package parser/reader, package discovery/scanning/catalog/registry,
  package verification integration, exact-extra-file validation,
  overwrite/force/create-dir/temp replacement/arbitrary output path, or
  single-export sidecar behavior.
- No fixture rewrites, renderer behavior changes, `NativeVulkanRenderer.cpp`,
  model-slot binding, gameplay/scripted/final-state changes, shader edits,
  native app source registration churn, `.igmesh` schema/material/texture/
  normal/UV/animation expansion, glTF/glb/JSON parser, third-party dependency,
  or docs-in-source changes.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified clean baseline at `76f2478e` before source edits.
- Builder verified focused build for
  `native_static_mesh_file_export_tests`,
  `native_static_mesh_export_package_manifest_tests`,
  `native_static_mesh_export_package_policy_tests`, and `iggy_native_play`.
- Builder verified focused CTest for file export, package manifest, package
  policy, directory verification report, and export policy tests passed 5/5.
- Builder verified fresh export smoke at
  `/tmp/iggy-native-package-sidecar-smoke-83.OV3EYV`, producing `cube.igmesh`,
  `bean.igmesh`, `npc-marker.igmesh`,
  `static-mesh-export-manifest.txt`, and
  `static-mesh-export-package-manifest.txt`.
- Builder verified the package sidecar matched
  `iggy_native_play --dump-static-mesh-export-package-manifest`.
- Builder verified `iggy_native_play --verify-static-mesh-export` against the
  export directory passed with `verified=3 manifest=ok`.
- Builder verified `iggy_native_play
  --dump-static-mesh-export-verification-report` against the export directory
  passed with all three assets verified.
- Builder verified pre-existing package sidecar collision failed nonzero with
  `TargetAlreadyExists`, left the directory containing only the original package
  sidecar, and preserved its `existing` content.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  a1801e2d HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
