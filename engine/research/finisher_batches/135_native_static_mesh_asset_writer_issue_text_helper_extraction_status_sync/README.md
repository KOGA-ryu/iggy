# 135 - Native Static Mesh Asset Writer Issue Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`5869297d Extract static mesh writer issue text`.

## Integrated Surface

- Added enum-owned helper
  `NativeStaticMeshAssetWriteIssueCodeText(NativeStaticMeshAssetWriteIssueCode code)`
  beside the writer issue enum in `NativeStaticMeshAssetWriter.hpp`.

## Stable Strings

- `InvalidMesh`
- `NonTriangleIndexCount`
- fallback `Unknown` for out-of-range values

## Current Consumers

- No CLI/app-shell text consumes the helper yet.
- Writer failure text continues to use issue count only.

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

- No `IggyNativePlay.cpp`, CMake, loader/parser, export policy,
  package/verification, renderer, asset/fixture, schema, package
  loading/acceptance/discovery, export write policy, generated sidecar, exact
  verifier, writer failure text, raw writer output, issue generation/order/count,
  writer result shape, `written()` semantic, or CLI behavior changes.

## Finisher Verification

- `git merge-base --is-ancestor 5869297d HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
