# File Spec

Files: `src/app/iggy3d/input/InteractionModeState.hpp`, `src/app/iggy3d/input/InteractionModeState.cpp`

Verified at: `5da10b9d`

## Owns

- Per-frame adapter between frontend/window state and pure interaction-mode toggle policy.
- Conversion from gamepad chord/action samples to `ProductControllerModeChordSample`.
- Active input surface resolution for interaction-mode decisions.
- Mutation of input-device interaction mode and mode-toggle receipt proof.

## Does Not Own

- Pure toggle rules, frontend active-surface rules, controller action mapping, SDL polling, creative launch behavior, or gameplay command execution.

## Reads

- `FrontendState`, `ProductAppWindowState`, gamepad chord/action samples, and prior controller chord state.
- Active surface from `resolveProductActiveSurface(productActiveSurfaceContextForWindow(...))`.

## Writes / Mutates

- Updates caller-owned chord state.
- Updates `window.inputDevice.interactionMode`.
- Updates `window.inputDevice.controllerModeToggle` requested, accepted, status, reason, and surface fields.

## Calls Out To / Wires Out To

- `productInputSurfaceFor(...)` calls `resolveProductActiveSurface(...)`.
- `applyProductInteractionModeFrameToggle(...)` delegates to `applyProductInteractionModeToggle(...)`.

## Called By / Entry Points

- `InputFrame.cpp` applies frame toggles during gamepad/controller input handling.
- Unit tests call conversion, surface resolution, and frame toggle helpers directly.
- Grep proof: `rg -n "applyProductInteractionModeFrameToggle|productInputSurfaceFor|productControllerModeChordSampleFromGamepad" src/app/iggy3d tests/unit`.

## Invariants

- This file may mutate app-window input proof; pure policy remains in `InteractionMode.*`.
- Surface must be resolved from current frontend/window state for each frame.
- Chord state must be written back even when no toggle is accepted.
- Mode-toggle proof strings must mirror the pure toggle result.

## Tests / Proof Commands

- `rg -n "product_interaction_mode_state_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "applyProductInteractionModeFrameToggle|productInputSurfaceFor|controllerModeToggle\\.surface|controllerModeToggle\\.status" tests/unit/product_interaction_mode_state_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/input/InteractionMode.*` unless pure policy changes.
- `src/app/iggy3d/menu/FrontendRouter.*` unless surface resolution changes.
- `src/app/iggy3d/window/InputFrame.*` unless frame invocation order changes.

## Update When

- Frame adapter mutation rules, active-surface inputs, gamepad sample conversion, or controller mode-toggle proof fields change.

## Do Not Update When

- Only pure toggle policy changes in `InteractionMode.*`.
