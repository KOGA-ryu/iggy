# E36: Product Creative Input Orchestrator Extraction

## Objective

Reduce the 460-line `processProductWindowInputFrame(...)` bottleneck by
extracting the Creative-document input orchestration into a coherent helper.

## Problem

The product Creative path inside `InputFrame.cpp` currently handles UI click
routing, manual Rebuild Room, downstream click suppression, viewport picking,
tool-key dispatch, pointer lifecycle, Move commit routing, Navigate fly mode,
document revision/stale recording, undo snapshot push, and RoomBake
auto-refresh in one corridor.

That makes every new Creative input feature risky because unrelated policies
are interleaved in the same function.

## Required Reads

- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/creative/bridge/InputFrame.hpp/.cpp`
- `src/app/iggy3d/creative/bridge/UiInputFrame.hpp/.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp/.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp/.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

## Scope

- Extract the Creative-document live-input subflow into a helper or adjacent
  bridge module with a request/result shape.
- Preserve order: UI command, manual rebuild, downstream click, viewport pick,
  tool input, Navigate fly, revision compare, undo snapshot, auto-refresh.
- Keep behavior identical.
- Keep normal gameplay/room-editor paths out of the new helper.

## Acceptance

- `processProductWindowInputFrame(...)` is materially shorter and delegates the
  Creative-document path through a named seam.
- Revision/stale, undo push suppression, and auto-refresh behavior remain pinned
  by focused tests.
- Tool-only, selection-only, no-change move, manual rebuild, create/delete, and
  undo scenarios keep existing receipt behavior.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_input_frame_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_input_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change live behavior while extracting.
- Do not add keyboard Undo/Redo or new commands.
- Do not move RoomBake policy into InputFrame.
- Do not widen to renderer/projection changes.

## Completion Brief

- Modified `src/app/iggy3d/window/InputFrame.cpp` only.
- Added a private `ProductCreativeDocumentInputOrchestrationRequest` /
  `ProductCreativeDocumentInputOrchestrationResult` seam and
  `processProductCreativeDocumentInputOrchestration(...)`.
- Moved the Creative-document live input corridor into that helper:
  Creative UI input, command routing, manual Rebuild Room refresh,
  downstream click suppression, viewport pick, tool-key/tool pointer input,
  Navigate fly input, document revision comparison, undo snapshot push,
  mutation-time RoomBake auto-refresh, undo mirrors, and interrupted pointer
  lifecycle reset.
- Kept room-editor/gameplay input local in `processProductWindowInputFrame(...)`.
  The main function now delegates the Creative path and consumes the helper
  `downstreamClick`/`pointerTarget` for the non-Creative path.
- No new commands, keyboard shortcuts, RoomBake policy, renderer/projection, or
  live-window work.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_input_frame_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_input_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused whitespace scan over `src/app/iggy3d/window/InputFrame.cpp` and this
  task card.
