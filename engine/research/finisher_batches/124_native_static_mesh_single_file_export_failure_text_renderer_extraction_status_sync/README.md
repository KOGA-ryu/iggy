# 124 - Native Static Mesh Single File Export Failure Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`099136bb Extract static mesh single export failure text`.

## Integrated Surface

- Added pure header helper
  `BuildNativeStaticMeshFileExportFailureText(const NativeStaticMeshFileExportResult &result)`
  in `NativeStaticMeshFileExport.hpp`.
- `PrintNativeStaticMeshAssetFileExport(...)` now delegates the non-`Exported`
  single-file export branch through
  `std::runtime_error(BuildNativeStaticMeshFileExportFailureText(result))`.
- The unknown-asset branch remains before the helper call.

## Preserved Failure Format

```text
static mesh export failed: <Status> output=<path> issues=<N>
```

The helper intentionally excludes the `iggy_native_play:` prefix and trailing
newline. The existing exception/catch path still supplies both.

## Preserved Behavior Contract

- Unknown-asset failure text is unchanged.
- Single-export success text remains routed through
  `BuildNativeStaticMeshFileExportSuccessText(...)`.
- Batch export success and failure text are unchanged.
- Export status assignment, write/preflight order, issue and byte count
  semantics, output path selection, no-overwrite/no-create-directory behavior,
  and CLI parser/help/dispatch/conflict/exit behavior are unchanged.

## Verification

- Baseline was clean at `61a6b879` before source edits.
- `git show --stat --oneline --name-only 099136bb` showed only
  `NativeStaticMeshFileExport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_file_export_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_file_export_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_file_export_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Success smoke preserved existing behavior:
  `static-mesh-export name=cube output=/tmp/iggy-native-single-export-failure-renderer-124/cube.igmesh bytes=523`.
- Collision smoke preserved existing failure behavior:
  `iggy_native_play: static mesh export failed: TargetAlreadyExists output=/tmp/iggy-native-single-export-failure-renderer-124/cube.igmesh issues=0`.
- Unknown-asset smoke preserved existing special-case behavior:
  `iggy_native_play: unknown static mesh asset: nope`.
- Exact helper tests cover target-exists and invalid-policy failure text.

## Boundaries Preserved

- No unknown-asset failure text, single-export success text, batch export
  success/failure text, export status assignment, write/preflight order, issue
  or byte count semantic, output path selection, no-overwrite/no-create-
  directory, CLI parser/help/dispatch/conflict/exit, static model,
  manifest/package/verification/package-directory, exact verification,
  generated sidecar, export write policy, package loading/discovery/acceptance,
  renderer/model-slot behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, glTF/glb/JSON
  parser/dependency work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor 099136bb HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
