# File Spec

Files: `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`

Verified at: `0fe74f9c`

## Owns

- Receipt field emission for app identity, frontend screen/action/status, settings, window/input state, active surface, controller action state, and gameplay movement tuning fields.
- Ordered prelude, movement-tuning descriptor loop, and post-tuning receipt groups.

## Does Not Own

- Frontend routing or input handling.
- Settings mutation or persistence.
- Controller action mapping.
- Gameplay movement tuning ownership.
- Top-level receipt build order.

## Reads

- `ProductAppOptions`, `FrontendState`, `FrontendSettings`, `ProductAppWindowState`, active creative surface kind, map-maker live flag, input device state, frontend shell state, viewport state, and gameplay movement tuning descriptors.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate frontend, settings, window, input, or movement state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- `floatReceiptValue(...)`.
- Frontend, settings, renderer, input, and active-surface name helpers.
- `productGameplayMovementTuningFieldValue(...)`.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductFrontendSettingsWindowFields(...)` first in the product receipt sequence.
- Focused proof: `rg -n "appendProductFrontendSettingsWindowFields|frontend_screen|settings_look_sensitivity|controller_action_status|gameplay_movement_tuning_" src/app tests`.

## Invariants

- This appender is read-only over app/window state.
- Movement tuning receipt keys are generated from the tuning descriptor table.
- Frontend screen/action/status fields should remain early receipt fields.
- Active surface and legacy map-maker flags are passed in by the receipt builder, not recomputed here.

## Tests / Proof Commands

- `rg -n "product_receipt_key_order_tests|product_gameplay_controls_smoke|product_controller_input_smoke|product_menu_usefulness_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "frontend_screen|gameplay_movement_tuning_|controller_action_status" tests/unit tests/smoke src/app/iggy3d/receipt`.

## Nearby Files Usually Not Touched

- `src/app/frontend/*` unless frontend enum/name contracts change.
- `src/app/iggy3d/input/*` unless input proof fields change.
- `src/app/iggy3d/gameplay/MovementTuning.*` unless movement tuning descriptors change.

## Update When

- Frontend/settings/window receipt keys, source packets, movement tuning receipt generation, or active-surface receipt semantics change.

## Do Not Update When

- Only frontend UI drawing, input event polling, or movement behavior changes without changing emitted receipt fields.
