# 127 - Native Static Mesh Export Manifest Failure Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`c8a5096e Extract static mesh export manifest failure text`.

## Integrated Surface

- Added pure header helper
  `BuildNativeStaticMeshExportManifestFailureText(const NativeStaticMeshExportManifestResult &result)`
  beside the mesh export manifest result/status boundary in
  `NativeStaticMeshExportManifest.hpp`.
- `PrintNativeStaticMeshExportManifest()` now delegates the non-written failure
  body through the helper after the existing `!result.written()` check.
- Successful manifest dumping still prints `result.text` unchanged.

## Preserved Failure Format

```text
static mesh export manifest failed: <Status> issues=<N>
```

The helper intentionally excludes the `iggy_native_play:` prefix and trailing
newline. The existing exception/catch path still supplies both.

## Verification

- Baseline was clean at `4b76f6d0` before source edits.
- `git show --stat --oneline --name-only c8a5096e` showed only
  `NativeStaticMeshExportManifest.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_export_manifest_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_manifest_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_manifest_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Successful manifest smoke matched the exact header row and cube/bean/
  npc-marker asset rows.
- Exact invalid-policy helper coverage was added.

## Boundaries Preserved

- No successful `--dump-static-mesh-export-manifest` output, generated manifest
  text, manifest build semantic, `written()` semantic,
  `NativeStaticMeshExportManifestResult` data shape, status string, issue count
  semantic, CLI parser/help/dispatch/conflict/exit, app-level error
  prefix/newline, package manifest failure extraction, package directory report,
  exact verification/report, file export helper, static model surface, renderer/
  model-slot behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, glTF/glb/JSON
  parser/dependency work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor c8a5096e HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
