# File Spec

Files: `src/app/input/ActionState.hpp`, `src/app/input/ActionState.cpp`

Verified at: `6b1418a6`

## Owns

- Per-frame logical input action accumulator.
- Action entry merge semantics for down, pressed, released, and axis value.
- Query helpers for button and axis state.

## Does Not Own

- Physical device polling, input bindings, action descriptors, menu routing, gameplay/editor command execution, or product controller mapping policy.

## Reads

- `InputAction` identities.
- Existing `ActionState.entries` when merging or querying actions.

## Writes / Mutates

- `clearActionState(...)` clears all entries.
- `recordAction(...)` appends or merges action entries and ignores `InputAction::None`.
- Polling/routing callers mutate their caller-owned `ActionState`.

## Calls Out To / Wires Out To

- No external calls.
- Used by keyboard, mouse, gamepad, product controller routing, room editor actions, creative input, gameplay controller, and tests.

## Called By / Entry Points

- Generic polling and product input paths call `recordAction(...)` and query helpers.
- Grep proof: `rg -n "ActionState|ActionStateEntry|clearActionState|recordAction|actionIsDown|actionWasPressed|actionWasReleased|actionAxisValue" src/app tests/unit`.

## Invariants

- Recording `InputAction::None` must be a no-op.
- Duplicate actions merge into one entry.
- Boolean flags OR together; axis values accumulate.
- Query helpers return false or zero when an action is absent.

## Tests / Proof Commands

- `rg -n "product_controller_action_routing_tests|room_editor_input_tests|product_window_input_frame_tests|product_creative_navigate_fly_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "recordAction|actionAxisValue|actionWasPressed|actionWasReleased|clearActionState" src/app tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/input/InputAction.*` unless action identity APIs change.
- `src/app/input/KeyboardInput.*`, `src/app/input/MouseInput.*`, and `src/app/input/GamepadInput.*` unless pollers change.
- `src/app/iggy3d/input/ControllerActionRouting.*` unless product controller routing changes.

## Update When

- Action entry fields, merge semantics, query behavior, or no-op rules change.

## Do Not Update When

- Only bindings, poller mappings, or command handlers change without changing accumulator semantics.
