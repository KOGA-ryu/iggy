# 116 - Native Static Mesh Export Manifest Status Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`49b1bc4c Move export manifest status text helper`.

## Integrated Surface

- Added central inline helper `NativeStaticMeshExportManifestStatusText(...)`
  beside `NativeStaticMeshExportManifestStatus` in
  `NativeStaticMeshExportManifest.hpp`.
- Removed the duplicate CLI-local
  `NativeStaticMeshExportManifestStatusName(...)` switch from
  `IggyNativePlay.cpp`.
- `PrintNativeStaticMeshExportManifest()` compact failure text now uses the
  central helper.

## Stable Strings

- `Built`
- `InvalidPolicy`
- `WriterFailed`
- fallback `Unknown`

## Preserved Behavior Contract

- Successful `--dump-static-mesh-export-manifest` output is preserved
  byte-for-byte for default built-ins.
- The preserved output contract includes the header, cube/bean/npc-marker rows,
  order, counts, byte totals, and trailing newlines.
- Compact failure string shape and status text are preserved.
- Builder validation/write semantics, `written()` behavior, reader/file-reader
  behavior, and generated sidecar content are unchanged.

## Verification

- `git show --stat --oneline --name-only 49b1bc4c` showed only
  `NativeStaticMeshExportManifest.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_export_manifest_tests.cpp`.
- `git diff --check 49b1bc4c^ 49b1bc4c` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_manifest_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_manifest_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-mesh-export-manifest`
  matched the exact expected header plus cube, bean, and npc-marker rows.
- Direct mesh export manifest status tests cover `Built`, `InvalidPolicy`,
  `WriterFailed`, and `Unknown` fallback.

## Boundaries Preserved

- No successful manifest output changes.
- No compact failure string shape or status text changes.
- No builder validation/write/`written()`/reader/file-reader/generated sidecar
  behavior changes.
- No CLI parser/help/dispatch/conflict changes.
- No package manifest status/helper, package-directory, exact verification,
  file export, export report, export policy, or asset writer behavior changes.
- No CMake, fixtures, assets, renderer/model-slot behavior, package
  loading/discovery, schema, glTF/glb/JSON parser work, or source/docs scope
  mixing.

## Finisher Verification

- `git merge-base --is-ancestor 49b1bc4c HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
