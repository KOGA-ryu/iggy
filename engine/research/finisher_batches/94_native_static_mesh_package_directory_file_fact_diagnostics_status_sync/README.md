# 94 - Native Static Mesh Package Directory File Fact Diagnostics Status Sync

## Goal

Sync planning and API docs after source commit
`4336c513 Add native static mesh package file fact diagnostics`.

## Integrated Surface

- Package directory report now adds read-only filesystem facts for already-known
  package paths.
- Summary now includes package sidecar facts:
  `packageManifestExists=1|0 packageManifestRegularFile=1|0 packageManifestBytes=N`.
- Summary keeps nested mesh manifest presence and adds
  `manifestRegularFile=1|0 manifestBytes=N`.
- Asset rows keep `exists=1|0` and add `regularFile=1|0 bytes=N`.
- Facts use non-throwing `std::filesystem` status/file-size calls; missing
  paths, directories, and size failures report `regularFile=0 bytes=0`.
- `readOk()` and CLI exit semantics are unchanged: missing nested mesh manifest,
  missing declared asset, and directory-at-asset remain `status=Read` / exit 0;
  missing package sidecar remains `PackageManifestReadFailed` / nonzero after
  report text.
- No nested manifest parsing, `.igmesh` loading, generated-text comparison,
  directory scanning, policy reconstruction, CLI parser changes, export behavior
  changes, renderer changes, docs-in-source, or CMake changes.

## Verification

- Builder verified focused build for
  `native_static_mesh_export_package_directory_report_tests` and
  `iggy_native_play`.
- Builder verified focused CTest for
  `native_static_mesh_export_package_directory_report_tests` passed.
- Builder verified valid package directory smoke passed and printed package
  sidecar, nested manifest, and all asset rows as existing regular files with
  byte counts: package manifest `250`, nested manifest `272`, cube `523`, bean
  `23882`, npc-marker `9474`.
- Builder verified missing nested mesh manifest smoke exited 0 with
  `manifestExists=0 manifestRegularFile=0 manifestBytes=0`.
- Builder verified missing declared asset smoke exited 0 with
  `cube.igmesh exists=0 regularFile=0 bytes=0`.
- Builder verified directory-at-asset smoke exited 0 with
  `cube.igmesh exists=1 regularFile=0 bytes=0`.
- Builder verified missing package sidecar smoke exited nonzero with
  `PackageManifestReadFailed`,
  `packageManifestExists=0 packageManifestRegularFile=0 packageManifestBytes=0`,
  and the existing `FileOpenFailed` issue row.
- Builder verified `git diff --check` and `git diff --cached --check` before
  source commit.

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- Changed source files stayed within approved report/test surfaces.
- No CLI, renderer, verification, export behavior, docs-in-source, CMake,
  runtime/product, shader, glTF/JSON/parser, or schema-scope changes.
- The only export helper references are existing test setup.
- No package discovery/scanning/catalog/registry, exact-extra-file rejection,
  repair, source mutation, arbitrary package loading, nested mesh manifest
  parsing, `.igmesh` loading, geometry checks, generated-text comparison,
  policy reconstruction, built-in id reconstruction, export write behavior,
  overwrite/create-dir/temp replacement/arbitrary output path policy,
  fixtures/generated assets, gameplay/scripted/final-state behavior, renderer
  behavior, `NativeVulkanRenderer.cpp`, or glTF/glb/JSON parser/dependency work.
- No next source packet opened or mixed into this docs packet.

## Finisher Verification

- `git merge-base --is-ancestor 4336c513 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
