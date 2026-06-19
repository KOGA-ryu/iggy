# 58 Native Model Slot / Cube Fallback Registry Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt Vulkan rendering added a
renderer-private model slot registry over the existing cube fallback mesh.

## Integrated Commit

- `cb1d3d52 Add native Vulkan model slot registry`

## Integrated Surface

- Source change is limited to `engine/apps/native_play/NativeVulkanRenderer.cpp`.
- Renderer-private `NativeVulkanModelSlot` and `NativeVulkanModelRegistry` were
  added in the renderer implementation.
- `NativeSceneModelId` now maps to renderer-private model slots, then model
  slots resolve to a mesh binding.
- `createSceneMeshes()` still creates the existing cube mesh and registers
  `Floor`, `Wall`, `NpcActor`, and `Player` slots to that same cube fallback.
- `meshForSceneModel(...)` delegates through `ModelSlotForSceneModel(...)` and
  `meshForModelSlot(...)`.
- Draw-item iteration and vector order remain unchanged.
- Preserved behavior: same `NativeSceneModelId` surface, same mesh readiness
  behavior, same cube mesh, same draw parameters, same shader pipeline, same
  push constants, and same tint behavior.
- This is model-slot groundwork only; it does not add asset loading.

## Source Verification Facts

- Source reviewer confirmed a clean worktree at `cb1d3d52`.
- `git diff --name-only c44ccdc1..HEAD` showed one source file.
- `git diff --check c44ccdc1..HEAD` passed.
- Native build passed.
- Focused product CTest regex passed 10/10.
- Focused render/resource CTest regex passed 9/9.
- `./engine/build/iggy_native_play --help` passed.
- Builder native smoke passed with display access and preserved debug/final-state
  lines.

## Boundaries Preserved

- No public renderer API changes.
- No `IggyNativePlay.cpp`, `NativeVulkanRenderer.hpp`,
  `NativeSceneDrawList.hpp`, CMake, shader, runtime/product/scene API, product
  session, or test changes.
- No CLI/debugger output changes.
- No gameplay/product/session/input/scripted-control changes.
- No shader source/interface changes or new shaders.
- No descriptors, samplers, textures/materials/assets/glTF, asset loader, model
  file IO, package discovery, resource catalog, authoring asset policy, staging/
  device-local upload, Linux/dGPU validation, or backend abstraction.
- Real static mesh loading, model files, materials/textures, descriptor/sampler
  policy, asset package discovery, and authoring asset policy remain future
  gates.

## Verification

- `git merge-base --is-ancestor cb1d3d52 HEAD`
- `git diff --check`
- `git status --short --branch`
