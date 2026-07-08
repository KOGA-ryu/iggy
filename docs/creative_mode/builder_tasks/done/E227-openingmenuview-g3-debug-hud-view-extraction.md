# E227 - OpeningMenuView G3: Debug HUD View Extraction

## Status

Done.

## Context

E224 preflighted the `OpeningMenuView.cpp` split. E225 extracted the SDL glyph
and draw helpers into `SdlDraw.*`. E226 extracted scene/viewport primitive
rendering into `ScenePrimitiveView.*`.

After E226, current sizes are:

- `src/app/iggy3d/view/OpeningMenuView.cpp`: 1162 lines
- `src/app/iggy3d/view/ScenePrimitiveView.cpp`: 400 lines
- `src/app/iggy3d/view/ScenePrimitiveView.hpp`: 20 lines

The next safe domain slice is the gameplay/debug HUD helper cluster. This card
should move HUD drawing helpers only; it must not touch menu panels, hit-test
routing, or facade behavior.

## Objective

Extract the current camera/feedback/debug/room-editor/position HUD drawing
helpers from `OpeningMenuView.cpp` into a new `DebugHudView` helper under
`src/app/iggy3d/view/`, preserving behavior.

## Implementation Scope

Edit only:

- `CMakeLists.txt`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- new `src/app/iggy3d/view/DebugHudView.hpp`
- new `src/app/iggy3d/view/DebugHudView.cpp`
- this task card

Add `src/app/iggy3d/view/DebugHudView.cpp` to the `iggy3d` library source list
near `OpeningMenuView.cpp`, `ScenePrimitiveView.cpp`, and `SdlDraw.cpp`.

## Move Scope

Move these current helpers out of `OpeningMenuView.cpp`:

- `drawCameraHeading(...)`
- `setFeedbackToneColor(...)`
- `drawGameplayFeedback(...)`
- `drawInteractionModeHud(...)`
- `drawMovementDebugHud(...)`
- `drawNpcBehaviorDebugHud(...)`
- `drawPhysicsDebugHud(...)`
- `drawRoomEditorHud(...)`
- `drawPositionHud(...)`

The new public header should expose only what `OpeningMenuView.cpp` needs:

- `void drawCameraHeading(SDL_Renderer&, float yawDegrees);`
- `void drawGameplayFeedback(SDL_Renderer&, const GameplayFeedback*);`
- `void drawInteractionModeHud(SDL_Renderer&, const InteractionModeHud*);`
- `void drawMovementDebugHud(SDL_Renderer&, const MovementDebugHud*);`
- `void drawNpcBehaviorDebugHud(SDL_Renderer&, const NpcBehaviorDebugHud*);`
- `void drawPhysicsDebugHud(SDL_Renderer&, const PhysicsDebugHud*);`
- `void drawRoomEditorHud(SDL_Renderer&, const ProductRoomEditorHud*);`
- `void drawPositionHud(SDL_Renderer&, const PositionHud*);`

Keep `setFeedbackToneColor(...)` file-local in `DebugHudView.cpp`.

## Explicit Deferral

Do **not** move `drawGameplayMovementTuningHud(...)` in this slice.

Reason: it currently depends on `fixedFloat(...)`, which is still shared with
menu/settings panel formatting. Moving it now would either duplicate formatting
logic or force a new formatting helper. Leave that for the menu-panel/tuning
slice.

Also leave `roundedDegrees(...)` in `OpeningMenuView.cpp` unless compiler
fallout proves a narrower need. The current facade still owns the yaw/pitch text
rows around `drawCameraHeading(...)`.

## Required Behavior Preservation

Preserve exactly:

- camera heading geometry and colors
- feedback tone color mapping
- gameplay feedback visibility, panel geometry, and row layout
- interaction-mode HUD visibility, panel geometry, and tone mapping
- movement/NPC/physics HUD row limits and layout
- room-editor HUD row limit and layout
- position HUD row limit and layout
- current call order inside `drawGameplayPanel(...)`

`OpeningMenuView.cpp` should include `app/iggy3d/view/DebugHudView.hpp` and keep
calling the moved helper names.

## Non-Scope

Do not move or edit behavior in:

- `drawGameplayMovementTuningHud(...)`
- `roundedDegrees(...)`, unless forced by compile fallout
- menu rows and panels
- settings/gameplay movement tuning rows
- `drawGameplayPanel(...)` orchestration beyond calling the moved helpers
- hit-test/action routing
- `drawOpeningMenuView(...)`
- `OpeningMenuView.hpp`
- `SdlDraw.*`
- `ScenePrimitiveView.*`
- CMake test definitions
- receipt golden files

Do not create:

- `MenuPanelsView`
- `OpeningMenuHitTest`
- a shared formatting helper

No staging, commit, push, broad CTest, or window launch.

## Required Greps

Run and report:

```sh
rg -n "drawCameraHeading|setFeedbackToneColor|drawGameplayFeedback|drawInteractionModeHud|drawMovementDebugHud|drawNpcBehaviorDebugHud|drawPhysicsDebugHud|drawRoomEditorHud|drawPositionHud|drawGameplayMovementTuningHud|roundedDegrees" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/DebugHudView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/DebugHudView.cpp
rg -n "DebugHudView\\.cpp|ScenePrimitiveView\\.cpp|SdlDraw\\.cpp|OpeningMenuView\\.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
git -C /Users/kogaryu/iggy3d diff --check
```

Expected classification:

- definitions for the moved HUD helpers live in `DebugHudView.cpp`
- `DebugHudView.hpp` exposes only the eight HUD draw functions listed above
- `setFeedbackToneColor(...)` is file-local in `DebugHudView.cpp`
- `OpeningMenuView.cpp` retains call sites only for moved helpers
- `drawGameplayMovementTuningHud(...)` and `roundedDegrees(...)` remain in
  `OpeningMenuView.cpp`
- `CMakeLists.txt` includes `DebugHudView.cpp` near the other view split files

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- the extraction requires changing any HUD geometry, colors, row caps, visibility
  checks, or call order
- moving `drawGameplayMovementTuningHud(...)` becomes necessary
- a new shared formatting helper becomes necessary
- compile fallout expands outside `OpeningMenuView.cpp`, `DebugHudView.*`, and
  CMake
- any menu panel, hit-test, receipt, scene primitive, SDL helper, or facade
  behavior needs to move with the HUD helper
- receipt golden output changes

## Completion Brief

When done, report:

- files changed
- exact helper/API shape created
- what stayed in `OpeningMenuView.cpp`
- CMake source-list placement
- required grep classification
- focused build/CTest result
- receipt golden diff result
- diff/whitespace checks
- confirmation that movement tuning, menu-panel, hit-test/facade policy,
  `OpeningMenuView.hpp`, `SdlDraw.*`, `ScenePrimitiveView.*`, CMake test
  definitions, staging, commit, push, and window launch were not touched

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/view/OpeningMenuView.cpp`
  - `src/app/iggy3d/view/DebugHudView.hpp`
  - `src/app/iggy3d/view/DebugHudView.cpp`
  - this task card
- Exact helper/API shape created:
  - Added `DebugHudView.hpp/.cpp` under `src/app/iggy3d/view/`.
  - `DebugHudView.hpp` is SDL3-gated, forward-declares `SDL_Renderer` and the
    HUD payload structs, and exposes only:
    - `drawCameraHeading(SDL_Renderer&, float)`
    - `drawGameplayFeedback(SDL_Renderer&, const GameplayFeedback*)`
    - `drawInteractionModeHud(SDL_Renderer&, const InteractionModeHud*)`
    - `drawMovementDebugHud(SDL_Renderer&, const MovementDebugHud*)`
    - `drawNpcBehaviorDebugHud(SDL_Renderer&, const NpcBehaviorDebugHud*)`
    - `drawPhysicsDebugHud(SDL_Renderer&, const PhysicsDebugHud*)`
    - `drawRoomEditorHud(SDL_Renderer&, const ProductRoomEditorHud*)`
    - `drawPositionHud(SDL_Renderer&, const PositionHud*)`
  - `DebugHudView.cpp` owns the moved implementations and keeps
    `setFeedbackToneColor(...)` file-local.
- What stayed in `OpeningMenuView.cpp`:
  - `drawGameplayMovementTuningHud(...)`
  - `roundedDegrees(...)`
  - menu rows and panels
  - `drawGameplayPanel(...)` orchestration and HUD call order
  - hit-test/action routing
  - `drawOpeningMenuView(...)`
- CMake source-list placement:
  - `OpeningMenuView.cpp` at line 87
  - `DebugHudView.cpp` at line 88
  - `ScenePrimitiveView.cpp` at line 89
  - `SdlDraw.cpp` at line 90
- Required grep classification:
  - Definitions for the moved HUD helpers live in `DebugHudView.cpp`.
  - `DebugHudView.hpp` exposes only the eight HUD draw functions listed above.
  - `setFeedbackToneColor(...)` is file-local in `DebugHudView.cpp`.
  - `OpeningMenuView.cpp` retains call sites only for moved helpers.
  - `drawGameplayMovementTuningHud(...)` and `roundedDegrees(...)` remain in
    `OpeningMenuView.cpp`.
  - `CMakeLists.txt` includes `DebugHudView.cpp` near the other view split
    files.
- Focused build result:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10`
    passed.
- Focused CTest result:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure`
    passed, 4/4 tests.
- Receipt golden diff result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
    was empty.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card was
    clean.
- Confirmation:
  - Movement tuning, menu-panel, hit-test/facade policy,
    `OpeningMenuView.hpp`, `SdlDraw.*`, `ScenePrimitiveView.*`, CMake test
    definitions, receipt golden, staging, commit, push, broad CTest, and window
    launch were not touched/performed.
