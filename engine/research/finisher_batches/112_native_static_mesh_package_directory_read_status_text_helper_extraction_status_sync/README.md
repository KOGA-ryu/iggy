# 112 - Native Static Mesh Package Directory Read Status Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`fe0b9a02 Move package directory read status text helper`.

## Integrated Surface

- Moved `NativeStaticMeshExportPackageDirectoryReadStatusText(...)` to the
  package-directory reader/result boundary in
  `NativeStaticMeshExportPackageDirectoryReader.hpp`.
- The helper now lives beside `NativeStaticMeshExportPackageDirectoryReadStatus`
  and `NativeStaticMeshExportPackageDirectoryReadResult`.
- Removed the report-local copy from
  `NativeStaticMeshExportPackageDirectoryReport.hpp`.
- Package-directory report summary rendering and the existing CLI compact
  failure status text path continue using the same helper name through includes.

## Stable Strings

- `Read`
- `MissingDirectory`
- `DirectoryNotDirectory`
- `PackageManifestReadFailed`
- fallback `Unknown`

## Preserved Behavior Contract

- Package-directory report text, row order, issue counts, `readOk()`, CLI exit
  behavior, compact CLI failure status text, reader status/data behavior, exact
  verification behavior, generated sidecar text, export behavior, and package
  acceptance semantics are preserved.

## Verification

- `git show --stat --oneline --name-only fe0b9a02` showed only the package-
  directory reader header, package-directory report header, and package-directory
  reader tests.
- `git diff --check fe0b9a02^ fe0b9a02` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_reader_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_reader_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed without touching CLI code.
- CLI valid package-directory report smoke passed and contained
  `static-mesh-export-package-directory-report status=Read`.
- CLI missing-package smoke failed nonzero after report text, report contained
  `status=PackageManifestReadFailed`, and stderr contained
  `static mesh export package directory report failed: PackageManifestReadFailed`.
- Direct reader tests cover `Read`, `MissingDirectory`, `DirectoryNotDirectory`,
  `PackageManifestReadFailed`, and `Unknown` fallback.

## Boundaries Preserved

- No package-directory reader/status data behavior changes.
- No package-directory report text/row/order/count changes, `readOk()` changes,
  CLI parser/dispatch/exit behavior changes, exact verification behavior
  changes, generated sidecar/export/package acceptance/write-policy changes,
  package loading/discovery, `.igmesh` loading beyond existing verifier behavior,
  renderer/model-slot changes, CMake/assets/fixtures, or glTF/glb/JSON
  parser/dependency work.

## Finisher Verification

- `git merge-base --is-ancestor fe0b9a02 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
