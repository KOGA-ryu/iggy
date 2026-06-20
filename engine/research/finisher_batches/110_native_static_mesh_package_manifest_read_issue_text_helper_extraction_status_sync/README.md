# 110 - Native Static Mesh Package Manifest Read Issue Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`fe339a95 Extract package manifest issue text helper`.

## Integrated Surface

- Added central inline helper
  `NativeStaticMeshExportPackageManifestReadIssueCodeText(...)` in
  `NativeStaticMeshExportPackageManifest.hpp`.
- Removed duplicated package-manifest read issue mapping switches from
  `NativeStaticMeshExportDirectoryVerificationReport.hpp` and
  `NativeStaticMeshExportPackageDirectoryReport.hpp`.
- Verification report and package-directory report package manifest read issue
  rows now route through the central helper.
- The nested mesh manifest issue-code mapper in
  `NativeStaticMeshExportPackageDirectoryReport.hpp` remains unchanged because it
  maps the separate `NativeStaticMeshExportManifestReadIssueCode` enum.

## Preserved Behavior Contract

- Report text, row order, issue counts, reader behavior, verifier behavior,
  package-directory read behavior, CLI behavior, export behavior, and generated
  sidecar text are unchanged.
- Exact sidecar matching and package acceptance semantics are unchanged.

## Verification

- `git show --stat --oneline --name-only fe339a95` showed only the package
  manifest header, verification report header, package-directory report header,
  and package manifest tests.
- `git diff --check fe339a95^ fe339a95` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_manifest_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_manifest_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_directory_verification_report_tests native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R 'native_static_mesh_export_(directory_verification_report|package_directory_report)_tests' --output-on-failure` passed 2/2.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid export smoke exported a temp static mesh package, dumped verification and
  package-directory reports, and confirmed no `packageManifestReadIssue code=`
  rows were emitted for valid reports.
- Direct package manifest tests cover every current package manifest read issue
  enum string plus the `Unknown` fallback.

## Boundaries Preserved

- No package-directory reader/status changes.
- No verifier internals changes, export behavior changes, package
  loading/discovery, `.igmesh` loading beyond existing verifier behavior,
  renderer/model-slot changes, CMake/assets/fixtures, or glTF/glb/JSON
  parser/dependency work.

## Finisher Verification

- `git merge-base --is-ancestor fe339a95 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
