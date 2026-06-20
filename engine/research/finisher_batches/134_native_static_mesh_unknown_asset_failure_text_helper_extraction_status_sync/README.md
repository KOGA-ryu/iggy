# 134 - Native Static Mesh Unknown Asset Failure Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`daee0f85 Extract unknown static mesh asset failure text`.

## Integrated Surface

- Added pure helper
  `BuildUnknownNativeStaticMeshExportAssetFailureText(std::string_view name)`
  beside the static mesh export policy lookup surface in
  `NativeStaticMeshExportPolicy.hpp`.
- Raw dump unknown-asset failures route through the helper.
- Single output-dir export `UnknownAsset` failures route through the helper.
- Branch ordering is preserved for both call sites.

## Preserved Failure Format

```text
unknown static mesh asset: <name>
```

The helper emits the failure body without an embedded app prefix and without an
embedded trailing newline. The existing app-level catch path still supplies the
`iggy_native_play:` prefix and newline.

## Preserved Behavior

- Raw successful asset dump stdout remains unchanged.
- Single export success remains unchanged.
- Generic non-unknown file export failure text remains unchanged.
- Writer failure helper behavior remains unchanged.
- Policy lookup and `UnknownAsset` result semantics remain unchanged.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_policy_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_policy_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed for raw dump unknown asset failure.
- CLI smoke passed for output-dir export unknown asset failure.
- CLI smoke passed for raw cube dump output: `# Native static mesh asset` and
  `tri 0 1 2`.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No `NativeStaticMeshAssetWriter.hpp`, `NativeStaticMeshFileExport.hpp`,
  default policy, lookup behavior, built-in mesh data, output path selection,
  raw dump success output, single export helper behavior, generic non-unknown
  file export failure text, writer failure helper behavior, CLI
  parser/help/dispatch/conflict behavior, app-level catch behavior, file export,
  batch export, manifest/package manifest, verification, verification report,
  package-directory report, static model, renderer, asset/fixture, schema,
  package loading/discovery/acceptance, generated sidecar, export write policy,
  exact verifier, source/CMake, or docs/source split changes.

## Finisher Verification

- `git merge-base --is-ancestor daee0f85 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
