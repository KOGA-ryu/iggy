# File Spec

Files: `src/app/input/MouseInput.hpp`, `src/app/input/MouseInput.cpp`

Verified at: `629833a0`

## Owns

- Mouse left-button edge state and click packet.
- SDL mouse click polling.
- SDL relative mouse gameplay axis polling.
- Conversion from `MouseClick` to click action.

## Does Not Own

- Menu or creative hit testing, click coordinate normalization, mouse capture policy, action routing, or gameplay camera command handling.

## Reads

- SDL mouse button and relative motion state when SDL is compiled.
- `MouseInputState.leftWasDown` for click edge detection.
- Mouse neutral input bindings through `actionForInput(...)`.

## Writes / Mutates

- Updates `MouseInputState.leftWasDown`.
- Records relative mouse actions into caller-owned `ActionState`.
- Returns `MouseClick` and click `InputAction` values.

## Calls Out To / Wires Out To

- `recordAction(...)` and `actionForInput(...)`.
- Used by product window input frame for menu, creative UI, viewport pick, and gameplay mouse paths.

## Called By / Entry Points

- `resolveProductWindowInputMouseClick(...)` calls `pollMouseClick(...)`.
- Product window input uses `mouseClickAction(...)` and `pollMouseGameplayActions(...)`.
- Grep proof: `rg -n "MouseInputState|MouseClick|pollMouseClick|pollMouseGameplayActions|mouseClickAction" src/app tests/unit`.

## Invariants

- Clicks are edge-triggered on left button down.
- Relative mouse deltas are scaled before recording look actions.
- Non-SDL builds return no click and record no gameplay mouse actions.
- Coordinate interpretation belongs to window/creative hit-test code, not this file.

## Tests / Proof Commands

- `rg -n "product_window_input_frame_tests|product_creative_ui_input_frame_tests|product_creative_pick_flow_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "mouseClickAction|pollMouseClick|pollMouseGameplayActions|MouseClick" src/app tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/InputFrame.*` unless click normalization or routing changes.
- `src/app/iggy3d/window/MouseCapturePolicy.*` unless capture policy changes.
- `src/app/input/InputBindings.*` unless mouse bindings change.

## Update When

- Mouse click packet fields, edge behavior, relative motion scaling, or click-action mapping changes.

## Do Not Update When

- Only downstream hit testing or command execution changes.
