# E230 - OpeningMenuView G6: Facade Include Cleanup

## Status

Ready.

## Context

E224 preflighted the `OpeningMenuView.cpp` split. E225 extracted SDL draw
helpers into `SdlDraw.*`. E226 extracted scene/viewport primitive rendering
into `ScenePrimitiveView.*`. E227 extracted debug/HUD drawing into
`DebugHudView.*`. E228 extracted menu/panel rendering into `MenuPanelsView.*`.
E229 extracted hit-test types/functions into `OpeningMenuHitTest.*`.

After E229, current sizes are:

- `src/app/iggy3d/view/OpeningMenuView.hpp`: 75 lines
- `src/app/iggy3d/view/OpeningMenuView.cpp`: 235 lines
- `src/app/iggy3d/view/OpeningMenuHitTest.hpp`: 55 lines
- `src/app/iggy3d/view/OpeningMenuHitTest.cpp`: 256 lines

The facade split is structurally complete. The remaining cleanup is include
hygiene: `OpeningMenuView.hpp` still includes many concrete payload headers
that can be forward-declared, and `OpeningMenuView.cpp` still carries includes
from helpers that have moved to child view files.

## Objective

Shrink `OpeningMenuView.hpp` to the minimal draw/facade surface and remove stale
includes from `OpeningMenuView.cpp`, preserving behavior.

## Implementation Scope

Edit only:

- `src/app/iggy3d/view/OpeningMenuView.hpp`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- direct compile fallout files if they need explicit includes after the facade
  header stops providing transitive dependencies
- this task card

Expected compile fallout should be small and likely limited to:

- `src/app/iggy3d/window/FramePresenter.cpp`

Do not edit CMake unless compile fallout proves a missing source registration,
which is not expected for this card.

## Cleanup Scope

In `OpeningMenuView.hpp`:

- Keep only standard includes needed by the declaration surface, such as
  `<cstddef>`, `<cstdint>`, and `<string>`.
- Keep concrete enum headers only where they are clearer/safer than opaque enum
  declarations:
  - `app/frontend/SettingsMenu.hpp` for `FrontendSettingsTab`
  - `app/iggy3d/gameplay/MovementTuning.hpp` for
    `ProductGameplayMovementTuningField`
- Forward-declare payload structs/classes that are only passed by reference or
  pointer:
  - `FrontendState`
  - `GameplayFeedback`
  - `InteractionModeHud`
  - `MovementDebugHud`
  - `NpcBehaviorDebugHud`
  - `PhysicsDebugHud`
  - `PositionHud`
  - `ProductAppOptions`
  - `ProductRoomEditorHud`
  - `ProductSaveBridgeResult`
  - `ProductViewportFrame`
  - `ProductWorldTemplate`
  - `TopDownMapOverlay`
  - `WorldSetupDraft`
  - `DebugProjectionResult`

In `OpeningMenuView.cpp`:

- Remove stale includes made unnecessary by E225-E229, such as split helper
  implementation headers no longer used directly in this file.
- Keep direct includes for every complete type or free function used in the
  remaining facade implementation.
- Do not rely on `OpeningMenuView.hpp` to pull in complete types needed by the
  `.cpp`; add explicit `.cpp` includes instead.

## Required Behavior Preservation

Preserve exactly:

- `OpeningMenuViewState`
- `drawOpeningMenuView(...)` signature
- gameplay panel drawing
- menu title and row drawing behavior
- child panel draw dispatch
- footer draw behavior
- receipt fields/order/values

This should be an include-only cleanup. No function body behavior should change
except for possible include ordering.

## Non-Scope

Do not move or edit behavior in:

- `drawGameplayPanel(...)`
- `drawOpeningMenuView(...)`
- `OpeningMenuHitTest.*`
- `MenuPanelsView.*`
- `DebugHudView.*`
- `ScenePrimitiveView.*`
- `SdlDraw.*`
- input dispatch
- hit-test routing
- menu/panel drawing
- CMake source lists
- receipt golden files

Do not create new files.

No staging, commit, push, broad CTest, or window launch.

## Required Greps

Run and report:

```sh
rg -n "#include" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/window/FramePresenter.cpp
rg -n "OpeningMenuHitArea|OpeningMenuHitTestResult|openingMenuActionAt|openingMenuDetailSurfaceFor" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp
git -C /Users/kogaryu/iggy3d diff --check
```

Expected classification:

- `OpeningMenuView.hpp` no longer includes hit-test, menu draw-list,
  primitive-draw-list, debug HUD payload, room-editor presentation, options,
  save bridge, world template, or viewport framing headers unless compile
  fallout proves one is truly required.
- `OpeningMenuView.hpp` still declares `OpeningMenuViewState` and
  `drawOpeningMenuView(...)`.
- `OpeningMenuView.hpp/.cpp` have no `OpeningMenuHitArea`,
  `OpeningMenuHitTestResult`, `openingMenuActionAt(...)`, or
  `openingMenuDetailSurfaceFor(...)` hits.
- `OpeningMenuView.cpp` keeps explicit direct includes for the remaining facade
  implementation dependencies.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_starter_menu_action_tests product_menu_transitions_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_starter_menu_action_tests|product_menu_transitions_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- cleanup requires changing any function signature or body behavior
- compile fallout expands into unrelated product subsystems
- CMake changes become necessary
- receipt golden output changes
- any hit-test, menu-panel, HUD, primitive, SDL helper, or input dispatch
  behavior needs to move or change

## Completion Brief

When done, report:

- files changed
- final `OpeningMenuView.hpp` include/forward-declare shape
- final `OpeningMenuView.cpp` direct include shape
- any direct compile fallout files given explicit includes
- required grep classification
- focused build/CTest result
- receipt golden diff result
- diff/whitespace checks
- confirmation that behavior, `OpeningMenuHitTest.*`, `MenuPanelsView.*`,
  `DebugHudView.*`, `ScenePrimitiveView.*`, `SdlDraw.*`, CMake source lists,
  staging, commit, push, and window launch were not touched
