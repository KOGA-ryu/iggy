# 133 - Native Static Mesh Asset Dump Writer Failure Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`889e1834 Extract static mesh asset writer failure text`.

## Integrated Surface

- Added pure helper
  `BuildNativeStaticMeshAssetWriteFailureText(std::string_view name, const NativeStaticMeshAssetWriteResult &result)`
  beside the writer result boundary in `NativeStaticMeshAssetWriter.hpp`.
- `PrintNativeStaticMeshAssetDump(...)` delegates only the writer failure body
  after writer failure.
- Unknown-asset handling remains before writer work.
- Raw successful `.igmesh` stdout remains unchanged.

## Preserved Failure Format

```text
failed to write static mesh asset: <name> issues=<N>
```

The helper emits the failure body without an embedded app prefix and without an
embedded trailing newline.

## Issue Count

- Uses `result.issues.size()` exactly as the previous inline CLI code did.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_asset_writer_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_asset_writer_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed for raw cube dump output: `# Native static mesh asset` and
  `tri 0 1 2`.
- CLI smoke passed for unchanged unknown asset failure:
  `iggy_native_play: unknown static mesh asset: nope`.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No unknown-asset behavior, raw asset dump success helper/output, writer result
  shape, `written()` semantic, issue generation, mesh validity, default policy,
  lookup, built-in mesh, CLI parser/help/dispatch/conflict behavior, file export,
  batch export, manifest/package manifest, verification, verification report,
  package-directory report, static model, renderer, asset/fixture, schema,
  package loading/discovery/acceptance, generated sidecar, export write policy,
  exact verifier, docs/source mixing, or CMake changes.

## Finisher Verification

- `git merge-base --is-ancestor 889e1834 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
