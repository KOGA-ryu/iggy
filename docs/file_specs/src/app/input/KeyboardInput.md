# File Spec

Files: `src/app/input/KeyboardInput.hpp`, `src/app/input/KeyboardInput.cpp`

Verified at: `629833a0`

## Owns

- Keyboard input edge-state packet.
- Keyboard menu action polling and record helpers.
- ASCII room paint glyph keyboard sampling.
- Gameplay, room-editor, creative tool, and creative fly keyboard action recording.

## Does Not Own

- Action accumulator semantics, action descriptors, menu/frontend routing, creative command execution, room editor mutation, or SDL window ownership.

## Reads

- SDL keyboard state when SDL is compiled.
- Synthetic sample packets in tests and non-SDL callers.
- Default bindings for menu keys through `actionForInput(...)`.

## Writes / Mutates

- Mutates `KeyboardInputState` held-key fields.
- Records logical actions into caller-owned `ActionState`.
- Returns menu actions, paint glyphs, and creative tool key press packets.

## Calls Out To / Wires Out To

- `actionForInput(...)` for menu key bindings.
- `recordAction(...)` for gameplay, editor, and creative fly actions.
- Used by product window input frame.

## Called By / Entry Points

- `InputFrame.cpp` polls menu, room editor, creative tools, and creative fly keyboard paths.
- Unit tests call record helpers directly.
- Grep proof: `rg -n "KeyboardInputState|recordKeyboardMenuAction|pollKeyboardMenuAction|recordKeyboardRoomEditorActions|recordKeyboardCreativeToolKeys|recordKeyboardCreativeFlyActions|recordKeyboardAsciiRoomPaintGlyph" src/app tests/unit`.

## Invariants

- Menu and editor record helpers are edge-triggered through held-key state.
- Creative document tool keys are limited to tool selection and do not record gameplay actions.
- Creative fly records only the fly-relevant movement axes/buttons.
- Non-SDL builds must return no input and avoid side effects beyond unused parameters.

## Tests / Proof Commands

- `rg -n "room_editor_input_tests|product_window_input_frame_tests|product_creative_input_frame_tests|product_creative_navigate_fly_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "recordKeyboardMenuAction|recordKeyboardRoomEditorActions|recordKeyboardCreativeToolKeys|recordKeyboardCreativeFlyActions" tests/unit src/app/input`.

## Nearby Files Usually Not Touched

- `src/app/input/ActionState.*` unless action recording semantics change.
- `src/app/input/InputBindings.*` unless menu key bindings change.
- `src/app/iggy3d/window/InputFrame.*` unless frame polling order changes.

## Update When

- Keyboard state fields, sample packets, key mappings, edge-trigger rules, or recorded action sets change.

## Do Not Update When

- Only downstream command handlers change after actions are recorded.
