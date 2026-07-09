# File Spec

File: `src/app/iggy3d/gameplay/MovementTuning.hpp`

Verified at: `08813cf2`

## Owns

- Product gameplay movement tuning descriptors and default tuning values.
- Movement tuning field metadata: stable field ids, names, labels, value pointers, scalar/toggle kind, min/max, and step.
- Product movement state descriptors and wall-run status descriptor lookup.

## Does Not Own

- Runtime movement algorithms, input command routing, movement proof copying, settings menu state, HUD drawing, or receipt serialization.

## Reads

- No runtime state; this is a header-only descriptor/default-value surface.
- Callers read the descriptor tables and default tuning object.

## Writes / Mutates

- `adjustProductGameplayMovementTuning(...)` mutates the passed `ProductGameplayMovementTuning` value.
- Other helpers return descriptors, names, labels, values, next/previous fields, and invert-look boolean without global mutation.

## Calls Out To / Wires Out To

- Uses `std::clamp` for scalar adjustment.
- Does not call runtime, app window, renderer, save, or receipt systems.

## Called By / Entry Points

- Settings/input frame paths, frame presenter settings rows, menu panels, receipts, gameplay movement proof, HUD tests, and gameplay controls smoke.
- Grep proof: `rg -n "productGameplayMovementTuning|ProductGameplayMovementState|ProductGameplayMovementTuningField" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Descriptor arrays must stay in enum order for index-based lookup.
- Unknown/out-of-range enum values fall back to index zero.
- Tuning field descriptors must point to valid members of `ProductGameplayMovementTuning`.
- Toggle fields flip between `0.0F` and `1.0F`; scalar fields clamp to descriptor min/max.
- Field names and labels are product-facing proof/UI strings and should remain stable unless callers/tests are updated.

## Tests / Proof Commands

- `rg -n "settings_menu_tests|product_window_input_frame_tests|product_gameplay_controls_smoke|product_movement_debug_hud_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "productGameplayMovementTuningFieldCount|productGameplayMovementTuningFieldName|adjustProductGameplayMovementTuning|ProductGameplayMovementStateDescriptor" tests/unit/settings_menu_tests.cpp tests/unit/product_window_input_frame_tests.cpp tests/unit/product_movement_debug_hud_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/movement/*` unless runtime movement contracts need matching tuning semantics.
- `src/app/iggy3d/window/InputFrame.*` unless field adjustment input handling changes.
- `src/app/iggy3d/view/MenuPanelsView.*` and `src/app/iggy3d/window/FramePresenter.*` unless settings display changes.

## Update When

- Movement tuning fields, defaults, min/max/step bounds, state descriptors, wall-run status descriptors, or adjustment semantics change.

## Do Not Update When

- Only runtime movement implementation, product proof copying, or renderer presentation changes without altering tuning descriptors or helper contracts.
