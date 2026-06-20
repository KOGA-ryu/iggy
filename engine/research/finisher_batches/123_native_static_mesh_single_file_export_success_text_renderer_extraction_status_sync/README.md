# 123 - Native Static Mesh Single File Export Success Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`f69b1211 Extract static mesh single export success text`.

## Integrated Surface

- Added pure header helper
  `BuildNativeStaticMeshFileExportSuccessText(std::string_view name, const NativeStaticMeshFileExportResult &result)`
  in `NativeStaticMeshFileExport.hpp`.
- `PrintNativeStaticMeshAssetFileExport(...)` now delegates to the helper after
  the existing `Exported` status check.
- The helper serializes only successful single-file export compact stdout.

## Preserved Success Format

```text
static-mesh-export name=<name> output=<path> bytes=<N>
```

The emitted string is newline-terminated.

## Preserved Behavior Contract

- Unknown-asset failure text is unchanged.
- Non-`Exported` failure text is unchanged.
- Batch export success and failure text are unchanged.
- Export policy defaults, export status assignment, write/preflight order, issue
  and byte count semantics, output path selection, no-overwrite/no-create-
  directory behavior, and CLI parser/help/dispatch/conflict/exit behavior are
  unchanged.

## Verification

- Baseline was clean at `3c8266ac` before source edits.
- `git show --stat --oneline --name-only f69b1211` showed only
  `NativeStaticMeshFileExport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_file_export_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_file_export_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_file_export_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Single-export smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-mesh-asset cube --output-dir /tmp/iggy-native-single-export-renderer-123`
  printed
  `static-mesh-export name=cube output=/tmp/iggy-native-single-export-renderer-123/cube.igmesh bytes=523`
  and wrote `cube.igmesh`.
- Collision smoke preserved existing failure behavior:
  `iggy_native_play: static mesh export failed: TargetAlreadyExists output=/tmp/iggy-native-single-export-renderer-123/cube.igmesh issues=0`.
- Exact text tests cover a real successful cube single export result.

## Boundaries Preserved

- No export policy default, export status assignment, write/preflight order,
  issue/byte count semantic, output path selection, no-overwrite/no-create-
  directory, CLI parser/help/dispatch/conflict/exit, unknown-asset failure text,
  non-`Exported` failure text, batch export success/failure text, static model,
  manifest/package/verification/package-directory, exact verification, generated
  sidecar, export write policy, package loading/discovery/acceptance, renderer/
  model-slot behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, glTF/glb/JSON
  parser/dependency work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor f69b1211 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
