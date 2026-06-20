# 93 - Native Static Mesh Package Directory Presence Diagnostics Status Sync

## Goal

Sync planning and API docs after source commit
`4fc98ce4 Add native static mesh package presence diagnostics`.

## Integrated Surface

- Package directory report now emits read-only presence facts for paths already
  projected by the package directory reader.
- Summary appends `manifestExists=1|0` when the nested mesh manifest path is
  available.
- Asset rows append `exists=1|0` for each declared package asset path.
- `readOk()` and CLI exit semantics are unchanged: missing nested mesh manifest
  or declared mesh asset remains `status=Read` / exit 0; missing package
  sidecar remains `PackageManifestReadFailed` / nonzero.
- No nested mesh manifest parsing, `.igmesh` loading, generated-text
  comparison, directory scanning, policy reconstruction, CLI parser changes,
  export behavior changes, renderer changes, docs-in-source, or CMake changes.

## Verification

- Builder verified focused build for
  `native_static_mesh_export_package_directory_report_tests` and
  `iggy_native_play`.
- Builder verified focused CTest for
  `native_static_mesh_export_package_directory_report_tests` passed.
- Builder verified valid package directory smoke passed and printed
  `manifestExists=1` plus `exists=1` for `cube`, `bean`, and `npc-marker`.
- Builder verified missing nested mesh manifest smoke exited 0 with
  `status=Read` and `manifestExists=0`.
- Builder verified missing declared asset smoke exited 0 with
  `cube.igmesh exists=0` and remaining assets `exists=1`.
- Builder verified missing package sidecar smoke exited nonzero with
  `PackageManifestReadFailed` and `FileOpenFailed` issue row.
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

- `git merge-base --is-ancestor 4fc98ce4 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
