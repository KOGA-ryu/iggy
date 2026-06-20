# 101 - Native Static Mesh Package Directory Manifest Comparison Helper Status Sync

## Goal

Sync planning and API docs after source commit
`e34f5ba4 Extract native static mesh package comparison helper`.

## Integrated Surface

- Extracted package-vs-nested-mesh-manifest row comparison into
  `NativeStaticMeshExportPackageDirectoryManifestComparisonResult` and
  `CompareNativeStaticMeshExportPackageDirectoryManifestRows(...)`.
- This is a behavior-preserving extraction. Report output text/order,
  `manifestMatches`, `manifestMismatches`, `manifestComparisonIssues`,
  `readOk()`, CLI exit behavior, package directory reader status/data, and
  verification/export behavior are unchanged.
- Comparison order remains package-order `MissingFromManifest` and
  `FilenameMismatch` rows first, then manifest-order `MissingFromPackage` rows.
- Valid CLI smoke still prints
  `manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0`, emits no
  `manifestComparison` rows, and keeps existing `manifestAsset=` plus package
  `asset=` rows.
- Combined mismatch coverage verifies
  `manifestMatches=1 manifestMismatches=3 manifestComparisonIssues=3` with row
  order `FilenameMismatch`, `MissingFromManifest`, `MissingFromPackage`.
- Missing or malformed nested manifest smokes still print
  `manifestRead=invalid manifestReadIssues=1`, no `manifestAsset=`, no
  `manifestComparison`, and no comparison summary fields.

## Verification

- `git show --stat --oneline --name-only e34f5ba4` showed only
  `NativeStaticMeshExportPackageDirectoryReport.hpp` and focused report tests.
- `git diff --check e34f5ba4^ e34f5ba4` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.

## Boundaries Preserved

- No docs-in-source in the source packet.
- No `IggyNativePlay.cpp`, CMake, package directory reader/status changes,
  manifest reader changes, verification/export behavior changes, `readOk()` or
  CLI exit behavior changes, or core `issues=` changes.
- No package acceptance/semantic verification, scanning/discovery, `.igmesh`
  loading, geometry validation, policy/built-in reconstruction, renderer
  changes, fixtures/generated assets, or glTF/glb/JSON parser work.

## Finisher Verification

- `git merge-base --is-ancestor e34f5ba4 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
