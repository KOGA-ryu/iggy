# File Spec

Files: `src/app/frontend/MenuInput.hpp`, `src/app/frontend/MenuInput.cpp`

Verified at: `a6f36636`

## Owns

- Frontend menu input action, source, and owner enums.
- Stable string names for menu input actions, sources, and owners.
- Menu owner priority selection from `MenuOwnerState`.
- Generic gameplay-blocking rule for menu owners.

## Does Not Own

- Physical input polling, `InputAction` routing, product active-surface resolution, frontend screen transitions, or command execution.

## Reads

- `MenuInputAction`, `MenuInputSource`, `MenuOwner`, and `MenuOwnerState` values.

## Writes / Mutates

- No mutation; returns names, selected owner, and gameplay-blocking boolean.

## Calls Out To / Wires Out To

- Used by generic `src/app/input/InputRouter.*`.
- Used by frontend route packets and product frontend/window surfaces for owner names and gameplay suppression.
- Receipt fields use owner names for active surface proof.

## Called By / Entry Points

- `chooseMenuOwner(...)` and `menuOwnerBlocksGameplay(...)` are called by input routing and frontend routing paths.
- Grep proof: `rg -n "menuInputActionName|menuInputSourceName|menuOwnerName|chooseMenuOwner|menuOwnerBlocksGameplay|MenuOwnerState" src/app tests/unit`.

## Invariants

- Owner priority is starter, pause, settings, dev-tools, editor, gameplay, none.
- Starter, pause, settings, dev-tools, and editor block gameplay input.
- Gameplay and none do not block gameplay through this generic rule.
- String names are receipt/debug-facing values and must stay stable unless downstream proof changes.

## Tests / Proof Commands

- `rg -n "menu_input_tests|product_frontend_router_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "chooseMenuOwner|menuOwnerBlocksGameplay|menuOwnerName" tests/unit/menu_input_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/input/InputRouter.*` unless action acceptance rules change.
- `src/app/frontend/FrontendRoute.*` unless route result ownership changes.
- `src/app/iggy3d/menu/FrontendRouter.*` unless product active-surface mapping changes.

## Update When

- Menu owner values, priority, names, or gameplay-blocking semantics change.

## Do Not Update When

- Only product-specific route behavior changes after the owner has been selected.
