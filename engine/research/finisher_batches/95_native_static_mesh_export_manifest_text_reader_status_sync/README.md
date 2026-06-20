# 95 - Native Static Mesh Export Manifest Text Reader Status Sync

## Goal

Sync planning and API docs after source commit
`1229e0ac Add native static mesh export manifest reader`.

## Integrated Surface

- Added an in-memory, dependency-free reader for the current generated native
  static mesh export manifest grammar only.
- New value/API surfaces in `NativeStaticMeshExportManifest.hpp`:
  - `NativeStaticMeshExportManifestAssetRow`
  - `NativeStaticMeshExportManifestDocument`
  - `NativeStaticMeshExportManifestReadIssueCode`
  - `NativeStaticMeshExportManifestReadIssue`
  - `NativeStaticMeshExportManifestReadResult`
  - `ReadNativeStaticMeshExportManifestText(std::string_view)`
- Accepted grammar:
  - `static-mesh-export-manifest version=1 assets=N bytes=N`
  - `asset=NAME filename=FILENAME vertices=V indices=I bytes=B`
- Reader validates empty input, malformed header, unsupported version, malformed
  asset and byte counts, missing fields, extra tokens, malformed or unexpected
  rows, basename-only filenames, unsigned numeric row facts, duplicate asset
  names and filenames, asset count mismatch, and total byte count mismatch.
- Generated default manifest readback was tested against builder output and
  report facts, preserving version, asset count, byte count, row order, and row
  facts.

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
  still reports existing manifest, package, and asset file facts.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No filesystem/file IO reader for mesh export manifests.
- No package directory report/reader integration.
- No verification/report/export/CLI behavior changes.
- No package loading, discovery/scanning/catalog/registry, semantic package
  acceptance, exact-extra-file rejection, or repair behavior.
- No reconstruction of `NativeStaticMeshExportPolicy` or built-in ids from mesh
  manifest rows.
- No nested mesh manifest file reader yet; this packet is text-only.
- No `.igmesh` loading, geometry validation, schema/material/texture/normal/UV/
  animation expansion.
- No renderer behavior, `NativeVulkanRenderer.cpp`, model-slot expansion,
  runtime/product/scene/server API, gameplay/scripted/final-state behavior,
  fixture/generated asset, CMake, glTF/glb/JSON dependency/parser, or
  write-policy changes.

## Finisher Verification

- `git merge-base --is-ancestor 1229e0ac HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
