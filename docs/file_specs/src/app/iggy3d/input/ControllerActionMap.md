# File Spec

Files: `src/app/iggy3d/input/ControllerActionMap.hpp`, `src/app/iggy3d/input/ControllerActionMap.cpp`

Verified at: `9da29fe8`

## Owns

- Product controller control enum and stable control names.
- Surface/mode/control to `InputAction` descriptor rows.
- Controller chord-reserved control classification.
- Lookup result for controller action mapping.

## Does Not Own

- Gamepad polling, held/repeat execution, action recording, interaction-mode toggle execution, keyboard/mouse bindings, or gameplay/editor command handling.

## Reads

- `ProductInputSurface`, `ProductInteractionMode`, and `ProductControllerControl` from mapping requests.
- Static controller control name, chord-component, and action-map tables.

## Writes / Mutates

- Returns `ProductControllerActionMapResult`.
- Does not mutate action state or app-window proof.

## Calls Out To / Wires Out To

- Uses `InputAction` values as target action identities.
- Consumed by `ControllerActionRouting.*` for per-frame routing execution.

## Called By / Entry Points

- `mapProductControllerAction(...)` is called by `recordProductControllerMappedActions(...)`.
- Control names are used by controller routing proof and tests.
- Grep proof: `rg -n "ProductControllerControl|ProductControllerActionMap|productControllerControlName|parseProductControllerControlName|productControllerControlIsModeChordComponent|productControllerActionMapRows|mapProductControllerAction" src/app/iggy3d tests/unit`.

## Invariants

- Mode-chord component controls are reserved from normal action mapping.
- Rows are data table policy; execution and held-state behavior belong in `ControllerActionRouting.*`.
- Control names are receipt/automation-facing strings and must remain stable unless callers and tests are updated.
- Mapping failure must return unmapped status without side effects.

## Tests / Proof Commands

- `rg -n "product_controller_action_map_tests|product_controller_action_routing_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "controller_action_chord_reserved|controller_action_mapped|parseProductControllerControlName|productControllerActionMapRows" tests/unit/product_controller_action_map_tests.cpp tests/unit/product_controller_action_routing_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/input/ControllerActionRouting.*` unless execution semantics change.
- `src/app/iggy3d/input/InteractionMode.*` unless surface or mode values change.
- `src/app/input/InputAction.*` unless action identities change.

## Update When

- Controller controls, stable names, chord-reserved controls, action-map rows, or map result semantics change.

## Do Not Update When

- Only routing held/repeat behavior changes in `ControllerActionRouting.*`.
