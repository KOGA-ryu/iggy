# File Spec

Files: `src/app/frontend/PauseMenu.hpp`, `src/app/frontend/PauseMenu.cpp`

Verified at: `d9665a0c`

## Owns

- Pause menu row model and pause context.
- Pause action row enablement, disabled reasons, and command names.
- Pause route decisions for resume, room editor entry/exit, save, load, settings, dev tools, return to title, and exit game.

## Does Not Own

- Product save execution, gameplay resume mutation, room editor implementation, UI drawing, input polling, or app/window state mutation.

## Reads

- `PauseMenuContext` facts: pause open, runtime/session/save availability, compatible save count, dev tools, active-room editability, and room-editor readiness.
- Selected `FrontendAction`.

## Writes / Mutates

- No mutation; returns `PauseMenuModel` and `FrontendRouteResult`.

## Calls Out To / Wires Out To

- Uses `pauseActionOrder()` from frontend state.
- Uses `makeIgnoredFrontendRouteResult(...)` and `makeAcceptedFrontendRouteResult(...)`.
- Product frontend/router, pause UI, draw-list, hit-test, and presenter paths consume the model.

## Called By / Entry Points

- `buildPauseMenuModel(...)`, `routePauseAction(...)`, and `pauseCommandName(...)`.
- Grep proof: `rg -n "buildPauseMenuModel|routePauseAction|pauseCommandName|PauseMenuContext|PauseMenuModel" src/app tests/unit`.

## Invariants

- Disabled rows must carry concrete disabled reasons.
- Save and save-and-exit require runtime session availability and writable save root.
- Load requires at least one compatible save.
- Resume/edit routes return to gameplay and do not request save/load transitions.
- Save/exit route results carry transition intent only; they do not perform the operation.

## Tests / Proof Commands

- `rg -n "pause_menu_tests|product_pause_ui_draw_list_tests|product_vulkan_pause_overlay_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "routePauseAction|pauseCommandName|buildPauseMenuModel" tests/unit/pause_menu_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/PauseUi.*` unless pause UI projection changes.
- `src/app/iggy3d/menu/FrontendRouter.*` unless route application changes.
- `src/app/iggy3d/save/*` unless save execution changes.

## Update When

- Pause row order, enablement, command names, route statuses, route targets, or transition intent changes.

## Do Not Update When

- Only pause menu drawing or product execution after route acceptance changes.
