# 80 - Native Static Mesh Export Verification Report CLI Status Sync

## Goal

Sync planning and API docs after source commit
`7f857f93 Add native static mesh verification report CLI`.

## Integrated Surface

- Added a read-only report builder around
  `VerifyNativeStaticMeshExportDirectory(policy, directory)`.
- Added `iggy_native_play --dump-static-mesh-export-verification-report
  --output-dir DIR`, which exits before `NativeVulkanApp`, SDL, or Vulkan
  startup.
- Success prints a summary plus per-asset rows for the existing default export
  policy.
- Failed verification prints the report, then returns nonzero through the
  existing compact `iggy_native_play:` error style.
- Existing `--verify-static-mesh-export` output and behavior are preserved.

Sample success output:

```text
static-mesh-export-verification-report status=Verified output=/tmp/iggy-native-verify-report-smoke-80.PBsYB2 verified=3 issues=0
asset=cube filename=cube.igmesh status=Verified vertices=8 expectedVertices=8 indices=36 expectedIndices=36 issues=0
asset=bean filename=bean.igmesh status=Verified vertices=234 expectedVertices=234 indices=1296 expectedIndices=1296 issues=0
asset=npc-marker filename=npc-marker.igmesh status=Verified vertices=98 expectedVertices=98 indices=504 expectedIndices=504 issues=0
```

Sample missing-manifest failure:

```text
static-mesh-export-verification-report status=MissingManifest output=/tmp/iggy-native-verify-report-missing-80.EG7aZh verified=0 issues=0 problem=/tmp/iggy-native-verify-report-missing-80.EG7aZh/static-mesh-export-manifest.txt
iggy_native_play: static mesh export verification report failed: MissingManifest output=/tmp/iggy-native-verify-report-missing-80.EG7aZh/static-mesh-export-manifest.txt issues=0
```

The report mode requires `--output-dir` and conflicts with:

- `--verify-static-mesh-export`
- `--export-static-mesh-assets`
- `--dump-static-mesh-asset`
- `--dump-static-mesh-export-report`
- `--dump-static-mesh-export-manifest`
- `--dump-static-model-load-report`

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No `NativeVulkanRenderer.cpp`, renderer behavior, shader behavior, asset
  fixture, runtime/product/scene/server API, docs-in-source, or native app CMake
  source registration changes.
- No writes, repair, scanning, package/catalog/parser/schema/material/texture
  behavior added.
- The new surface reports the existing verifier result only.
- No gameplay/scripted/final-state changes.
- No next research/scout source implementation mixed into this docs packet.

## Verification

- Builder verified full configure.
- Builder verified focused build for
  `native_static_mesh_export_directory_verification_report_tests` and
  `iggy_native_play`.
- Builder verified focused CTest for directory verification report, directory
  verification, file export, manifest, export report, and policy tests passed
  6/6.
- Builder verified `iggy_native_play --help` shows the new flag.
- Builder smoke-tested batch export, verification report success, missing
  manifest report failure, and the existing verifier output.
- Builder verified conflict smokes and missing `--output-dir` failed nonzero as
  expected.
- Builder verified `git diff --check` and `git diff --cached --check`.
- Finisher verification for this docs packet: `git merge-base --is-ancestor
  7f857f93 HEAD`, docs-only changed-file set, `git diff --check`,
  `git diff --cached --check`, and clean final status.
