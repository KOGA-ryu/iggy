# 113 - Native Static Mesh Export Directory Verification Status Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`e43a5e9a Move export verification status text helper`.

## Integrated Surface

- Added central inline helper
  `NativeStaticMeshExportDirectoryVerificationStatusText(...)` beside
  `NativeStaticMeshExportDirectoryVerificationStatus` in
  `NativeStaticMeshExportDirectoryVerification.hpp`.
- Removed the duplicate report-local verification status text switch from
  `NativeStaticMeshExportDirectoryVerificationReport.hpp`.
- Removed the duplicate CLI-local
  `NativeStaticMeshExportDirectoryVerificationStatusName(...)` switch from
  `IggyNativePlay.cpp`.
- Verification report summary and per-asset status rendering plus the two
  compact CLI failure paths now use the central helper.

## Stable Strings

- `Verified`
- `InvalidPolicy`
- `MissingOutputDirectory`
- `OutputDirectoryNotDirectory`
- `MissingManifest`
- `ManifestMismatch`
- `MissingAsset`
- `AssetLoadFailed`
- `GeometryMismatch`
- `ManifestBuildFailed`
- `MissingPackageManifest`
- `PackageManifestReadFailed`
- `PackageManifestMismatch`
- `PackageManifestBuildFailed`
- fallback `Unknown`

## Preserved Behavior Contract

- Report `status=...`, per-asset `status=...`,
  `--verify-static-mesh-export` compact failure text, and
  `--dump-static-mesh-export-verification-report` compact failure text are
  preserved.
- No CLI parser/dispatch/help/success output, verifier logic/status ordering,
  issue counts, problem paths, verified flags, entry data, sidecar matching,
  generated sidecar/export behavior, package acceptance, or package-directory
  diagnostics changed.

## Verification

- `git show --stat --oneline --name-only e43a5e9a` showed only
  `IggyNativePlay.cpp`, the export directory verification header, verification
  report header, and verification tests.
- `git diff --check e43a5e9a^ e43a5e9a` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_directory_verification_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_directory_verification_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_directory_verification_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_directory_verification_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI valid export verification smoke passed with compact success output.
- CLI missing-manifest verification failure smoke preserved stderr containing
  `static mesh export verification failed: MissingManifest`.
- CLI missing-manifest verification report failure smoke preserved stdout
  containing `static-mesh-export-verification-report status=MissingManifest` and
  stderr containing `static mesh export verification report failed: MissingManifest`.
- Direct verification status tests cover every current enum value plus
  `Unknown` fallback.

## Boundaries Preserved

- No report text/per-asset status/compact CLI failure string changes.
- No CLI parser/dispatch/help/success output changes, verifier logic/status
  ordering/issue counts/problem paths/verified flags/entry data changes,
  sidecar matching/generated sidecar/export/package acceptance changes,
  package-directory diagnostics changes, CMake/fixtures/assets/renderer/
  model-slot/package loading/discovery/schema/glTF/parser work.

## Finisher Verification

- `git merge-base --is-ancestor e43a5e9a HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
