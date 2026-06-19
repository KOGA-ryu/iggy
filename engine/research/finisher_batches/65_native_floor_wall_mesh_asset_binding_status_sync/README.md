# 65 Native Floor/Wall Mesh Asset Binding Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt rendering bound checked-in floor
and wall text mesh assets to renderer-private terrain model slots.

## Integrated Commit

- `75fea578 Bind native floor wall mesh assets`

## Integrated Surface

- `engine/apps/native_play/assets/floor.igmesh` and
  `engine/apps/native_play/assets/wall.igmesh` add checked-in native text mesh
  assets for the floor and wall slots.
- `NativeVulkanRenderer.cpp` loads both files via
  `LoadNativeStaticMeshAssetFile(NativePlayAssetPath(...))` while creating
  scene meshes.
- `floorMesh_` and `wallMesh_` are created only when the corresponding loaded
  asset succeeds.
- `NativeVulkanModelSlot::Floor` and `NativeVulkanModelSlot::Wall` register to
  loaded floor/wall meshes only when `HasMesh(...)` succeeds.
- If either file load fails or validates false, that slot reuses the existing
  `cubeMesh_` fallback.
- Floor/wall fallback does not upload duplicate cube GPU meshes.
- `native_static_mesh_asset_loader_tests` validates the checked-in floor and
  wall fixture counts.
- Existing Player and NPC file bindings remain unchanged.

## Source Verification Facts

- Source packet touched only:
  - `engine/apps/native_play/NativeVulkanRenderer.cpp`
  - `engine/apps/native_play/assets/floor.igmesh`
  - `engine/apps/native_play/assets/wall.igmesh`
  - `engine/tests/native_static_mesh_asset_loader_tests.cpp`
- Source reviewer confirmed clean worktree at `75fea578`.
- `git diff --check` passed for the source packet.
- Native build passed.
- Focused product CTest passed.
- Focused render/resource CTest passed.
- `./engine/build/iggy_native_play --help` passed.
- Builder native smoke passed with display access and preserved debug/final-state
  lines.

## Boundaries Preserved

- No public renderer API change.
- No CMake change.
- No app-shell/CLI option change.
- No product session, runtime/product/scene API, or draw-list change.
- No shader change.
- No descriptor/sampler/material/texture policy.
- No staging/device-local upload policy.
- No asset registry/catalog, package discovery, glTF/static model parsing, or
  authoring asset policy.
- No Linux/dGPU validation or backend abstraction.
- No CLI/debugger output, gameplay, session, input, or scripted-control change.

## Verification

- `git merge-base --is-ancestor 75fea578 HEAD`
- `git diff --check`
- `git diff --cached --check`
- `git status --short --branch`
