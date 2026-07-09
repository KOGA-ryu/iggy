# File Spec

Files: `src/app/input/InputDeviceEvent.hpp`, `src/app/input/InputDeviceEvent.cpp`

Verified at: `629833a0`

## Owns

- Neutral physical/automation input identity enum.
- Generic `InputDeviceEvent` packet.
- Stable neutral input string names.

## Does Not Own

- Binding neutral inputs to actions, physical device polling, action-state recording, product controller action maps, or menu/gameplay routing.

## Reads

- `NeutralInput` value passed to `neutralInputName(...)`.

## Writes / Mutates

- No mutation; returns string names.

## Calls Out To / Wires Out To

- Used by `InputBindings.*` as binding keys.
- Used by keyboard, mouse, gamepad, and automation input paths through binding lookup.

## Called By / Entry Points

- `neutralInputName(...)` is called by tests and debug/receipt-adjacent input assertions.
- Grep proof: `rg -n "NeutralInput|InputDeviceEvent|neutralInputName" src/app tests/unit`.

## Invariants

- Neutral inputs describe device-level facts, not product action semantics.
- Automation inputs remain explicit neutral values to keep scripted paths deterministic.
- Names are stable debug/test strings.
- Adding a neutral input requires binding and poller review.

## Tests / Proof Commands

- `rg -n "room_editor_input_tests|menu_input_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "neutralInputName|NeutralInput::" tests/unit src/app/input`.

## Nearby Files Usually Not Touched

- `src/app/input/InputBindings.*` unless bindings change.
- `src/app/input/KeyboardInput.*`, `src/app/input/MouseInput.*`, and `src/app/input/GamepadInput.*` unless pollers change.

## Update When

- Neutral input values, event packet fields, or neutral input names change.

## Do Not Update When

- Only action descriptors or product command handlers change.
