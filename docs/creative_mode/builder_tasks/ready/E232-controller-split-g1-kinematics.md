# E232 - Controller Split G1: Kinematics

## Status

Ready.

## Objective

Start the `Controller.cpp` split with the smallest safe extraction: pure
first-person and horizontal-velocity kinematics. Preserve gameplay behavior
while giving later jump/dash/wall traversal slices a controller-owned helper
instead of keeping math buried in the 2428-line controller implementation.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerKinematics.hpp`
- `src/app/iggy3d/gameplay/ControllerKinematics.cpp`
- `tests/unit/product_gameplay_controller_kinematics_tests.cpp`

Move/rename only these pure helpers out of
`src/app/iggy3d/gameplay/Controller.cpp`:

- `manualFirstPersonDirection(...)`
- `manualFirstPersonMaxSpeedMetersPerSecond(...)`
- `manualFirstPersonMovementProfile(...)`
- `manualFirstPersonMoveDelta(...)`
- `manualFirstPersonDesiredVelocity(...)`
- `moveHorizontalVelocityToward(...)`
- `clampHorizontalVelocity(...)`

Suggested exported helper names:

- `productManualFirstPersonDirection(...)`
- `productManualFirstPersonMaxSpeedMetersPerSecond(...)`
- `productManualFirstPersonMovementProfile(...)`
- `productManualFirstPersonMoveDelta(...)`
- `productManualFirstPersonDesiredVelocity(...)`
- `moveProductHorizontalVelocityToward(...)`
- `clampProductHorizontalVelocity(...)`

Use the new helpers from `Controller.cpp`.

## Non-Scope

Do not move or reshape:

- `applyProductGameplayActions(...)`
- input intent sampling or phase orchestration
- jump submit/advance/timing/coyote/buffer/cut logic
- dash submit/cooldown/proof logic beyond calling the new direction helper
- wall jump, wall run, traversal, or collision query logic
- target/outcome proof logic
- command submission/tick logic
- reset-zone, fall, ledge-fall, or active-room collision logic
- receipt keys/order/values

Do not introduce dependencies from the new kinematics helper to:

- `Session`
- `ProductAppWindowState`
- `SpatialSurfaceSet`
- receipt/proof stores

The helper should depend only on math/vector and movement tuning types needed by
the moved pure functions.

## Required Tests

Add direct coverage in `product_gameplay_controller_kinematics_tests.cpp` for:

- yaw 0 forward direction maps to negative Z.
- yaw 90 forward direction maps to positive X.
- diagonal input normalizes to unit horizontal direction.
- zero input returns the camera forward direction.
- sprint/walk max speed and profile selection use the tuning fields.
- move delta scales by speed, input step, response multiplier, and normalized
  diagonal input.
- desired velocity returns zero for no movement and preserves double-axis
  normalized speed for movement.
- `moveProductHorizontalVelocityToward(...)` reaches the target when max delta
  is large enough, advances partially when smaller, and returns target on
  non-finite distance.
- `clampProductHorizontalVelocity(...)` preserves sub-limit velocity and clamps
  over-limit X/Z speed.

Keep existing behavior guards green:

- `product_gameplay_controller_tests`
- `product_active_room_collision_tests`
- `product_receipt_key_order_tests`

## CMake

- Add `src/app/iggy3d/gameplay/ControllerKinematics.cpp` to the `iggy3d`
  library source list near `Controller.cpp`.
- Add `product_gameplay_controller_kinematics_tests` to
  `cmake/iggy3d_tests.cmake` near `product_gameplay_controller_tests`.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_kinematics_tests product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_kinematics_tests|product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if extracting these helpers requires changing public controller behavior
  or the `applyProductGameplayActions(...)` signature.
- Stop if the new helper needs window/session/collision/proof dependencies.
- Stop if the focused gameplay controller tests expose any behavior drift.

No stage, commit, push, broad CTest, or window launch.
