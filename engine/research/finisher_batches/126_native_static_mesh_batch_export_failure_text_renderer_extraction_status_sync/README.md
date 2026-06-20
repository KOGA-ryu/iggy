# 126 - Native Static Mesh Batch Export Failure Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`c622c9ad Extract static mesh batch export failure text`.

## Integrated Surface

- Added pure header helper
  `BuildNativeStaticMeshFileExportBatchFailureText(const NativeStaticMeshFileExportBatchResult &result)`
  in `NativeStaticMeshFileExport.hpp`.
- `PrintNativeStaticMeshAssetBatchExport(...)` now delegates the non-`Exported`
  branch through
  `std::runtime_error(BuildNativeStaticMeshFileExportBatchFailureText(result))`.
- Batch success helper/output, single-export success/failure helpers, export
  behavior, and the app-level `iggy_native_play:` catch prefix are unchanged.

## Preserved Failure Format

```text
static mesh batch export failed: <Status> output=<path> issues=<N>
```

The helper intentionally excludes the `iggy_native_play:` prefix and trailing
newline. The existing exception/catch path still supplies both.

## Output Selection Rules

- Default output is `result.outputDirectory`.
- Manifest sidecar collision selects `manifestOutputPath`.
- Package manifest collision selects `packageManifestOutputPath`.
- The first matching non-`Exported` entry selects that entry output path when
  non-empty and adds that entry issue count.

## Verification

- Baseline was clean at `6455f645` before source edits.
- `git show --stat --oneline --name-only c622c9ad` showed only
  `NativeStaticMeshFileExport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_file_export_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_file_export_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_file_export_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Manifest collision smoke passed with exact failure text for
  `static-mesh-export-manifest.txt`.
- Package manifest collision smoke passed with exact failure text for
  `static-mesh-export-package-manifest.txt`.
- Asset collision smoke passed with exact failure text for `bean.igmesh`.
- Success smoke preserved the exact batch success line with `exported=3`,
  `bytes=33879`, `manifestBytes=272`, and `packageManifestBytes=250`.

## Boundaries Preserved

- No batch success text, single-export text, export data shape, export status
  assignment, preflight/write order, no-overwrite/no-create-directory, sidecar
  filename/content, byte count semantic, CLI parser/help/dispatch/conflict/exit,
  static model, package directory diagnostics, exact verification, generated
  sidecar beyond existing export behavior, package loading/discovery/acceptance,
  renderer/model-slot behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, glTF/glb/JSON
  parser/dependency work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor c622c9ad HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
