# File Spec

Files: `src/app/iggy3d/gameplay/ControllerInputIntent.hpp`, `src/app/iggy3d/gameplay/ControllerInputIntent.cpp`

Verified at: `dd59395f`

## Owns

- Product gameplay input intent packet sampled from app action state.
- Mapping from gameplay `InputAction` values to movement axes and button edge/down facts.
- Movement-intent predicate that treats retained horizontal velocity as active movement.

## Does Not Own

- Physical input polling, action binding tables, gameplay phase ordering, command execution, movement physics, or receipt/debug rendering.

## Reads

- `ActionState` axis/button state.
- `InputAction::PlayerMoveX`, `PlayerMoveY`, `PlayerSprint`, `PlayerJump`, `PlayerDash`, `PlayerInteract`, `PlayerAttack`, and `PlayerRetryOrReset`.
- `ProductAppWindowState` movement proof through `productHorizontalVelocityActive(...)`.

## Writes / Mutates

- Returns `ProductGameplayInputIntent`.
- Does not mutate action state, window state, session state, command queues, or collision surfaces.

## Calls Out To / Wires Out To

- `actionAxisValue(...)`, `actionIsDown(...)`, `actionWasPressed(...)`, and `actionWasReleased(...)`.
- `productHorizontalVelocityActive(...)`.

## Called By / Entry Points

- `sampleProductGameplayInputIntent(...)`.
- Called by `applyProductGameplayActions(...)`; intent is then consumed by action phases.
- Grep proof: `rg -n "sampleProductGameplayInputIntent|productGameplayIntentHasMovement|ProductGameplayInputIntent" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Movement axes are sampled as float axes, not button edges.
- Sprint is held-state; jump, dash, interact, attack, and reset are edge-triggered except jump release.
- Reset intent maps to `PlayerRetryOrReset`.
- Movement intent remains true while retained horizontal velocity is active.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_window_input_frame_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "PlayerMoveX|PlayerJump|PlayerDash|PlayerRetryOrReset|sampleProductGameplayInputIntent|ProductGameplayInputIntent" src/app/iggy3d tests/unit tests/smoke`.

## Nearby Files Usually Not Touched

- `src/app/input/*` unless action-state semantics change.
- `src/app/iggy3d/gameplay/ControllerActionPhases.*` unless intent consumption changes.
- `src/app/iggy3d/gameplay/ControllerMoveActions.*` unless movement-active predicate changes.

## Update When

- Gameplay input fields, input-action mapping, button edge/held semantics, or movement-intent predicate changes.

## Do Not Update When

- Only per-action gameplay execution, runtime movement, renderer/HUD, or receipt fields change without altering sampled intent.
