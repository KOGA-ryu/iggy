# 114 - Native Static Mesh File Export Status Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`43b5916a Move file export status text helper`.

## Integrated Surface

- Added central inline helper `NativeStaticMeshFileExportStatusText(...)`
  beside `NativeStaticMeshFileExportStatus` in
  `NativeStaticMeshFileExport.hpp`.
- Removed the duplicate CLI-local `NativeStaticMeshFileExportStatusName(...)`
  switch from `IggyNativePlay.cpp`.
- Single-export and batch-export compact CLI failure paths now use the central
  helper.

## Stable Strings

- `Exported`
- `InvalidPolicy`
- `UnknownAsset`
- `MissingOutputDirectory`
- `OutputDirectoryNotDirectory`
- `TargetAlreadyExists`
- `WriterFailed`
- `FileOpenFailed`
- `WriteFailed`
- fallback `Unknown`

## Preserved Behavior Contract

- Single-export and batch-export success output are preserved.
- Compact failure prefixes and status strings are preserved, including
  single-export `static mesh export failed: TargetAlreadyExists` and
  batch-export `static mesh batch export failed: TargetAlreadyExists`.
- Export status assignment, preflight/write order, issue counts, output path
  selection, sidecar writes, no-overwrite/no-create-directory behavior,
  parser/dispatch/conflicts, verifier/report/package-directory behavior, and
  generated text are unchanged.

## Verification

- `git show --stat --oneline --name-only 43b5916a` showed only
  `NativeStaticMeshFileExport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_file_export_tests.cpp`.
- `git diff --check 43b5916a^ 43b5916a` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_file_export_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_file_export_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI single-export collision smoke passed with stderr containing
  `static mesh export failed: TargetAlreadyExists`.
- CLI batch-export collision smoke passed with stderr containing
  `static mesh batch export failed: TargetAlreadyExists`.
- Direct file-export status tests cover every current enum value plus
  `Unknown` fallback.

## Boundaries Preserved

- No single/batch success output changes.
- No compact failure prefix or status string changes.
- No export status assignment, preflight/write order, issue-count, output-path,
  sidecar, no-overwrite, no-create-directory, parser/dispatch/conflict,
  verifier, package-directory, report, or generated text changes.
- No CMake, fixtures, assets, renderer/model-slot behavior, package
  loading/discovery, schema, glTF/glb/JSON parser work, or source/docs scope
  mixing.

## Finisher Verification

- `git merge-base --is-ancestor 43b5916a HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
