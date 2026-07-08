# E233 - Controller Split G2: Movement Proof Writers

## Status

Ready.

## Objective

Continue the `Controller.cpp` split by extracting movement proof/debug writer
helpers into a controller-owned helper file. This should reduce controller size
without moving jump, dash, traversal, wall-run evaluation, command submission,
or target/outcome behavior.

## Context

E232 extracted pure kinematics into `ControllerKinematics.*`. The existing
`src/app/iggy3d/gameplay/MovementProof.*` is a read-side packet builder for HUD
and receipts; do not merge this writer slice into that file. Keep this as a
controller writer/helper seam for now.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerMovementProof.hpp`
- `src/app/iggy3d/gameplay/ControllerMovementProof.cpp`

Move only these helpers out of `src/app/iggy3d/gameplay/Controller.cpp`:

- `clearProductMovementDebug(...)`
- `productHorizontalMovementSpeedMetersPerSecond(...)`
- `updateProductMovementStateProof(...)`
- `recordProductMovementProfile(...)`
- `recordProductAirborneMovementDebug(...)`
- `recordProductLedgeFallMovementDebug(...)`
- `productMovementDebugChangedPosition(...)`
- `recordProductMovementDebug(...)`

The new helper may depend on:

- `ProductAppWindowState`
- `Session`
- `ControllerKinematics.hpp`
- runtime movement result/facts helpers already used by these moved functions

Use the new helper from `Controller.cpp`.

## Non-Scope

Do not move or reshape:

- `applyProductGameplayActions(...)`
- input intent sampling or phase orchestration
- jump submit/advance/timing/coyote/buffer/cut logic
- dash submit/cooldown/proof logic
- wall jump, wall run, traversal, or collision query logic
- `ProductWallRun*EvaluationResult` structs and wall-run publish helpers
- target/outcome proof logic
- command submission/tick logic
- reset-zone, fall, ledge-fall fallback policy
- existing `MovementProof.hpp/.cpp` packet builder
- receipt keys/order/values

Do not rename status strings, reason codes, HUD labels, receipt fields, or
movement state enum values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerMovementProof.cpp` to the `iggy3d`
  library source list near `Controller.cpp` / `ControllerKinematics.cpp`.

No new test target is required for this slice; existing focused behavior tests
are the guard.

## Required Greps

After the move:

```sh
rg -n "clearProductMovementDebug|productHorizontalMovementSpeedMetersPerSecond|updateProductMovementStateProof|recordProductMovementProfile|recordProductAirborneMovementDebug|recordProductLedgeFallMovementDebug|productMovementDebugChangedPosition|recordProductMovementDebug" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerMovementProof.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerMovementProof.cpp
```

Classify results:

- declarations/definitions should live in `ControllerMovementProof.*`.
- `Controller.cpp` should retain call sites only.

Also verify `MovementProof.hpp/.cpp` are unchanged unless compile fallout
requires an include-only repair.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_movement_debug_hud_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_movement_debug_hud_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if extracting these helpers requires behavior changes in movement state,
  wall-run state, jump/dash/traversal state, command submission, or receipt
  output.
- Stop if the move requires widening into wall-run publish helpers or
  traversal/jump proof helpers; those are later slices.
- Stop if focused gameplay controller or movement-debug HUD tests drift.

No stage, commit, push, broad CTest, or window launch.
