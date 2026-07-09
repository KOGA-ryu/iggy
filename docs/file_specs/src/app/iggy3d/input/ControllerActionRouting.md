# File Spec

Files: `src/app/iggy3d/input/ControllerActionRouting.hpp`, `src/app/iggy3d/input/ControllerActionRouting.cpp`

Verified at: `5da10b9d`

## Owns

- Per-frame gamepad controller-control to `InputAction` routing executor.
- Press/held state for controller controls through `ProductControllerActionRoutingState`.
- Routing result packet and skipped-routing result helper.
- Test/automation helpers that synthesize controller samples for a named control or mode chord.
- Recording of routing proof into `window.inputDevice.controllerAction`.

## Does Not Own

- The controller action map table, keyboard/mouse input, SDL gamepad polling, interaction-mode toggle policy, menu action handling, or gameplay command execution.

## Reads

- `ProductInputSurface`, `ProductInteractionMode`, `GamepadControllerActionSample`, controller routing state, and `ActionState`.
- Controller action map results from `mapProductControllerAction(...)`.

## Writes / Mutates

- Updates per-control held state.
- Records mapped actions into `ActionState` through `recordAction(...)`.
- Writes controller action proof fields on `ProductAppWindowState`.

## Calls Out To / Wires Out To

- `mapProductControllerAction(...)` for surface/mode/control mapping.
- `inputActionGroup(...)`, `inputActionName(...)`, `productControllerControlName(...)`, `productInteractionModeName(...)`, and `productInputSurfaceName(...)`.
- `recordAction(...)` to enqueue accepted app input actions.

## Called By / Entry Points

- `InputFrame.cpp` routes controller samples and records routing results.
- `AutomationGameplay.cpp` uses sample helpers for scripted controller input.
- Unit tests call routing, skip, sample, and proof-record helpers directly.
- Grep proof: `rg -n "recordProductControllerMappedActions|productControllerActionRoutingSkipped|productControllerActionSampleForControl|productControllerModeChordActionSample|recordProductControllerActionRoutingResult" src/app/iggy3d tests/unit`.

## Invariants

- Continuous player actions may repeat while held; non-continuous actions require a new press.
- Unmapped controls update held state but do not record actions.
- A mode chord may be represented as a sample helper but mode-toggle consumption is decided outside this file.
- Routing proof must describe the last routed or skipped controller action consistently.

## Tests / Proof Commands

- `rg -n "product_controller_action_routing_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "controller_action_mapped|controller_action_held|controller_action_chord_consumed|productControllerActionSampleForControl" tests/unit/product_controller_action_routing_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/input/ControllerActionMap.*` unless mapping table semantics change.
- `src/app/iggy3d/input/InteractionMode.*` unless mode/surface values change.
- `src/app/input/ActionState.*` unless action recording semantics change.
- `src/app/iggy3d/window/InputFrame.*` unless frame routing order changes.

## Update When

- Routing state, held/repeat policy, proof fields, sample helpers, or mapping executor behavior changes.

## Do Not Update When

- Only the controller mapping table changes without changing routing execution semantics.
