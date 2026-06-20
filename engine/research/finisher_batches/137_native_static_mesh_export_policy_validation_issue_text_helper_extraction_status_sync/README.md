# 137 - Native Static Mesh Export Policy Validation Issue Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`27da604a Extract static mesh export policy issue text`.

## Integrated Surface

- Added enum-owned helper
  `NativeStaticMeshExportPolicyValidationIssueCodeText(NativeStaticMeshExportPolicyValidationIssueCode code)`
  beside the base mesh export policy validation issue enum in
  `NativeStaticMeshExportPolicy.hpp`.

## Stable Strings

- `EmptyName`
- `DuplicateName`
- `EmptyDefaultFilename`
- `DefaultFilenameContainsSeparator`
- `DuplicateDefaultFilename`
- fallback `Unknown` for out-of-range values

## Current Consumers

- No CLI/app-shell/report/package-policy diagnostics consume the helper yet.
- Output bytes remain unchanged.

## Preserved Validation Behavior

- Issue generation order/counts remain unchanged.
- Result data remains unchanged.
- `assetIndex`, `previousAssetIndex`, and `value` fields remain unchanged.
- Default policy, lookup behavior, built-in asset mapping, and unknown asset
  failure text remain unchanged.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_policy_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_policy_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_policy_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_policy_tests --output-on-failure` passed.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No `IggyNativePlay.cpp`, `NativeStaticMeshExportPackagePolicy.hpp`,
  package-policy helper extraction, file export, manifest, package manifest,
  verification, report, package-directory, loader, writer, renderer, static
  model, CMake, source/docs mixing, asset/fixture, package
  loading/acceptance/discovery, exact verifier, generated sidecar, export write
  policy, schema, parser/dependency, or runtime output changes.

## Planner Note

- After packet 137 docs sync, do not assume the next close should open another
  helper-only packet.
- The next step should be a decision/scout toward visible static asset pipeline
  consumption, such as package acceptance/loading, renderer/package model-slot
  consumption, or authoring workflow integration, unless the user overrides.

## Finisher Verification

- `git merge-base --is-ancestor 27da604a HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
