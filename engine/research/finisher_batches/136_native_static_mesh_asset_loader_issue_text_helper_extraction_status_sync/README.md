# 136 - Native Static Mesh Asset Loader Issue Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`a91e298b Extract static mesh loader issue text`.

## Integrated Surface

- Added enum-owned helper
  `NativeStaticMeshAssetLoadIssueCodeText(NativeStaticMeshAssetLoadIssueCode code)`
  beside the loader issue enum in `NativeStaticMeshAssetLoader.hpp`.

## Stable Strings

- `FileOpenFailed`
- `UnknownDirective`
- `MalformedVertex`
- `MalformedTriangle`
- `IndexOutOfRange`
- `ExtraToken`
- `InvalidMesh`
- fallback `Unknown` for out-of-range values

## Current Consumers

- No CLI/app-shell/report text consumes the helper yet.
- Loader issue rendering and all existing output remain unchanged.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_asset_loader_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_asset_loader_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_asset_writer_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_asset_writer_tests --output-on-failure` passed.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No `IggyNativePlay.cpp`, `NativeStaticMeshAssetWriter.hpp`, export policy,
  package policy, CMake, docs/source mixing, renderer/model-slot behavior,
  `NativeVulkanRenderer.cpp`, package loading/semantic acceptance/discovery/
  catalog, export write policy, exact verifier, generated sidecar,
  asset/fixture rewrite, `.igmesh` schema/material/texture/normal/UV/animation
  expansion, or glTF/glb/JSON parser/dependency work.
- Loader parser behavior, issue generation order/counts, issue line/token data,
  `loaded()` semantics, file-open behavior, `.igmesh` text grammar, runtime
  behavior, and user-facing CLI output remain unchanged.

## Finisher Verification

- `git merge-base --is-ancestor a91e298b HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
