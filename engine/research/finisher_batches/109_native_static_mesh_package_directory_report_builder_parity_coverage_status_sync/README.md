# 109 - Native Static Mesh Package Directory Report Builder Parity Coverage Status Sync

## Goal

Sync planning and API docs after source commit
`96cc7f04 Cover static mesh package report builder parity`.

## Integrated Surface

- Test-only parity coverage update for the package-directory report builder
  extraction.
- Added data-builder/full-builder parity plus text-renderer parity assertions to
  remaining package-directory report branches:
  - missing-from-manifest comparison;
  - missing-from-package comparison;
  - filename mismatch comparison;
  - missing directory;
  - file path instead of directory;
  - malformed package sidecar data-builder parity;
  - malformed nested manifest;
  - directory at declared asset path;
  - extra unrelated file ignored.
- Coverage uses existing helpers:
  `BuildNativeStaticMeshExportPackageDirectoryReportData(...)`,
  `ExpectDataBuilderMatchesFullReport(...)`, and
  `ExpectTextRendererMatches(...)`.

## Preserved Behavior Contract

- No production source changed in the source packet.
- Report text, CLI behavior, package-directory read behavior/status/issue counts
  and rows, manifest comparison row/count semantics, file facts, verifier
  behavior, exact sidecar matching, generated sidecar/export behavior, and
  package acceptance semantics are preserved.

## Verification

- `git show --stat --oneline --name-only 96cc7f04` showed only
  `engine/tests/native_static_mesh_export_package_directory_report_tests.cpp`.
- `git diff --check 96cc7f04^ 96cc7f04` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid package-directory report smoke passed with `status=Read`,
  `manifestRead=ok manifestReadIssues=0`,
  `manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0`, and the
  cube asset row.
- Missing package sidecar smoke preserved nonzero CLI behavior and report rows
  for `PackageManifestReadFailed`, missing package sidecar file facts, and
  `packageManifestReadIssue code=FileOpenFailed`.
- Missing nested mesh manifest smoke preserved exit 0 and report rows for
  `status=Read`, missing nested manifest file facts,
  `manifestRead=invalid manifestReadIssues=1`, and
  `manifestReadIssue code=FileOpenFailed`.

## Boundaries Preserved

- No package-directory report/reader internals changes.
- No exact verifier changes, package loading/discovery, `.igmesh` loading beyond
  existing verifier behavior, renderer/model-slot changes, CMake/assets/fixtures,
  or glTF/glb/JSON parser/dependency work.

## Finisher Verification

- `git merge-base --is-ancestor 96cc7f04 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
