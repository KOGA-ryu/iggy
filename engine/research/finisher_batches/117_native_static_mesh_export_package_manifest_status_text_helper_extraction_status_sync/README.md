# 117 - Native Static Mesh Export Package Manifest Status Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`866691ff Move package manifest status text helper`.

## Integrated Surface

- Added central inline helper
  `NativeStaticMeshExportPackageManifestStatusText(...)` beside
  `NativeStaticMeshExportPackageManifestStatus` in
  `NativeStaticMeshExportPackageManifest.hpp`.
- Removed the duplicate CLI-local
  `NativeStaticMeshExportPackageManifestStatusName(...)` switch from
  `IggyNativePlay.cpp`.
- `PrintNativeStaticMeshExportPackageManifest()` compact failure text now uses
  the central helper.

## Stable Strings

- `Built`
- `InvalidPolicy`
- fallback `Unknown`

## Preserved Behavior Contract

- Successful `--dump-static-mesh-export-package-manifest` output is preserved
  byte-for-byte for default built-ins.
- The preserved output contract includes the header, cube/bean/npc-marker rows,
  row order, format id, version, nested manifest filename, asset count, and
  trailing newlines.
- Compact failure string shape and status text are preserved.
- Builder validation/write semantics, `written()` behavior, reader/file-reader
  behavior, and generated package sidecar content are unchanged.

## Verification

- `git show --stat --oneline --name-only 866691ff` showed only
  `NativeStaticMeshExportPackageManifest.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_export_package_manifest_tests.cpp`.
- `git diff --check 866691ff^ 866691ff` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_manifest_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_manifest_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-mesh-export-package-manifest`
  matched the exact expected header plus cube, bean, and npc-marker rows.
- Direct package manifest status tests cover `Built`, `InvalidPolicy`, and
  `Unknown` fallback.

## Boundaries Preserved

- No successful package manifest output changes.
- No compact failure string shape or status text changes.
- No builder validation/write/`written()`/reader/file-reader/generated sidecar
  behavior changes.
- No CLI parser/help/dispatch/conflict changes.
- No mesh manifest helper, package-directory, exact verification, file export,
  export report, export policy, or asset writer behavior changes.
- No CMake, fixtures, assets, renderer/model-slot behavior, package
  loading/discovery, schema, glTF/glb/JSON parser work, or source/docs scope
  mixing.

## Finisher Verification

- `git merge-base --is-ancestor 866691ff HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
