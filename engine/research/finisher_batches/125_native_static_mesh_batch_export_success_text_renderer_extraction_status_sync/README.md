# 125 - Native Static Mesh Batch Export Success Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`cc3b9be7 Extract static mesh batch export success text`.

## Integrated Surface

- Added pure header helper
  `BuildNativeStaticMeshFileExportBatchSuccessText(const NativeStaticMeshFileExportBatchResult &result)`
  in `NativeStaticMeshFileExport.hpp`.
- `PrintNativeStaticMeshAssetBatchExport(...)` now delegates to the helper after
  the existing `Exported` status check.
- The helper serializes only successful batch export compact stdout.

## Preserved Success Format

```text
static-mesh-export-batch output=<dir> exported=<N> bytes=<N> manifest=<path> manifestBytes=<N> packageManifest=<path> packageManifestBytes=<N>
```

The emitted string is newline-terminated.

## Preserved Behavior Contract

- Batch failure path/output selection and compact failure text are unchanged.
- Export status assignment, preflight/write order, no-overwrite/no-create-
  directory behavior, sidecar filenames/content, byte count semantics, and CLI
  parser/help/dispatch/conflict/exit behavior are unchanged.
- Exact helper coverage uses a real successful default batch export result.

## Verification

- Baseline was clean at `1469f067` before source edits.
- `git show --stat --oneline --name-only cc3b9be7` showed only
  `NativeStaticMeshFileExport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_file_export_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_file_export_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_file_export_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Success smoke matched the exact batch success line with `exported=3`,
  `bytes=33879`, `manifestBytes=272`, and `packageManifestBytes=250`.
- File existence checks passed for `cube.igmesh`, `bean.igmesh`,
  `npc-marker.igmesh`, `static-mesh-export-manifest.txt`, and
  `static-mesh-export-package-manifest.txt`.
- Collision smoke preserved existing failure behavior:
  `iggy_native_play: static mesh batch export failed: TargetAlreadyExists output=/tmp/iggy-native-batch-success-renderer-125/static-mesh-export-manifest.txt issues=0`.

## Boundaries Preserved

- No batch failure text extraction, batch failure output/issue selection, export
  status assignment, preflight/write order, no-overwrite/no-create-directory,
  sidecar filename/content, byte count semantic, CLI parser/help/dispatch/
  conflict/exit, static model, package directory report, exact verification,
  generated sidecar beyond existing export behavior, package loading/discovery/
  acceptance, renderer/model-slot behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, glTF/glb/JSON
  parser/dependency work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor cc3b9be7 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
