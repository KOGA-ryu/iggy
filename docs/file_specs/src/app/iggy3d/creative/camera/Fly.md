# File Spec

Files: `src/app/iggy3d/creative/camera/Fly.hpp`, `src/app/iggy3d/creative/camera/Fly.cpp`

Verified at: `116e4a9e`

## Owns

- Product creative fly camera movement kernel.
- Creative fly config, input, and result packets.
- Axis sanitization, movement normalization, yaw-relative movement, sprint speed, and fixed-step integration.
- Result reason codes for disabled, invalid config, no input, and applied movement.

## Does Not Own

- Keyboard polling or action recording.
- Creative fly anchor storage.
- Window-level creative fly routing.
- Camera projection/framing or receipt emission.
- Creative document or runtime session state.

## Reads

- `ProductCreativeFlyConfig`.
- `ProductCreativeFlyInput`.
- Starting camera anchor position.

## Writes / Mutates

- Returns `ProductCreativeFlyResult`.
- Does not mutate window state, anchor state, input state, or creative document state.

## Calls Out To / Wires Out To

- Window input code calls `applyProductCreativeFlyInput(...)` and records integrated anchors.
- Keyboard creative fly action recording feeds the input axes.
- Creative fly tests and navigate-fly tests call the kernel directly.

## Called By / Entry Points

- `isValidProductCreativeFlyConfig(...)`.
- `applyProductCreativeFlyInput(...)`.
- Focused proof: `rg -n "ProductCreativeFlyConfig|applyProductCreativeFlyInput|creative_fly_" src/app tests`.

## Invariants

- Disabled config reports disabled and does not move.
- Invalid config, non-finite start position, or non-finite yaw reports invalid config.
- Input axes are clamped to the allowed range and non-finite axes become zero.
- Zero effective input reports no input and does not move.
- Movement is yaw-relative, normalized, and scaled by speed, sprint multiplier, and input step seconds.

## Tests / Proof Commands

- `rg -n "product_creative_fly_tests|product_creative_navigate_fly_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "creative_fly_applied|creative_fly_no_input|applyProductCreativeFlyInput" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/view/CreativeFlyAnchorStore.*` unless anchor ownership changes.
- `src/app/iggy3d/window/InputFrame.*` unless window routing or action mapping changes.
- `src/app/input/KeyboardInput.*` unless fly action sampling changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless camera anchor consumption changes.

## Update When

- Fly config fields, input axes, movement math, sprint policy, validation rules, or result reason codes change.

## Do Not Update When

- Only keyboard bindings, anchor persistence, projection camera consumption, or receipt fields change without changing this fly movement kernel.
