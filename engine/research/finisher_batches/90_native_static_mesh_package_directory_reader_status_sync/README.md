# 90 - Native Static Mesh Package Directory Reader Status Sync

## Goal

Sync planning and API docs after source commit
`1f0aeb55 Add native static mesh package directory reader`.

## Integrated Surface

- Added header-only explicit package directory reader API:
  `NativeStaticMeshExportPackageDirectoryReadStatus`,
  `NativeStaticMeshExportPackageDirectoryAsset`,
  `NativeStaticMeshExportPackageDirectoryReadResult`, and
  `ReadNativeStaticMeshExportPackageDirectory(const std::filesystem::path &directory)`.
- The reader records the supplied directory, requires it to exist and be a
  directory, composes `directory / static-mesh-export-package-manifest.txt`, and
  delegates to `ReadNativeStaticMeshExportPackageManifestFile(...)`.
- On package manifest read failure, it returns `PackageManifestReadFailed`,
  copies package manifest read issues, sets `issueCount`, and does not inspect
  the nested manifest or asset files.
- On success, it copies the parsed package manifest document, projects
  `manifestPath = directory / document.manifestFilename`, and projects each
  parsed asset row to `{ name, filename, directory / filename }` in row order.
- `read()` returns true only for `Read`.

## Non-Behavior

- Does not compare package sidecar text to generated default text.
- Does not verify nested mesh manifest text.
- Does not parse mesh export manifests.
- Does not load `.igmesh` assets or check geometry.
- Does not check existence of nested mesh manifest or asset files.
- Does not reconstruct export policy or built-in ids from package rows.
- Does not scan directories or reject extra files.
- No CLI, export, verification, renderer, or package-loading integration.

## Verification

- Builder verified configure:
  `cmake -S /Users/kogaryu/iggy/engine -B /Users/kogaryu/iggy/engine/build`.
- Builder verified focused build for
  `native_static_mesh_export_package_directory_reader_tests`,
  `native_static_mesh_export_package_manifest_tests`,
  `native_static_mesh_export_directory_verification_tests`,
  `native_static_mesh_export_directory_verification_report_tests`,
  `native_static_mesh_file_export_tests`, and `iggy_native_play`.
- Builder verified focused CTest for package directory reader, package manifest,
  directory verification, verification report, file export, and package policy
  tests passed 6/6.
- Builder verified fresh valid export, verify, and verification report smoke.
- Builder verified malformed package sidecar verification report smoke failed
  nonzero with existing `PackageManifestReadFailed` and read issue row.
- Builder verified `git diff --check` and `git diff --cached --check` before
  source commit.

## Test Coverage

- Batch-exported temp directory reads successfully and returns package sidecar
  path, nested mesh manifest path, three parsed rows, and three projected asset
  paths in order.
- Missing directory returns `MissingDirectory` and creates nothing.
- File path returns `DirectoryNotDirectory`.
- Missing package sidecar returns `PackageManifestReadFailed` with
  `FileOpenFailed` from the explicit file reader.
- Malformed package sidecar returns `PackageManifestReadFailed` with the
  underlying text-reader issue.
- Removing the nested mesh manifest does not fail the reader.
- Removing a declared mesh asset does not fail the reader.
- Extra unrelated files are ignored.

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No CLI, ParseArgs, usage, or `iggy_native_play` behavior changes.
- No verification/report/export/package-loading integration.
- No package discovery/scanning/catalog/registry, exact-extra-file rejection,
  repair, source mutation, semantic package acceptance, or package policy
  reconstruction.
- No nested mesh manifest parsing, `.igmesh` loading, geometry checks, renderer
  changes, schema changes, fixtures, runtime/product/scene/server API changes,
  gameplay changes, glTF/JSON dependency, or export write policy changes.
- No next source packet opened or mixed into this docs packet.

## Finisher Verification

- `git merge-base --is-ancestor 1f0aeb55 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
