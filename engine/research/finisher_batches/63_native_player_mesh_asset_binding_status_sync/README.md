# 63 Native Player Mesh Asset Binding Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt rendering bound the first
checked-in text mesh asset to a renderer-private model slot.

## Integrated Commit

- `14320a8d Bind native player mesh asset`

## Integrated Surface

- `engine/apps/native_play/assets/player.igmesh` adds the first checked-in
  minimal native text mesh asset.
- `iggy_native_play` receives `IGGY_NATIVE_PLAY_ASSET_DIR` pointing at
  `apps/native_play/assets`.
- `NativeVulkanRenderer.cpp` loads `player.igmesh` through
  `LoadNativeStaticMeshAssetFile(...)` while creating scene meshes.
- A valid loaded mesh becomes `playerMesh_` and remains bound to
  `NativeVulkanModelSlot::Player`.
- If loading fails or validates false, the procedural bean remains the silent
  fallback.
- `native_static_mesh_asset_loader_tests` validates the checked-in player asset
  fixture.

## Source Verification Facts

- Source packet touched only:
  - `engine/apps/native_play/NativeVulkanRenderer.cpp`
  - `engine/apps/native_play/assets/player.igmesh`
  - `engine/cmake/iggy_core_tests.cmake`
  - `engine/cmake/iggy_native_play.cmake`
  - `engine/tests/native_static_mesh_asset_loader_tests.cpp`
- CMake configure passed after adding the native asset directory compile
  definitions.
- `native_static_mesh_asset_loader_tests` built and passed.
- `iggy_native_play` built.
- Focused product CTest regex passed 10/10.
- Focused render/resource CTest regex passed 9/9.
- `./engine/build/iggy_native_play --help` passed.
- Native SDL/Vulkan smoke passed with display access and preserved the scripted
  debug/final-state lines.
- `git diff --check` passed.
- Guard checks found no new glTF/material/texture/descriptor/sampler/staging/
  device-local/session/app-shell/SDL/Vulkan policy beyond the approved asset
  directory compile definitions.

## Boundaries Preserved

- One loaded text mesh is bound to one renderer-private model slot only.
- No public renderer API change.
- No app-shell/CLI option change.
- No product session, runtime/product/scene API, or draw-list change.
- No shader change.
- No glTF/static model parsing.
- No asset registry/catalog, package discovery, resource catalog, or model
  authoring policy.
- No material/texture/descriptor/sampler policy.
- No staging/device-local upload policy.
- No Linux/dGPU validation or backend abstraction.
- No CLI/debugger output, gameplay, session, input, or scripted-control change.

## Verification

- `git merge-base --is-ancestor 14320a8d HEAD`
- `git diff --check`
- `git status --short --branch`
