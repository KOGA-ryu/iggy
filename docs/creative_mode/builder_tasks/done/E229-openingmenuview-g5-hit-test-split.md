# E229 - OpeningMenuView G5: Hit-Test Split

## Status

Done.

## Context

E224 preflighted the `OpeningMenuView.cpp` split. E225 extracted SDL draw
helpers into `SdlDraw.*`. E226 extracted scene/viewport primitive rendering
into `ScenePrimitiveView.*`. E227 extracted debug/HUD drawing into
`DebugHudView.*`. E228 extracted menu/panel rendering into `MenuPanelsView.*`.

After E228, current sizes are:

- `src/app/iggy3d/view/OpeningMenuView.cpp`: 481 lines
- `src/app/iggy3d/view/MenuPanelsView.cpp`: 499 lines
- `src/app/iggy3d/view/MenuPanelsView.hpp`: 58 lines

The remaining `OpeningMenuView.cpp` mixed domain is hit-test/action routing.
E224 explicitly recommended splitting `OpeningMenuHitTest` before the final
facade include cleanup.

## Objective

Extract opening-menu hit-test types and action-routing helpers from
`OpeningMenuView.hpp/.cpp` into `OpeningMenuHitTest.hpp/.cpp`, preserving
behavior and keeping draw-row action order in one shared place.

## Implementation Scope

Edit only:

- `CMakeLists.txt`
- `src/app/iggy3d/view/OpeningMenuView.hpp`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- new `src/app/iggy3d/view/OpeningMenuHitTest.hpp`
- new `src/app/iggy3d/view/OpeningMenuHitTest.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- this task card

Add `src/app/iggy3d/view/OpeningMenuHitTest.cpp` to the `iggy3d` library source
list near `OpeningMenuView.cpp`, `MenuPanelsView.cpp`, `DebugHudView.cpp`,
`ScenePrimitiveView.cpp`, and `SdlDraw.cpp`.

## Move Scope

Move these types and functions out of `OpeningMenuView.hpp/.cpp`:

- `OpeningMenuHitArea`
- `OpeningMenuHitTestResult`
- `openingMenuDetailSurfaceFor(...)`
- `openingMenuActionAt(...)`
- `uiRectContains(...)`
- `hitRegionActionAt(...)`
- `pauseMenuActionAt(...)`

Move or expose the shared starter/pause row-order helpers needed by both draw
and hit-test code:

- `usesPauseMenuRows(...)`
- `menuActionOrderForFrontend(...)`

Recommended public names in `OpeningMenuHitTest.hpp`:

- `bool openingMenuUsesPauseRows(const FrontendState& frontend);`
- `const std::vector<FrontendAction>& openingMenuActionOrderForFrontend(const FrontendState& frontend);`
- `ProductFrontendSurface openingMenuDetailSurfaceFor(const FrontendState& frontend);`
- `OpeningMenuHitTestResult openingMenuActionAt(const ProductUiDrawListRequest& request, float x, float y);`

`OpeningMenuView.cpp` should call the shared row helpers from the new header so
the visual row order and hit-test row order cannot drift.

Keep these helper details file-local in `OpeningMenuHitTest.cpp`:

- `uiRectContains(...)`
- `hitRegionActionAt(...)`
- `pauseMenuActionAt(...)`

## Required Header Shape

`OpeningMenuView.hpp` should keep only draw/facade declarations:

- `OpeningMenuViewState`
- `drawOpeningMenuView(...)`

`OpeningMenuHitTest.hpp` should own:

- `OpeningMenuHitArea`
- `OpeningMenuHitTestResult`
- the four public helpers listed above

Direct callers should include the narrow header they use:

- `src/app/iggy3d/window/InputFrame.cpp` should include
  `app/iggy3d/view/OpeningMenuHitTest.hpp` for hit types/functions.
- `tests/unit/product_window_input_frame_tests.cpp` should include
  `app/iggy3d/view/OpeningMenuHitTest.hpp` for direct hit-test assertions.
- `src/app/iggy3d/window/FramePresenter.cpp` should keep using
  `OpeningMenuView.hpp` for drawing and should not need the hit-test header.

`InputFrame.hpp` already forward-declares `OpeningMenuHitTestResult`; do not
force it to include the new header unless compile fallout proves it necessary.

## Required Behavior Preservation

Preserve exactly:

- pause-menu hit routing through `buildPauseMenuModel(...)`,
  `buildProductPauseUiDrawList(...)`, and `routeProductUiHit(...)`
- starter/pause row hit-band coordinates and row step
- `buildProductStarterUiDrawList(request)` based child-panel hit regions
- NewWorld create/back/previous/next hit rectangles
- LoadSave slot, load, delete, and back hit behavior
- DeleteConfirm confirm/back hit behavior
- StarterDevTools category/back hit behavior
- Settings tab/back hit behavior
- `openingMenuDetailSurfaceFor(...)` active-surface behavior
- draw menu row count and selected-row behavior in `drawOpeningMenuView(...)`

The extraction should be mechanical. Do not table-drive, normalize, or otherwise
change the hit-test logic in this slice.

## Non-Scope

Do not move or edit behavior in:

- `drawGameplayPanel(...)`
- `drawOpeningMenuView(...)` beyond calling the renamed/shared row helpers
- menu/panel drawing
- debug/HUD drawing
- scene primitive drawing
- SDL draw helpers
- pause-menu model or draw-list construction semantics
- input dispatch behavior after a hit result is returned
- CMake test definitions
- receipt golden files

Do not create:

- a new widget or hit-region routing layer
- new menu/panel models
- new tests unless compile fallout or a narrow include migration needs them

No staging, commit, push, broad CTest, or window launch.

## Required Greps

Run and report:

```sh
rg -n "OpeningMenuHitArea|OpeningMenuHitTestResult|openingMenuUsesPauseRows|openingMenuActionOrderForFrontend|openingMenuDetailSurfaceFor|openingMenuActionAt|uiRectContains|hitRegionActionAt|pauseMenuActionAt|drawOpeningMenuView" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuHitTest.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuHitTest.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/window/InputFrame.cpp /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp
rg -n "OpeningMenuHitTest\\.cpp|MenuPanelsView\\.cpp|DebugHudView\\.cpp|ScenePrimitiveView\\.cpp|SdlDraw\\.cpp|OpeningMenuView\\.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
git -C /Users/kogaryu/iggy3d diff --check
```

Expected classification:

- hit-test result types live only in `OpeningMenuHitTest.hpp`
- public hit-test functions live in `OpeningMenuHitTest.hpp/.cpp`
- `uiRectContains(...)`, `hitRegionActionAt(...)`, and `pauseMenuActionAt(...)`
  are file-local in `OpeningMenuHitTest.cpp`
- `OpeningMenuView.hpp` no longer exposes hit-test types or functions
- `OpeningMenuView.cpp` retains `drawGameplayPanel(...)` and
  `drawOpeningMenuView(...)`, and calls the shared row helper names only
- `InputFrame.cpp` and `product_window_input_frame_tests.cpp` include/use
  `OpeningMenuHitTest.hpp`
- `CMakeLists.txt` includes `OpeningMenuHitTest.cpp` near the other view split
  files

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

- the extraction requires changing any hit-test coordinate, row order, surface,
  action, or active-surface behavior
- drawing and hit-testing cannot share the same action-order helper cleanly
- `InputFrame.hpp` needs broad include or type reshaping beyond the existing
  forward declaration
- compile fallout expands outside the scoped source/test files and CMake
- receipt golden output changes
- any menu-panel, HUD, primitive, SDL helper, input dispatch, or facade behavior
  needs to move with the hit-test helper

## Completion Brief

When done, report:

- files changed
- exact helper/API shape created
- what stayed in `OpeningMenuView.hpp/.cpp`
- CMake source-list placement
- direct caller include updates
- required grep classification
- focused build/CTest result
- receipt golden diff result
- diff/whitespace checks
- confirmation that drawing behavior, input dispatch behavior after hit result,
  menu panels, debug HUD, scene primitives, SDL helpers, CMake test definitions,
  staging, commit, push, and window launch were not touched

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/view/OpeningMenuView.hpp`
  - `src/app/iggy3d/view/OpeningMenuView.cpp`
  - `src/app/iggy3d/view/OpeningMenuHitTest.hpp`
  - `src/app/iggy3d/view/OpeningMenuHitTest.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - this task card
- Exact helper/API shape created:
  - Added `OpeningMenuHitTest.hpp/.cpp` under `src/app/iggy3d/view/`.
  - `OpeningMenuHitTest.hpp` owns:
    - `OpeningMenuHitArea`
    - `OpeningMenuHitTestResult`
    - `openingMenuUsesPauseRows(const FrontendState&)`
    - `openingMenuActionOrderForFrontend(const FrontendState&)`
    - `openingMenuDetailSurfaceFor(const FrontendState&)`
    - `openingMenuActionAt(const ProductUiDrawListRequest&, float, float)`
  - `OpeningMenuHitTest.cpp` owns the moved hit-test implementation and keeps
    `uiRectContains(...)`, `hitRegionActionAt(...)`, and
    `pauseMenuActionAt(...)` file-local.
- What stayed in `OpeningMenuView.hpp/.cpp`:
  - `OpeningMenuView.hpp` now keeps only `OpeningMenuViewState` and
    `drawOpeningMenuView(...)`.
  - `OpeningMenuView.cpp` retains `menuTitleForFrontend(...)`,
    `roundedDegrees(...)`, `drawGameplayPanel(...)`, and
    `drawOpeningMenuView(...)`.
  - `OpeningMenuView.cpp` now calls
    `openingMenuUsesPauseRows(...)`,
    `openingMenuActionOrderForFrontend(...)`, and
    `openingMenuDetailSurfaceFor(...)` from `OpeningMenuHitTest.hpp`.
- CMake source-list placement:
  - `OpeningMenuHitTest.cpp` at line 87
  - `OpeningMenuView.cpp` at line 88
  - `MenuPanelsView.cpp` at line 89
  - `DebugHudView.cpp` at line 90
  - `ScenePrimitiveView.cpp` at line 91
  - `SdlDraw.cpp` at line 92
- Direct caller include updates:
  - `src/app/iggy3d/window/InputFrame.cpp` now includes
    `app/iggy3d/view/OpeningMenuHitTest.hpp`.
  - `tests/unit/product_window_input_frame_tests.cpp` now includes
    `app/iggy3d/view/OpeningMenuHitTest.hpp`.
  - `src/app/iggy3d/window/FramePresenter.cpp` remains on
    `app/iggy3d/view/OpeningMenuView.hpp`.
  - `src/app/iggy3d/window/InputFrame.hpp` stayed on its existing forward
    declaration and was not changed.
- Required grep classification:
  - Hit-test result types live only in `OpeningMenuHitTest.hpp`.
  - Public hit-test functions live in `OpeningMenuHitTest.hpp/.cpp`.
  - `uiRectContains(...)`, `hitRegionActionAt(...)`, and
    `pauseMenuActionAt(...)` are file-local in `OpeningMenuHitTest.cpp`.
  - `OpeningMenuView.hpp` no longer exposes hit-test types or functions.
  - `OpeningMenuView.cpp` retains `drawGameplayPanel(...)` and
    `drawOpeningMenuView(...)`, and calls the shared row helper names only.
  - `InputFrame.cpp` and `product_window_input_frame_tests.cpp` include/use
    `OpeningMenuHitTest.hpp`.
  - `CMakeLists.txt` includes `OpeningMenuHitTest.cpp` near the other view split
    files.
- Focused build result:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_starter_menu_action_tests product_menu_transitions_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10`
    passed.
- Focused CTest result:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_starter_menu_action_tests|product_menu_transitions_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure`
    passed, 6/6 tests.
- Receipt golden diff result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
    was empty.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card was
    clean.
- Confirmation:
  - Drawing behavior, input dispatch behavior after hit result, menu panels,
    debug HUD, scene primitives, SDL helpers, CMake test definitions, receipt
    golden, staging, commit, push, broad CTest, and window launch were not
    touched/performed.
