# 99 - Native Static Mesh Package Directory Manifest Row Comparison Diagnostics Status Sync

## Goal

Sync planning and API docs after source commit
`739e27f7 Add native static mesh package manifest comparison diagnostics`.

## Integrated Surface

- Package directory report now performs diagnostic-only comparison between
  package sidecar asset rows and parsed nested mesh manifest rows, only when the
  nested manifest reads successfully.
- Summary appends `manifestMatches=N manifestMismatches=N` when
  `manifestRead=ok`.
- New deterministic mismatch rows are emitted after `manifestAsset=` rows and
  before package `asset=` rows:
  - `manifestComparison code=MissingFromManifest asset=<packageName> packageFilename=<packageFilename>`
  - `manifestComparison code=MissingFromPackage manifestAsset=<manifestName> manifestFilename=<manifestFilename>`
  - `manifestComparison code=FilenameMismatch asset=<name> packageFilename=<packageFilename> manifestFilename=<manifestFilename>`
- Comparison is by asset name and filename only.
- Valid default batch export reports `manifestMatches=3 manifestMismatches=0`
  and no `manifestComparison` rows.
- Missing or malformed nested manifests do not emit comparison summary or rows;
  existing `manifestReadIssue` diagnostics remain unchanged.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid smoke passed: batch export followed by package directory report printed
  `manifestMatches=3 manifestMismatches=0`, no `manifestComparison` rows,
  existing `manifestAsset=` rows, and package `asset=` rows.
- Missing-from-manifest smoke exited 0 and printed
  `manifestComparison code=MissingFromManifest asset=npc-marker packageFilename=npc-marker.igmesh`.
- Missing-from-package smoke exited 0 and printed
  `manifestComparison code=MissingFromPackage manifestAsset=extra manifestFilename=extra.igmesh`.
- Filename-mismatch smoke exited 0 and printed
  `manifestComparison code=FilenameMismatch asset=cube packageFilename=cube.igmesh manifestFilename=cube-renamed.igmesh`.
- Missing/malformed nested manifest smokes exited 0 and emitted no
  `manifestComparison` rows while preserving existing `manifestReadIssue` rows.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- Diagnostic-only comparison; no package acceptance, verification, nonzero CLI
  behavior, or issue-count/status semantics.
- No exact deterministic verification replacement, generated-text comparison,
  semantic package acceptance, or package acceptance validation.
- No package directory reader status/data changes, `readOk()` changes, or CLI
  exit behavior changes.
- Missing/malformed nested mesh manifests still emit no comparison summary/rows
  and keep existing `manifestReadIssue` behavior.
- No policy reconstruction, built-in id reconstruction, `.igmesh` loading,
  geometry validation, asset path traversal from mesh-manifest rows, file facts
  for mesh-manifest-declared filenames, package loading,
  discovery/scanning/catalog/registry, exact-extra-file rejection, repair,
  source mutation, or write behavior.
- No renderer behavior, `NativeVulkanRenderer.cpp`, model-slot expansion,
  runtime/product/scene/server API, gameplay/scripted/final-state behavior,
  fixture/generated asset, CMake, glTF/glb/JSON dependency/parser, or
  schema/material/texture/normal/UV/animation changes.

## Finisher Verification

- `git merge-base --is-ancestor 739e27f7 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
