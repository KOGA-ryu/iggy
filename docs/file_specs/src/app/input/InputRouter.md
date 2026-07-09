# File Spec

Files: `src/app/input/InputRouter.hpp`, `src/app/input/InputRouter.cpp`

Verified at: `629833a0`

## Owns

- Generic action acceptance gate by active menu owner and action group.
- `InputRoutingContext` and `InputRoutingResult` packets.
- Gameplay suppression flag derived from menu ownership.

## Does Not Own

- Menu owner selection rules, action identity metadata, physical input polling, product frontend routing, automation command behavior, or gameplay/editor command execution.

## Reads

- `MenuOwnerState` through `chooseMenuOwner(...)`.
- `InputAction` and `InputActionGroup` through `inputActionGroup(...)`.

## Writes / Mutates

- Returns `InputRoutingResult`.
- Does not mutate action state, frontend state, window state, or gameplay state.

## Calls Out To / Wires Out To

- `chooseMenuOwner(...)` and `menuOwnerBlocksGameplay(...)`.
- Used by product menu input router, product window input frame, and automation dispatch.

## Called By / Entry Points

- `routeInputAction(...)` is called from product input/menu/automation paths.
- Grep proof: `rg -n "InputRoutingContext|InputRoutingResult|routeInputAction|chooseMenuOwner|menuOwnerBlocksGameplay" src/app tests/unit`.

## Invariants

- System actions are accepted when non-none even if a menu owns input.
- Starter, pause, settings, and dev-tools owners accept menu-group actions only.
- Editor owner accepts editor and menu actions.
- Gameplay owner accepts player and system actions.
- None owner accepts no non-system action here.

## Tests / Proof Commands

- `rg -n "menu_input_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "routeInputAction|gameplaySuppressed|MenuOwner::" src/app tests/unit/product_window_input_frame_tests.cpp tests/unit/menu_input_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/frontend/MenuInput.*` unless owner selection changes.
- `src/app/input/InputAction.*` unless action groups change.
- `src/app/iggy3d/menu/InputRouter.*` unless product route integration changes.

## Update When

- Owner/action-group acceptance rules, routing result fields, or gameplay suppression contract changes.

## Do Not Update When

- Only product-specific handling after a route is accepted changes.
