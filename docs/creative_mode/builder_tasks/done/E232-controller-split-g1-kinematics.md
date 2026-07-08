# E232 - Controller Split G1: Kinematics

## Status

Done.

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

## Completion Brief

- Card moved to done: yes.
- Files changed:
  - `CMakeLists.txt`
  - `cmake/iggy3d_tests.cmake`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/ControllerKinematics.hpp`
  - `src/app/iggy3d/gameplay/ControllerKinematics.cpp`
  - `tests/unit/product_gameplay_controller_kinematics_tests.cpp`
  - this task card
- Helper API added:
  - `productManualFirstPersonDirection(...)`
  - `productManualFirstPersonMaxSpeedMetersPerSecond(...)`
  - `productManualFirstPersonMovementProfile(...)`
  - `productManualFirstPersonMoveDelta(...)`
  - `productManualFirstPersonDesiredVelocity(...)`
  - `moveProductHorizontalVelocityToward(...)`
  - `clampProductHorizontalVelocity(...)`
- Extraction summary:
  - Moved only the pure first-person direction, speed/profile, move-delta,
    desired-velocity, horizontal move-toward, and horizontal clamp math into
    `ControllerKinematics.*`.
  - `Controller.cpp` now includes `ControllerKinematics.hpp` and calls the new
    helper names from dash, traversal/wall-run direction, ground movement, and
    airborne movement paths.
  - The new helper depends only on math/vector and movement tuning types; it
    does not include or depend on session, window state, collision, or receipt
    stores.
- Tests added:
  - `product_gameplay_controller_kinematics_tests` covers yaw-forward mapping,
    yaw 90 mapping, diagonal normalization, zero-input forward fallback,
    sprint/walk speed/profile selection, move delta scaling, desired velocity,
    horizontal velocity move-toward, and horizontal clamp behavior.
- CMake updates:
  - Added `src/app/iggy3d/gameplay/ControllerKinematics.cpp` next to
    `Controller.cpp` in the `iggy3d` library source list.
  - Added `product_gameplay_controller_kinematics_tests` next to
    `product_gameplay_controller_tests`.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_kinematics_tests product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_kinematics_tests|product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 4/4.
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card was clean.
- Confirmation:
  - No `applyProductGameplayActions(...)` signature change.
  - No jump, dash policy, wall jump, wall run, traversal, collision query,
    target/outcome proof, command submission, reset/fall, receipt, save, CMake
    test-definition widening beyond the requested target, staging, commit,
    push, broad CTest, or window launch changes.
