# File Spec

Files: `src/app/iggy3d/menu/InputRouter.hpp`, `src/app/iggy3d/menu/InputRouter.cpp`

Verified at: `6c79462b`

## Owns

- Product opening-menu input context that bundles frontend, saves, options, settings, active session, world setup draft, window, close flag, and optional creative app.
- Dispatch from resolved active product surface to the matching menu action handler.
- `productInputOwnerFor(...)` helper.
- Conversion of gameplay/editor `MenuBack` into `SystemPause`.

## Does Not Own

- Low-level input device polling.
- Input action classification policy in `app/input/InputRouter.*`.
- The actual behavior of starter, pause, load/save, settings, dev tools, new-world, delete-confirm, or system-pause actions.
- UI hit-region routing or mouse coordinate conversion.

## Reads

- `FrontendState` and `ProductAppWindowState` via `resolveProductActiveSurface(...)`.
- Input action and `ActionState`.
- Shared app state references stored in `ProductOpeningMenuInputContext`.

## Writes / Mutates

- Records accepted input into `ActionState`.
- Updates `window.inputDevice.lastInputAction` and `lastInputAccepted`.
- Mutates state only through delegated action handlers.

## Calls Out To / Wires Out To

- `resolveProductActiveSurface(...)`.
- `routeInputAction(...)` in the generic input router.
- `applyProductSystemPauseMenuAction(...)` before surface-specific dispatch.
- Surface-specific action handlers in `ActionHandlers.cpp`.

## Called By / Entry Points

- `window/InputFrame.cpp` routes live menu input through `routeProductOpeningMenuInput(...)`.
- Automation dispatch can route scripted frontend input through the same seam.
- Grep proof: `rg -n "routeProductOpeningMenuInput|applyProductOpeningMenuAction|productInputOwnerFor" src tests cmake`.

## Invariants

- `InputAction::None` is ignored.
- System pause gets first chance before active-surface dispatch.
- Menu owner state passed to `routeInputAction(...)` must match the active surface owner.
- Gameplay/editor back opens system pause instead of being handled as a local menu back.
- This file dispatches; it should not grow screen-specific business logic.

## Tests / Proof Commands

- `rg -n "product_window_input_frame_tests|product_automation_dispatch_tests|product_starter_menu_action_tests" cmake tests`.
- `rg -n "routeProductOpeningMenuInput|applyProductOpeningMenuAction" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/FrontendRouter.*` unless active surface ownership changes.
- `src/app/iggy3d/menu/ActionHandlers.*` unless command behavior changes.
- `src/app/input/InputRouter.*` unless generic owner/action acceptance changes.

## Update When

- Dispatch routing, system-pause precedence, owner-state construction, or context payload changes.

## Do Not Update When

- Only a delegated action handler changes behavior behind the same dispatch call.
