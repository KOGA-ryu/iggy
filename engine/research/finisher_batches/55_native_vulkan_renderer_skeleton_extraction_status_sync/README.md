# 55 Native Vulkan Renderer Skeleton Extraction Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt Vulkan renderer lifetime and draw
submission code moved into an app-local renderer skeleton.

## Integrated Commit

- `6cd702d2 Extract native Vulkan renderer skeleton`

## Integrated Surface

- `NativeVulkanRenderer.hpp/.cpp` defines app-local
  `iggy::native_play::NativeVulkanRenderer` with a minimal pimpl public surface.
- `NativeVulkanFrameInput` carries `Mat4 viewProjection` and a borrowed
  draw-item vector pointer.
- `NativeVulkanRenderer` exposes `initialize(SDL_Window *)`, `drawFrame(...)`,
  `markFramebufferResized()`, `aspectRatio()`, `waitIdle()`, and `cleanup()`.
- Vulkan lifetime/resources, swapchain, render pass, pipeline, depth,
  framebuffers, command pool, command buffers, sync, cube mesh, recording,
  acquire/submit/present, recreate, and cleanup code moved from
  `IggyNativePlay.cpp` to `NativeVulkanRenderer.cpp`.
- `IggyNativePlay.cpp` remains responsible for CLI/help/validation, MoltenVK
  fallback setup, SDL init/window/event loop/destruction/quit, SDL key mapping,
  product session calls, seconds/camera/draw-list orchestration, and per-frame
  renderer input.
- The renderer stores only a non-owning `SDL_Window *` for Vulkan interop and is
  not the SDL app shell.
- The renderer does not depend on `NativeProductSession`, runtime gameplay
  state, product loop/frame request, scripted controls, or
  `BuildNativeSceneDrawItems(...)`.
- Input lifetime is per-call: the app builds local draw items, passes a pointer
  through `NativeVulkanFrameInput`, and the renderer consumes it synchronously
  without storing it.
- The cube mesh moved mechanically into the renderer because command recording
  and draw submission moved; this is not a generalized mesh/resource
  abstraction.
- CMake registers `apps/native_play/NativeVulkanRenderer.cpp` for
  `iggy_native_play`.

## Boundaries Preserved

- No debugger CLI/output string changes.
- No SDL app-shell extraction beyond delegating renderer calls, window/resize,
  wait, and cleanup.
- The renderer does not call `SDL_Init`, `SDL_CreateWindow`, `SDL_PollEvent`,
  `SDL_DestroyWindow`, or `SDL_Quit`.
- No gameplay/product/session/input/scripted-control semantic changes.
- No runtime/product/scene/server/render-command API changes.
- No `NativeProductSession`, runtime gameplay state, product loop, product frame
  request, or scripted-control dependency in the renderer.
- No `NativePlayMath.hpp` or `NativeSceneDrawList.hpp` behavior changes.
- No shader behavior/interface changes or new shaders.
- No generalized mesh/resource registry; the cube mesh move is mechanical only.
- No glTF/assets/textures/material registry/animation work.
- No Linux/dGPU validation policy work.
- GPU mesh resource wrapper, app shell/CLI extraction, glTF/assets/textures/
  materials, animation, Linux/dGPU validation, and backend abstraction remain
  separate future packets.

## Verification

- `git merge-base --is-ancestor 6cd702d2 HEAD`
- `git diff --check`
- `git status --short --branch`
