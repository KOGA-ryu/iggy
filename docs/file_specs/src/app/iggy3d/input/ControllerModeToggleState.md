# File Spec

File: `src/app/iggy3d/input/ControllerModeToggleState.hpp`

Verified at: `9da29fe8`

## Owns

- Controller interaction-mode toggle proof packet embedded in `InputDeviceStore`.
- Requested, accepted, status, reason code, and active surface proof for controller mode toggles.

## Does Not Own

- Pure interaction-mode toggle policy, active-surface resolution, gamepad chord polling, controller action routing, or receipt emission.

## Reads

- No live inputs; this header defines a packet.
- Receipt field appenders and tests read fields through `window.inputDevice.controllerModeToggle`.

## Writes / Mutates

- `applyProductInteractionModeFrameToggle(...)` writes all fields from a toggle result and resolved surface.
- Tests may write fields directly for receipt or transition scenarios.

## Calls Out To / Wires Out To

- No calls; embedded by `InputDeviceStore`.
- Emitted by `FrontendSettingsWindowFields.cpp` as controller mode-toggle receipt proof.

## Called By / Entry Points

- Reached through `ProductAppWindowState.inputDevice.controllerModeToggle`.
- Grep proof: `rg -n "ProductControllerModeToggleState|controllerModeToggle\\." src/app/iggy3d tests/unit`.

## Invariants

- This packet is proof state only; toggle decisions stay in `InteractionMode.*` and `InteractionModeState.*`.
- Requested and accepted must distinguish chord presence from accepted surface toggle.
- Surface is stored as a stable string for receipt/debug proof.

## Tests / Proof Commands

- `rg -n "product_interaction_mode_state_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "controllerModeToggle\\.requested|controllerModeToggle\\.accepted|controllerModeToggle\\.surface|interaction_mode_toggled" tests/unit/product_interaction_mode_state_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/input/InteractionModeState.*` unless writer semantics change.
- `src/app/iggy3d/input/InteractionMode.*` unless toggle statuses or policy change.
- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp` unless receipt fields change.

## Update When

- Controller mode-toggle proof fields, defaults, writer ownership, or receipt-facing meanings change.

## Do Not Update When

- Only the pure toggle policy changes without changing this proof packet.
