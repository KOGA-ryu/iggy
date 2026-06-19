# 61 Native Procedural NPC Mesh Slot Binding Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt rendering added a procedural
non-cube NPC marker mesh and bound it to the NPC actor model slot.

## Integrated Commit

- `fa2e1374 Add native procedural NPC mesh`

## Integrated Surface

- `engine/apps/native_play/NativeStaticMeshAsset.hpp` adds
  `NativeNpcMarkerStaticMeshAsset()`.
- The NPC marker is an in-memory procedural tapered CPU mesh using the existing
  `NativeStaticMeshVertex` position/color shape and `std::uint16_t` indexed
  triangles.
- `engine/apps/native_play/NativeVulkanRenderer.cpp` creates a separate
  `npcMesh_` from that asset through the existing mesh resource upload path.
- `NativeVulkanModelSlot::NpcActor` now binds to `npcMesh_`.
- `Player` stays registered to the bean mesh, and `Floor`/`Wall` stay
  registered to the cube fallback.
- Vertex binding/attributes, shader files/interfaces, push constants, item
  tints, host-visible/coherent upload semantics, indexed draw with
  `VK_INDEX_TYPE_UINT16`, draw-item order, app shell, product session,
  CLI/debugger output, and public renderer API are unchanged.

## Source Verification Facts

- Source packet touched only `engine/apps/native_play/NativeStaticMeshAsset.hpp`
  and `engine/apps/native_play/NativeVulkanRenderer.cpp`.
- Native build passed.
- Focused product CTest regex passed 10/10.
- Focused render/resource CTest regex passed 9/9.
- `./engine/build/iggy_native_play --help` passed.
- Native SDL/Vulkan smoke passed with display access and preserved the scripted
  debug/final-state lines.
- `git diff --check` and `git diff --cached --check` passed.
- Guard checks found no loader/glTF/material/texture/descriptor/sampler/asset
  registry/catalog/staging/device-local/session/app-shell terms introduced by
  the packet.

## Boundaries Preserved

- No public renderer API changes.
- No `IggyNativePlay.cpp`, CMake, tests, shader, runtime/product/scene,
  draw-list, product session, or app-shell changes.
- No CLI/debugger output or gameplay/session/input/scripted-control changes.
- No loader, file IO, glTF/static model parsing, asset registry/catalog,
  materials/textures/descriptors/samplers, staging/device-local upload, Linux/
  dGPU validation, backend abstraction, or package/authoring asset policy.

## Verification

- `git merge-base --is-ancestor fa2e1374 HEAD`
- `git diff --check`
- `git status --short --branch`
