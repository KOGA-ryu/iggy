# E234 - Controller Split G3: Ground Queries

## Status

Done.

## Objective

Continue the `Controller.cpp` split by extracting the pure-ish walkable-ground
surface query helpers into a controller-owned helper file. Keep reset, fall,
jump, dash, wall traversal, command submission, and target/outcome behavior in
`Controller.cpp`.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. After E233, `Controller.cpp` is still roughly 2116
lines. The next lowest-risk seam is the walkable-ground query cluster used by
jump landing, fall detection, and ledge-fall fallback.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerGroundQueries.hpp`
- `src/app/iggy3d/gameplay/ControllerGroundQueries.cpp`

Move only these helpers out of `src/app/iggy3d/gameplay/Controller.cpp`:

- `surfaceContainsXZ(...)`
- `walkableSurfaceHeightAt(...)`
- `findHighestWalkableGroundAtOrBelow(...)`
- `playerHasNearbyGround(...)`
- `findLowestWalkableFloorY(...)`

Suggested API shape:

- Keep `surfaceContainsXZ(...)` and `walkableSurfaceHeightAt(...)` file-local in
  `ControllerGroundQueries.cpp` unless compile fallout proves otherwise.
- Export:
  - `findHighestWalkableGroundAtOrBelow(...)`
  - `playerHasNearbyGround(...)`
  - `findLowestWalkableFloorY(...)`
- Provide a single controller-owned ground contact tolerance constant if needed
  by both `Controller.cpp` and `ControllerGroundQueries.cpp`; do not duplicate
  the `0.12F` policy silently.

Use the new helper from `Controller.cpp`.

## Non-Scope

Do not move or reshape:

- `setProductPlayerPosition(...)`
- `findRoomAnchorByKind(...)`
- `findResetZoneAt(...)`
- `recordProductGameplayReset(...)`
- `resetProductPlayerToSpawn(...)`
- `applyProductGameplayResetIfNeeded(...)`
- `beginProductFallIfUnsupported(...)`
- `applyProductLedgeFallMoveFallback(...)`
- `recordProductJumpPosition(...)`
- `clearProductJumpTiming(...)`
- jump, dash, wall jump, wall run, traversal, target/outcome, or command
  submission logic
- movement proof writer helpers from E233
- receipt keys/order/values

Do not rename status strings, reason codes, HUD labels, receipt fields, room
anchor kinds, traversal tags, or movement state enum values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerGroundQueries.cpp` to the `iggy3d`
  library source list near the other controller split files.

No new test target is required for this slice; existing focused behavior tests
are the guard.

## Required Greps

After the move:

```sh
rg -n "surfaceContainsXZ|walkableSurfaceHeightAt|findHighestWalkableGroundAtOrBelow|playerHasNearbyGround|findLowestWalkableFloorY" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerGroundQueries.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerGroundQueries.cpp
```

Classify results:

- file-local lower-level helpers should live only in
  `ControllerGroundQueries.cpp`.
- exported helper declarations/definitions should live in
  `ControllerGroundQueries.*`.
- `Controller.cpp` should retain call sites only for exported helpers.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if extracting these helpers requires moving reset/fall/jump policy into
  the new helper.
- Stop if compile fallout requires exposing room anchors, session mutation,
  jump proof helpers, or active-room ownership through the new ground-query
  header.
- Stop if focused gameplay controller tests expose any movement, landing, or
  reset/fall behavior drift.

No stage, commit, push, broad CTest, or window launch.

## Completion Brief

- Card moved to done: yes, after this brief was appended.
- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/ControllerGroundQueries.hpp`
  - `src/app/iggy3d/gameplay/ControllerGroundQueries.cpp`
  - `docs/creative_mode/builder_tasks/done/E234-controller-split-g3-ground-queries.md`
- Helper/API shape added:
  - `ControllerGroundQueries.hpp/.cpp` exports `findHighestWalkableGroundAtOrBelow(...)`, `playerHasNearbyGround(...)`, and `findLowestWalkableFloorY(...)`.
  - `surfaceContainsXZ(...)` and `walkableSurfaceHeightAt(...)` are file-local in `ControllerGroundQueries.cpp`.
  - `kProductGameplayGroundContactToleranceMeters` is the single shared controller-owned contact tolerance constant used by both `Controller.cpp` and `ControllerGroundQueries.cpp`.
- Controller migration:
  - `Controller.cpp` now includes `ControllerGroundQueries.hpp` and retains only call sites for the exported ground-query helpers.
  - Reset, fall, ledge fallback, jump, dash, wall traversal, command submission, target/outcome, room anchors, and receipt behavior were not moved.
- CMake:
  - Added `src/app/iggy3d/gameplay/ControllerGroundQueries.cpp` beside the other controller split sources.
- Required grep classification:
  - Lower-level `surfaceContainsXZ(...)` and `walkableSurfaceHeightAt(...)` definitions live only in `ControllerGroundQueries.cpp`.
  - Exported helper declarations/definitions live in `ControllerGroundQueries.*`.
  - `Controller.cpp` retains call sites only for the exported helpers.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed.
  - Required ground-query `rg` classification was run.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card passed.
- Concerns/deferred:
  - None. No stage, commit, push, broad CTest, or window launch was performed.
