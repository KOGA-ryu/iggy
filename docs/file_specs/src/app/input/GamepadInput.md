# File Spec

Files: `src/app/input/GamepadInput.hpp`, `src/app/input/GamepadInput.cpp`

Verified at: `629833a0`

## Owns

- Generic gamepad state packet and SDL gamepad lifetime helpers.
- Menu gamepad polling.
- Controller action sample polling.
- Gameplay and room-editor gamepad action recording.

## Does Not Own

- Product controller action map tables, interaction-mode chord policy, action-state merge semantics, menu/frontend routing, or gameplay/editor command execution.

## Reads

- SDL gamepad subsystem, buttons, axes, and gamepad name when SDL is compiled.
- `GamepadMenuState` held-button fields for edge-triggered actions.
- Default bindings for menu buttons through `actionForInput(...)`.

## Writes / Mutates

- Opens/closes native SDL gamepad handle and mutates availability/name fields.
- Updates held-button state.
- Records gameplay and room-editor actions into caller-owned `ActionState`.
- Returns menu actions and controller action samples.

## Calls Out To / Wires Out To

- `actionForInput(...)` for menu controls.
- `recordAction(...)` for gameplay and editor actions.
- Product controller routing consumes `GamepadControllerActionSample`.

## Called By / Entry Points

- Product window input frame initializes, shuts down, polls menu actions, polls controller samples, and polls gameplay actions.
- Unit tests call room editor record helper directly.
- Grep proof: `rg -n "GamepadMenuState|initializeGamepadMenuState|shutdownGamepadMenuState|pollGamepadMenuAction|pollGamepadControllerActionSample|pollGamepadGameplayActions|recordGamepadRoomEditorActions" src/app tests/unit`.

## Invariants

- SDL subsystem is initialized at most once per state and shut down through the matching helper.
- Axis values use deadzone and clamp behavior before action recording.
- Menu and editor button actions are edge-triggered.
- Non-SDL builds return no input and do not mutate external systems.

## Tests / Proof Commands

- `rg -n "room_editor_input_tests|product_window_input_frame_tests|product_controller_action_routing_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "recordGamepadRoomEditorActions|pollGamepadControllerActionSample|pollGamepadMenuAction|GamepadControllerActionSample" src/app tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/input/ControllerActionRouting.*` unless product controller routing changes.
- `src/app/input/InputBindings.*` unless menu gamepad bindings change.
- `src/app/iggy3d/window/InputFrame.*` unless gamepad polling order changes.

## Update When

- Gamepad state fields, SDL lifetime behavior, sample packet fields, axis normalization, edge-trigger rules, or recorded action sets change.

## Do Not Update When

- Only product controller action mapping rows change.
