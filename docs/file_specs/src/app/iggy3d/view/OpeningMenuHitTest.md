# File Spec

Files: `src/app/iggy3d/view/OpeningMenuHitTest.hpp`, `src/app/iggy3d/view/OpeningMenuHitTest.cpp`

Verified at: `2b846f78`

## Owns

- Opening/pause menu mouse hit-test result model and hit-area classification.
- Mapping virtual coordinates to starter/pause rows, settings tabs, dev-tools categories, new-world buttons, load-save slots/buttons, and delete-confirm buttons.
- Shared row-order helpers for starter vs pause menu use.
- Bridge from generated UI draw-list hit regions into legacy opening menu hit-test results.

## Does Not Own

- Mouse polling or coordinate scaling.
- Dispatching hit results into frontend actions.
- Drawing menu panels.
- Frontend action handlers.
- Save/delete execution.

## Reads

- `FrontendState`, active frontend surface, starter/pause action orders, settings/dev-tools tab orders, and `ProductUiDrawListRequest`.
- UI hit regions produced by `buildProductStarterUiDrawList(...)` and pause UI hit routing.

## Writes / Mutates

- Returns `OpeningMenuHitTestResult`.
- Does not mutate frontend, window, save, input, or route state.

## Calls Out To / Wires Out To

- `resolveProductActiveSurface(...)`.
- `starterActionOrder(...)`, `pauseActionOrder(...)`, and `frontendPauseMenuOpen(...)`.
- `buildProductStarterUiDrawList(...)`.
- `buildPauseMenuModel(...)`, `buildProductPauseUiDrawList(...)`, and `routeProductUiHit(...)`.

## Called By / Entry Points

- `InputFrame.cpp` calls `openingMenuActionAt(...)` and dispatches the returned hit result.
- Tests construct `OpeningMenuHitTestResult` and exercise hit routing in `product_window_input_frame_tests`.
- Focused proof: `rg -n "openingMenuActionAt|OpeningMenuHitTestResult|dispatchProductOpeningMenuMouseHit" src/app tests/unit`.

## Invariants

- Pause menu hit testing uses the pause UI draw-list path before starter-style row checks.
- Hit regions for widgetized screens come from the same draw-list request shape used by frame drawing.
- Manual row/button rectangles remain virtual-coordinate policy for screens not fully widgetized.
- This file identifies hits; command execution stays in `InputFrame` and frontend/menu handlers.

## Tests / Proof Commands

- `rg -n "OpeningMenuHitTestResult|openingMenuActionAt|dispatchProductOpeningMenuMouseHit" tests/unit/product_window_input_frame_tests.cpp src/app`.
- `rg -n "product_window_input_frame_tests|product_starter_menu_action_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/view/OpeningMenuView.*` unless draw geometry changes.
- `src/app/iggy3d/menu/DrawList.*` unless widget hit regions change.
- `src/app/iggy3d/window/InputFrame.*` unless hit dispatch or coordinate scaling changes.
- `src/app/iggy3d/menu/PauseUi.*` unless pause hit-layer semantics change.

## Update When

- Menu hit areas, virtual rectangles, row order, widget hit-region routing, or active-surface hit policy changes.

## Do Not Update When

- Only drawn colors/text change without moving hit geometry or changing hit meaning.
