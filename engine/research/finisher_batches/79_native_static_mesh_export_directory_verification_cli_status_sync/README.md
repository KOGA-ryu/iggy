# 79 - Native Static Mesh Export Directory Verification CLI Status Sync

## Goal

Sync planning and API docs after source commit
`aba6d838 Verify native static mesh export directories`.

## Integrated Surface

- Added read-only `VerifyNativeStaticMeshExportDirectory(policy, directory)`.
- The verifier validates the supplied export policy and requires an existing
  output directory.
- It compares `static-mesh-export-manifest.txt` exactly to
  `BuildNativeStaticMeshExportManifestText(policy).text`.
- It loads only expected policy files and checks vertex/index counts against the
  export report.
- Extra unrelated files are ignored; no directory scanning/discovery semantics
  are introduced.
- Added `iggy_native_play --verify-static-mesh-export --output-dir DIR`, which
  exits before `NativeVulkanApp`, SDL, or Vulkan startup.
- Added explicit verify conflicts with batch export, single mesh dump, export
  report, export manifest, and model-load report.

Success output:

```text
static-mesh-export-verify output=/tmp/iggy-native-verify-smoke-79 verified=3 manifest=ok
```

Missing manifest failure:

```text
iggy_native_play: static mesh export verification failed: MissingManifest output=/tmp/iggy-native-verify-missing-79/static-mesh-export-manifest.txt issues=0
```

Single-export directory failure:

```text
iggy_native_play: static mesh export verification failed: MissingManifest output=/tmp/iggy-native-verify-single-79/static-mesh-export-manifest.txt issues=0
```

Conflict examples:

```text
iggy_native_play: --verify-static-mesh-export cannot be combined with --export-static-mesh-assets
iggy_native_play: --verify-static-mesh-export cannot be combined with --dump-static-mesh-asset
iggy_native_play: --verify-static-mesh-export cannot be combined with --dump-static-mesh-export-report
iggy_native_play: --verify-static-mesh-export cannot be combined with --dump-static-mesh-export-manifest
iggy_native_play: --verify-static-mesh-export cannot be combined with --dump-static-model-load-report
```

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No directory scanning/discovery beyond expected policy files.
- No package discovery, registry/catalog/package semantics, source mutation,
  overwrite/force/create-directory/temp replacement policy, arbitrary output
  paths beyond existing `--output-dir`, or fixture rewrites/canonicalization.
- No renderer changes or `NativeVulkanRenderer.cpp`.
- No JSON/glTF/glb parser/dependency work.
- No `.igmesh` schema, material, texture, normal, UV, or animation changes.
- No gameplay/scripted/final-state changes.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified full configure.
- Builder verified focused build for
  `native_static_mesh_export_directory_verification_tests` and
  `iggy_native_play`.
- Builder verified focused CTest for directory verification, file export,
  manifest, report, and policy tests passed 5/5.
- Builder smoke-tested fresh batch export to
  `/tmp/iggy-native-verify-smoke-79`, verified the directory successfully, and
  confirmed `bean.igmesh`, `cube.igmesh`, `npc-marker.igmesh`, and
  `static-mesh-export-manifest.txt`.
- Builder verified missing-manifest and single-output-dir smokes failed nonzero
  as expected.
- Builder verified all conflict smokes failed nonzero as expected.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  aba6d838 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
