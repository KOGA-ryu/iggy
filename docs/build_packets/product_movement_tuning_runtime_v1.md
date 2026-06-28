# Product Movement Runtime Tuning v1

Use this before changing product first-person movement. The goal is to avoid
rediscovering the same paths when adding jump, sprint, dash, clamber, wall-jump,
fall/reset, or future stamina/ability tuning.

## Runtime Ownership

- Default constants live in
  `src/app/iggy3d/gameplay/MovementTuning.hpp`.
- The live, per-window editable copy lives on
  `ProductAppWindowState::gameplayMovementTuning` in
  `src/app/iggy3d/ReceiptBuilder.hpp`.
- Product gameplay reads the live copy from `ProductAppWindowState`, not directly
  from the constant, inside `src/app/iggy3d/gameplay/Controller.cpp`.
- Runtime movement/physics defaults remain separate. This tuning is product
  first-person feel, not a physics-kernel default.

## Settings UI

- Open Settings, choose the Gameplay tab.
- `Enter` cycles the selected movement parameter.
- `Left` and `Right` adjust the selected parameter immediately.
- SDL drawing for the compact slider rows is in
  `src/app/iggy3d/view/OpeningMenuView.cpp`.
- Input routing for the Gameplay settings controls is in
  `src/app/iggy3d/menu/ActionHandlers.cpp`.

## Current Tunable Fields

The descriptor table is `kProductGameplayMovementTuningFields` in
`MovementTuning.hpp`.

- `walk_speed_mps`
- `sprint_speed_mps`
- `jump_impulse_mps`
- `gravity_mps2`
- `dash_speed_mps`
- `dash_duration_s`
- `dash_cooldown_s`

Each descriptor owns the label, min, max, step, and pointer to the field. Add
new movement feel controls there first, then make the controller consume the
field from `window.gameplayMovementTuning`.

## Controller Consumption

`Controller.cpp` uses the live tuning for:

- walk/sprint profile speed and per-frame step distance;
- jump impulse and gravity integration;
- dash speed, duration, and cooldown;
- wall-jump probe/push/rise settings.

If a future movement feature needs a tunable value, thread it through
`ProductGameplayMovementTuning`, then read it from the `window` object at the
call site that actually applies movement.

## Receipt Proof

`src/app/iggy3d/ReceiptBuilder.cpp` emits the selected field and current values:

- `gameplay_movement_tuning_status`
- `gameplay_movement_tuning_selected_field`
- `gameplay_movement_tuning_walk_speed_mps`
- `gameplay_movement_tuning_sprint_speed_mps`
- `gameplay_movement_tuning_jump_impulse_mps`
- `gameplay_movement_tuning_gravity_mps2`
- `gameplay_movement_tuning_dash_speed_mps`
- `gameplay_movement_tuning_dash_duration_s`
- `gameplay_movement_tuning_dash_cooldown_s`

Movement-result receipts still prove the applied effect through the existing
`gameplay_movement_*`, `gameplay_jump_*`, and `gameplay_dash_*` fields.

## Tests To Start With

- `settings_menu_tests` covers descriptor names/order and editable Gameplay
  settings row policy.
- `product_window_input_frame_tests` covers live left/right/enter settings
  adjustment on `ProductAppWindowState`.
- `product_gameplay_controller_tests` covers movement and dash consuming the
  live tuning values.

For a new movement feature, add one model/controller unit proof and one product
receipt/no-window proof before adding visual polish.
