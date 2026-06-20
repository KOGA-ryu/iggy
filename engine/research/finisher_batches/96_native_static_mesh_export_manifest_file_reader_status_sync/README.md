# 96 - Native Static Mesh Export Manifest File Reader Status Sync

## Goal

Sync planning and API docs after source commit
`442c014c Add native static mesh export manifest file reader`.

## Integrated Surface

- Added `NativeStaticMeshExportManifestReadIssueCode::FileOpenFailed`.
- Added
  `ReadNativeStaticMeshExportManifestFile(const std::filesystem::path &path)`.
- The file wrapper opens exactly the supplied path in binary mode.
- On open failure, it returns one `FileOpenFailed` issue with `line=0` and
  `token=path.string()`.
- On open success, it reads the full file into memory and delegates unchanged to
  `ReadNativeStaticMeshExportManifestText(...)`.
- Tests cover generated-file read success, missing explicit file
  `FileOpenFailed`, and propagation of text-reader issues from a readable
  malformed file.
- Existing mesh export manifest text reader behavior and generated manifest
  output are unchanged.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_manifest_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_manifest_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-mesh-export-manifest` passed and preserved:
  - `static-mesh-export-manifest version=1 assets=3 bytes=33879`
  - cube row `vertices=8 indices=36 bytes=523`
  - bean row `vertices=234 indices=1296 bytes=23882`
  - npc-marker row `vertices=98 indices=504 bytes=9474`
- Optional no-change smoke passed: batch export followed by
  `--dump-static-mesh-export-package-directory-report --output-dir <tmpdir>`
  still reports existing package directory file facts.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No package-directory reader/report integration.
- No verification/report/export/CLI behavior changes.
- No package loading, discovery/scanning/catalog/registry, semantic package
  acceptance, exact-extra-file rejection, or repair behavior.
- No default path composition, directory traversal, or reading via package
  directory.
- No reconstruction of `NativeStaticMeshExportPolicy` or built-in ids from mesh
  manifest rows.
- No nested mesh manifest report rows; this packet only adds the explicit file
  reader.
- No `.igmesh` loading, geometry validation, schema/material/texture/normal/UV/
  animation expansion.
- No renderer behavior, `NativeVulkanRenderer.cpp`, model-slot expansion,
  runtime/product/scene/server API, gameplay/scripted/final-state behavior,
  fixture/generated asset, CMake, glTF/glb/JSON dependency/parser, or
  write-policy changes.

## Finisher Verification

- `git merge-base --is-ancestor 442c014c HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
