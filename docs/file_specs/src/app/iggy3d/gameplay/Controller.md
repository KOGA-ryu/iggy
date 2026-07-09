# File Spec

Files: `src/app/iggy3d/gameplay/Controller.hpp`, `src/app/iggy3d/gameplay/Controller.cpp`

Verified at: `dd59395f`

## Owns

- Product gameplay controller facade from app `ActionState` to gameplay action phases.
- One public entry point for live input, automation, scripted driver, and tests to apply gameplay actions.

## Does Not Own

- Input polling, action binding, per-phase gameplay command execution, movement algorithms, collision surface construction, session internals, HUD/receipt writing, or scripted tape parsing.

## Reads

- `ActionState` from app input.
- `Session`, `ProductAppWindowState`, source label, and optional collision surfaces passed by the caller.

## Writes / Mutates

- Mutates session/window only by delegating to action phases.
- Does not directly write gameplay proof fields, player transforms, command queues, collision surfaces, or receipts.

## Calls Out To / Wires Out To

- `sampleProductGameplayInputIntent(...)`.
- `applyProductGameplayActionPhases(...)`.

## Called By / Entry Points

- `applyProductGameplayActions(...)`.
- Called by window input frame, automation gameplay, scripted driver, active room tests, and gameplay controller tests.
- Grep proof: `rg -n "applyProductGameplayActions\\(" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Action sampling and phase execution remain separate seams.
- Callers provide collision surfaces when collision-aware gameplay is needed; this file does not fetch or bake them.
- Source labels pass through to phase handling for product proof/status context.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_active_room_collision_tests|product_window_input_frame_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "applyProductGameplayActions\\(" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_active_room_collision_tests.cpp src/app/iggy3d/window/InputFrame.cpp src/app/iggy3d/automation/AutomationGameplay.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ControllerActionPhases.*` unless phase ordering changes.
- `src/app/iggy3d/gameplay/ControllerInputIntent.*` unless input sampling changes.
- `src/app/iggy3d/window/InputFrame.*` unless live routing changes.

## Update When

- The public gameplay action facade, delegation order, source/collision-surface forwarding, or caller contract changes.

## Do Not Update When

- Only individual move/jump/dash/target phase behavior, input bindings, or receipt/HUD formatting changes without changing the facade contract.
