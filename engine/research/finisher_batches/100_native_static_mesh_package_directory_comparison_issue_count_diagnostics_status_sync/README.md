# 100 - Native Static Mesh Package Directory Comparison Issue Count Diagnostics Status Sync

## Goal

Sync planning and API docs after source commit
`76650efc Add native static mesh package comparison issue count`.

## Integrated Surface

- Package directory report summary now includes `manifestComparisonIssues=N`
  when nested manifest comparison runs.
- `manifestComparisonIssues` equals the number of emitted
  `manifestComparison` rows and matches `manifestComparisons.size()`.
- Valid default export prints
  `manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0` and still
  emits no comparison rows.
- One-row mismatch cases print `manifestComparisonIssues=1` while preserving
  existing row text and order for:
  - `MissingFromManifest`
  - `MissingFromPackage`
  - `FilenameMismatch`
- Missing/malformed nested manifests still emit no comparison summary/rows and
  preserve existing `manifestRead=invalid manifestReadIssues=N` behavior.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Source `git diff --check 76650efc^ 76650efc` passed.

## Boundaries Preserved

- Diagnostic-only summary text.
- No core `issues=` or `report.read.issueCount` changes.
- No package directory reader status/data changes, `readOk()` changes, or CLI
  exit behavior changes.
- No verification behavior, package acceptance, generated sidecar text, export
  behavior, renderer behavior, docs-in-source, CMake, assets, or fixtures
  changes.
- No `.igmesh` loading, geometry validation, discovery/scanning, or write
  policy changes.

## Finisher Verification

- `git merge-base --is-ancestor 76650efc HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
