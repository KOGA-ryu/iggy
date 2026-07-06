# E62: Product Creative Input Phase Helpers

## Objective

Split the new Creative document input orchestration helper into smaller phase
helpers so new input features do not keep landing in a single 280+ line
function.

## Problem

E36 extracted the live Creative input corridor out of
`processProductWindowInputFrame(...)`, which was the right first step. The new
helper is still a 284-line policy corridor inside `InputFrame.cpp`, covering UI
click routing, command execution, manual rebuild, downstream click suppression,
viewport pick, pointer lifecycle, Move drag destination synthesis, Navigate fly
input, revision comparison, undo snapshot push, auto-refresh, undo mirrors, and
interrupted-pointer cleanup.

That is better than the old top-level function, but it is still hard to add a
new Creative input feature without touching unrelated policy.

## Required Reads

- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/creative/bridge/InputFrame.hpp/.cpp`
- `src/app/iggy3d/creative/bridge/UiInputFrame.hpp/.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp/.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp/.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

## Evidence

- `processProductCreativeDocumentInputOrchestration(...)` is currently
  `InputFrame.cpp:1006-1289`, about 284 lines.
- Pointer lifecycle and Move destination synthesis are embedded in the same
  function as UI command routing and bake-refresh policy.
- Revision compare, undo snapshot push, auto-refresh, and undo mirror recording
  are also embedded in that same function.

## Scope

- Extract named private phase helpers or a small adjacent bridge module for:
  - Creative UI command routing plus manual Rebuild Room refresh recording.
  - Viewport pick/downstream click handling.
  - Pointer lifecycle and Move destination synthesis.
  - Navigate fly handling.
  - Revision/stale/undo/auto-refresh finalization.
- Keep the same ordering and behavior.
- Keep `processProductWindowInputFrame(...)` as the owner of non-Creative
  gameplay/room-editor input.

## Acceptance

- `processProductCreativeDocumentInputOrchestration(...)` becomes a short
  ordered pipeline, not another long implementation body.
- Each extracted phase has a request/result shape that names its inputs and
  outputs instead of mutating the whole frame context implicitly.
- Existing behavior is preserved for:
  - tool-only frames,
  - selection-only frames,
  - no-change Move release,
  - manual Rebuild Room,
  - Create/Delete/Undo,
  - Navigate fly input,
  - interrupted pointer lifecycle cleanup.
- Focused tests still pass without weakening assertions.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_input_frame_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_input_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change routing behavior while extracting.
- Do not add keyboard Undo/Redo or new commands.
- Do not move RoomBake policy into lower-level Creative bridge code.
- Do not broaden to renderer/projection/product launch changes.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/window/InputFrame.cpp`
- Implementation:
  - Split `processProductCreativeDocumentInputOrchestration(...)` into a short
    ordered pipeline.
  - Added private phase helpers for:
    - revision/undo preimage capture;
    - Creative UI click/command routing plus manual Rebuild Room refresh;
    - downstream click and viewport pick routing;
    - pointer lifecycle and Move destination synthesis;
    - Navigate fly handling;
    - revision/stale/undo/auto-refresh finalization and interrupted-pointer
      cleanup.
  - Kept all behavior local to `InputFrame.cpp`; no lower-level bridge,
    RoomBake, launch, renderer, or command behavior was changed.
- Verification:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing whitespace scan over `InputFrame.cpp` passed.
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_input_frame_tests product_creative_world_launch_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_input_frame_tests|product_creative_world_launch_tests)$' --output-on-failure` passed.
- Concerns:
  - This intentionally stays as private helper extraction. A later slice can
    move stable phases into an adjacent bridge module if the APIs settle.
