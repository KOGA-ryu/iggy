# 64 Native NPC Mesh Asset Binding Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt rendering bound the checked-in NPC
text mesh asset to the renderer-private NPC model slot.

## Integrated Commit

- `178a0174 Bind native NPC mesh asset`

## Integrated Surface

- `engine/apps/native_play/assets/npc.igmesh` adds a checked-in minimal native
  NPC text mesh asset.
- `NativeVulkanRenderer.cpp` loads `npc.igmesh` through
  `LoadNativeStaticMeshAssetFile(...)` while creating scene meshes.
- A valid loaded mesh becomes `npcMesh_` and remains bound to
  `NativeVulkanModelSlot::NpcActor`.
- If loading fails or validates false, the procedural NPC marker remains the
  silent fallback.
- `native_static_mesh_asset_loader_tests` validates the checked-in NPC asset
  fixture.

## Source Verification Facts

- Source packet touched only:
  - `engine/apps/native_play/NativeVulkanRenderer.cpp`
  - `engine/apps/native_play/assets/npc.igmesh`
  - `engine/tests/native_static_mesh_asset_loader_tests.cpp`
- `native_static_mesh_asset_loader_tests` built and passed.
- `iggy_native_play` built.
- Focused product CTest regex passed 10/10.
- Focused render/resource CTest regex passed 9/9.
- `./engine/build/iggy_native_play --help` passed.
- Native SDL/Vulkan smoke passed with display access and preserved the scripted
  debug/final-state lines.
- `git diff --check` and `git diff --cached --check` passed.
- Guard checks found no new glTF/material/texture/descriptor/sampler/staging/
  device-local/session/app-shell/SDL/Vulkan policy.

## Boundaries Preserved

- One loaded text mesh is bound to one renderer-private NPC model slot only.
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

- `git merge-base --is-ancestor 178a0174 HEAD`
- `git diff --check`
- `git status --short --branch`
