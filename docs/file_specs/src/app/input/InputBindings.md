# File Spec

Files: `src/app/input/InputBindings.hpp`, `src/app/input/InputBindings.cpp`

Verified at: `6b1418a6`

## Owns

- Default mapping from `NeutralInput` values to `InputAction` values.
- Binding scale values for axes/buttons.
- `InputDeviceKind` enum for generic input source categories.
- First-match `actionForInput(...)` lookup helper.

## Does Not Own

- Neutral input enum names, action identity metadata, action-state recording, keyboard/gamepad polling state, product controller action map rows, or menu/gameplay command execution.

## Reads

- Static binding vector returned by `defaultInputBindings()`.
- `NeutralInput` values from keyboard, mouse, gamepad, and automation input paths.

## Writes / Mutates

- No runtime mutation; default bindings are static data.

## Calls Out To / Wires Out To

- Used by keyboard, mouse, and gamepad polling to translate neutral device input into `InputAction`.
- Used by room editor tests for expected key/button mappings.

## Called By / Entry Points

- `actionForInput(...)` is called by `KeyboardInput.cpp`, `MouseInput.cpp`, `GamepadInput.cpp`, and tests.
- Grep proof: `rg -n "InputBinding|defaultInputBindings|actionForInput|NeutralInput|InputDeviceKind" src/app tests/unit`.

## Invariants

- First matching binding wins.
- Multiple bindings for the same neutral input are intentional only when callers handle one action path directly.
- Bindings are generic app input policy, not product controller routing table policy.
- Automation neutral inputs must stay explicit to keep smokes deterministic.

## Tests / Proof Commands

- `rg -n "menu_input_tests|room_editor_input_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "actionForInput|NeutralInput::Key|NeutralInput::Automation|NeutralInput::MouseLeft" tests/unit/room_editor_input_tests.cpp src/app/input`.

## Nearby Files Usually Not Touched

- `src/app/input/InputDeviceEvent.*` unless neutral input values change.
- `src/app/input/InputAction.*` unless action identities change.
- `src/app/input/KeyboardInput.*`, `src/app/input/MouseInput.*`, and `src/app/input/GamepadInput.*` unless polling behavior changes.

## Update When

- Default bindings, binding scales, lookup behavior, or input-device kind values change.

## Do Not Update When

- Only a product-specific controller action map changes under `src/app/iggy3d/input/`.
