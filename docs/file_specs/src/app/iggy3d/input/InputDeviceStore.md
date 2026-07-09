# File Spec

File: `src/app/iggy3d/input/InputDeviceStore.hpp`

Verified at: `5da10b9d`

## Owns

- Product app input-device state packet embedded in `ProductAppWindowState`.
- Gamepad availability/name/mapping proof.
- Last accepted input action proof.
- Mouse capture, controller mode-toggle, controller action, interaction mode, and interaction mode HUD sub-packets.

## Does Not Own

- SDL/gamepad polling, keyboard/mouse input parsing, controller action mapping policy, interaction-mode toggle policy, mouse capture policy, or receipt field emission.

## Reads

- No live inputs; this header defines the product input-device packet.
- App/window/receipt paths read fields through `window.inputDevice`.

## Writes / Mutates

- Window input frame writes last action acceptance, interaction mode, controller action, and toggle proof.
- Mouse capture policy writes `mouseCapture`.
- Creative/world/menu transition paths set `interactionMode`.
- Tests write fields directly for focused input and receipt scenarios.

## Calls Out To / Wires Out To

- Embeds `ProductMouseCaptureState`, `ProductControllerModeToggleState`, `ProductControllerActionState`, `ProductInteractionMode`, and `InteractionModeHud`.
- Carries app input state into menu, creative, gameplay, renderer, and receipt paths.

## Called By / Entry Points

- Reached through `ProductAppWindowState.inputDevice`.
- Grep proof: `rg -n "InputDeviceStore|inputDevice\\.|lastInputAction|gamepadAvailable|mouseCapture|controllerModeToggle|controllerAction|interactionModeHud" src/app/iggy3d tests/unit`.

## Invariants

- This is a state packet only; behavior belongs in input/window policy files.
- `interactionMode` is product surface mode, not runtime gameplay state.
- Last-input fields are receipt/debug proof, not command history.
- New input facts should use the narrowest sub-packet when one exists.

## Tests / Proof Commands

- `rg -n "product_god_struct_ownership_coverage_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_controller_action_routing_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "inputDevice\\.interactionMode|inputDevice\\.lastInputAction|inputDevice\\.controllerAction|inputDevice\\.controllerModeToggle" src/app/iggy3d tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ProductAppWindowState.hpp` unless aggregate embedding changes.
- `src/app/iggy3d/input/InteractionMode.*` unless interaction mode semantics change.
- `src/app/iggy3d/window/InputFrame.*` unless input frame writes change.

## Update When

- Input-device fields, default values, embedded sub-packets, or field ownership rules change.

## Do Not Update When

- Only a writer implementation changes without changing the packet contract.
