# 54 Native Product Session Extraction Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt product session orchestration was
extracted into an app-local session type.

## Integrated Commits

- `3e56b766 Extract native product session`
- `af97e351 Preserve native scripted load order`

## Integrated Surface

- `NativeProductSession.hpp/.cpp` defines app-local
  `iggy::native_play::NativeProductSession`.
- The session owns/delegates product load/play state, product input accumulator,
  active movement controls, latest product frame plus has flag, presentation
  camera plus has flag, scripted-control cadence/state, final dump state,
  movement guard, camera request config, and one-frame product request/tick flow.
- `IggyNativePlay.cpp` remains the app shell and renderer owner:
  `LaunchOptions`/CLI parsing/help, SDL key mapping/event loop/window lifecycle,
  Vulkan setup/swapchain/render pass/pipeline/shaders/command buffers/buffer
  upload/mesh ownership/destruction, and draw-list/camera orchestration stay
  there.
- CMake registers `apps/native_play/NativeProductSession.cpp` for
  `iggy_native_play`.
- `NativeProductSession.*` is app-local and non-SDL/non-Vulkan; it does not use
  renderer ownership terms.
- `NativeProductSessionConfig` carries raw scripted-control specs.
- The session constructor loads the product scenario first, then parses
  scripted-control specs into session-owned controls, preserving pre-extraction
  side-effect/error order.
- `ParseArgs` still uses the shared parser for existing
  `--expect-player-tiles` count validation only.
- Output/error strings and normal scripted final-state behavior are intended
  unchanged.

## Source Verification Facts

- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play`
  passed.
- Focused product CTest regex passed 10/10.
- Valid native smoke passed with `multi_frame_guard_room.toml`, scripted `east`,
  expected tile `5,1`, and final line containing
  `scripted final-state playerTile=5,1 nextFrameIndex=1 renderCommands=30 activeInputCount=0 heldInputCount=0`.
- Invalid-control smoke with `--scripted-controls nope` exits 1 and prints
  product load output before
  `iggy_native_play: unknown scripted control: nope`, preserving old
  load-before-parse ordering.

## Boundaries Preserved

- No runtime/product/scene/server/render-command API changes.
- No gameplay/input/scripted-control semantic changes.
- No CLI/debugger output string changes.
- No SDL extraction, CLI parse/help extraction, or
  `MapSdlKeyToProductControl` extraction.
- No Vulkan setup/swapchain/render pass/pipeline/command buffer/buffer upload/
  destruction extraction.
- No renderer class/skeleton extraction.
- No mesh-buffer ownership changes.
- No `NativePlayMath.hpp` or `NativeSceneDrawList.hpp` behavior changes.
- No glTF/assets/textures/material registry/animation/shader work.
- Renderer/swapchain/pipeline/command-buffer extraction and mesh-buffer
  ownership remain separate future packets.

## Verification

- `git merge-base --is-ancestor af97e351 HEAD`
- `git diff --check`
- `git status --short --branch`
