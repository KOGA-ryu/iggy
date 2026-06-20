# 91 - Native Static Mesh Package Directory Report Builder Status Sync

## Goal

Sync planning and API docs after source commit
`273db1df Add native static mesh package directory report`.

## Integrated Surface

- Added header-only no-write report builder over
  `ReadNativeStaticMeshExportPackageDirectory(...)`.
- New API:
  - `NativeStaticMeshExportPackageDirectoryReport`
  - `NativeStaticMeshExportPackageDirectoryReadStatusText(...)`
  - local package manifest read issue code text mapping for report rows
  - `BuildNativeStaticMeshExportPackageDirectoryReport(const std::filesystem::path &directory)`
- Report summary includes status, explicit directory, asset count, issue count,
  package manifest path when available, and nested mesh manifest path when
  available.
- Successful reports emit one asset row per parsed package asset in row order:
  `asset=<name> filename=<filename> path=<directory/filename>`.
- Package manifest read failures emit deterministic
  `packageManifestReadIssue code=... line=... token=...` rows.
- The report uses only the package directory reader result. It does not verify
  nested files or inspect beyond the reader.

## Non-Behavior

- No CLI, ParseArgs, usage, or `iggy_native_play` behavior changes.
- No verification, export, or package-loading integration.
- No nested mesh manifest existence check, mesh manifest parsing, `.igmesh`
  loading, geometry checks, deterministic generated-text comparison, directory
  scanning, exact-extra-file rejection, policy reconstruction, or built-in id
  reconstruction.

## Verification

- Builder verified configure:
  `cmake -S /Users/kogaryu/iggy/engine -B /Users/kogaryu/iggy/engine/build`.
- Builder verified focused build for
  `native_static_mesh_export_package_directory_report_tests`,
  `native_static_mesh_export_package_directory_reader_tests`,
  `native_static_mesh_export_package_manifest_tests`,
  `native_static_mesh_export_directory_verification_tests`,
  `native_static_mesh_export_directory_verification_report_tests`, and
  `native_static_mesh_file_export_tests`.
- Builder verified focused CTest for package directory report, package
  directory reader, package manifest, directory verification, verification
  report, file export, and package policy tests passed 7/7.
- Builder verified fresh valid export, verify, and verification report smoke.
- Builder verified malformed package sidecar verification report smoke failed
  nonzero with existing `PackageManifestReadFailed` and read issue row.
- Builder verified `git diff --check` and `git diff --cached --check` before
  source commit.

## Test Coverage

- Valid exported directory report includes summary and three asset rows.
- Missing directory reports summary only and creates nothing.
- File-not-directory reports summary only.
- Missing package sidecar reports `PackageManifestReadFailed` plus
  `FileOpenFailed` issue row.
- Malformed package sidecar reports parser issue row.
- Removing nested mesh manifest still reports `Read` and projected manifest
  path.
- Removing a declared mesh asset still reports `Read` and projected asset path.
- Extra unrelated file still reports `Read` and does not appear in output.

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No CLI flag, ParseArgs, usage, or native app behavior changes.
- No package loading, renderer integration, model-slot expansion, discovery/
  scanning/catalog/registry/source mutation/repair/exact-extra-file rejection.
- No export behavior changes, overwrite/create-directory/temp replacement/
  arbitrary output path policy.
- No semantic package acceptance, policy reconstruction, mesh manifest parser,
  `.igmesh` loading, geometry checks, schema/material/texture/normal/UV/
  animation expansion, glTF/JSON dependency, fixture rewrite, runtime/product/
  scene/server API, or gameplay changes.
- No next source packet opened or mixed into this docs packet.

## Finisher Verification

- `git merge-base --is-ancestor 273db1df HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
