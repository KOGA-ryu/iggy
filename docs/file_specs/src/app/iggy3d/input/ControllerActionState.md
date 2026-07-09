# File Spec

File: `src/app/iggy3d/input/ControllerActionState.hpp`

Verified at: `9da29fe8`

## Owns

- Controller action routing proof packet embedded in `InputDeviceStore`.
- Last controller routing mapped flag, status, reason code, control name, mode name, surface name, and input-action name.

## Does Not Own

- Controller action mapping, gamepad sample routing, action recording, interaction-mode toggle proof, or receipt emission.

## Reads

- No live inputs; this header defines a packet.
- Receipt field appenders and tests read fields through `window.inputDevice.controllerAction`.

## Writes / Mutates

- `recordProductControllerActionRoutingResult(...)` writes all fields from a routing result.
- Tests may write fields directly for receipt or input-frame scenarios.

## Calls Out To / Wires Out To

- No calls; embedded by `InputDeviceStore`.
- Emitted by `FrontendSettingsWindowFields.cpp` as controller routing receipt proof.

## Called By / Entry Points

- Reached through `ProductAppWindowState.inputDevice.controllerAction`.
- Grep proof: `rg -n "ProductControllerActionState|controllerAction\\." src/app/iggy3d tests/unit`.

## Invariants

- This packet is proof state only; it must not drive action execution.
- Names stored here are stable receipt strings, not enum values.
- Skipped routing may still write a proof packet with mapped false and a concrete status.

## Tests / Proof Commands

- `rg -n "product_controller_action_routing_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "controllerAction\\.mapped|controllerAction\\.status|controllerAction\\.inputAction" tests/unit/product_controller_action_routing_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/input/ControllerActionRouting.*` unless writer semantics change.
- `src/app/iggy3d/input/InputDeviceStore.hpp` unless embedding changes.
- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp` unless receipt fields change.

## Update When

- Controller action proof fields, defaults, writer ownership, or receipt-facing meanings change.

## Do Not Update When

- Only the controller action map table changes without changing proof fields.
