# 53 Native Scene Draw List Extraction Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt renderer prep extracted a
backend-neutral app-local scene draw list.

## Integrated Commit

- `b5faab99 Extract native scene draw list`

## Integrated Surface

- `NativeSceneDrawList.hpp` defines `NativeSceneModelId`,
  `NativeSceneDrawItem`, `NativeSceneDrawListInput`, pure transform helpers, and
  `BuildNativeSceneDrawItems(...)` under `iggy::native_play`.
- Input is nullable `const runtime::RuntimeGameplayState *state` plus `seconds`.
- `IggyNativePlay.cpp` adapts `product_->play.state.loop.currentState` through
  `nativeSceneDrawState()` and calls
  `BuildNativeSceneDrawItems({ nativeSceneDrawState(), seconds })`.
- Vulkan command recording consumes the returned draw items as before.
- Null state or product state with no player keeps the fallback rotating player
  cube.
- Floor cubes are emitted for all map tiles in y/x order.
- Wall cubes are emitted only for non-walkable tiles using existing
  `tileAt(...)` behavior.
- Present modern NPC actors are emitted in registry order.
- Player is appended last.
- Transforms, tints, inclusion policy, and vector order are intended unchanged.
- The extraction is header-only, so no CMake change is included.

## Boundaries Preserved

- No Vulkan mesh-buffer ownership extraction.
- No renderer class, swapchain, render pass, pipeline, shader module, command
  pool, command buffer, descriptor, buffer upload, or destruction extraction.
- No runtime/product/scene/server/render-command API changes.
- No CLI/debugger/docs changes in the source packet.
- No SDL/input/scripted-control/free-play/gameplay stepping changes.
- No glTF/assets/textures/materials/animation/shader changes.
- No intended visual behavior changes.
- Vulkan mesh-buffer ownership remains a separate later packet.
- Renderer/swapchain/pipeline/command-buffer extraction remains separate from
  mesh-buffer ownership and from debugger CLI/output/docs changes.

## Verification

- `git merge-base --is-ancestor b5faab99 HEAD`
- `git diff --check`
- `git status --short --branch`
