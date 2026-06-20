# 111 - Native Static Mesh Export Manifest Read Issue Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`3d357b45 Extract mesh manifest issue text helper`.

## Integrated Surface

- Added central inline helper
  `NativeStaticMeshExportManifestReadIssueCodeText(...)` beside the mesh export
  manifest read enum/reader in `NativeStaticMeshExportManifest.hpp`.
- Removed the redundant local nested mesh manifest issue-code mapper from
  `NativeStaticMeshExportPackageDirectoryReport.hpp`.
- Package-directory report `manifestReadIssue` rows now route through the
  central mesh manifest helper.
- This mirrors packet 110's package manifest helper extraction while staying on
  the separate `NativeStaticMeshExportManifestReadIssueCode` enum.

## Preserved Behavior Contract

- Package-directory report text and behavior are unchanged.
- Preserved scope includes summary fields, `manifestReadIssue code=...` rows,
  tokens, counts, row order, trailing newlines, `readOk()`, CLI exit behavior,
  reader behavior, exact verification, generated sidecar text, and export
  behavior.

## Verification

- `git show --stat --oneline --name-only 3d357b45` showed only the mesh export
  manifest header, package-directory report header, and mesh export manifest
  tests.
- `git diff --check 3d357b45^ 3d357b45` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_manifest_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_manifest_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Missing nested mesh manifest smoke exported a temp package, removed
  `static-mesh-export-manifest.txt`, dumped the package-directory report, and
  confirmed `manifestReadIssue code=FileOpenFailed` remained present.
- Direct mesh export manifest tests cover every current read issue enum string
  plus the `Unknown` fallback.

## Boundaries Preserved

- No package-directory reader/status changes.
- No verifier internals changes, package manifest helper changes, export
  behavior changes, package loading/discovery, `.igmesh` loading beyond existing
  verifier behavior, renderer/model-slot changes, CMake/assets/fixtures, or
  glTF/glb/JSON parser/dependency work.

## Finisher Verification

- `git merge-base --is-ancestor 3d357b45 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
