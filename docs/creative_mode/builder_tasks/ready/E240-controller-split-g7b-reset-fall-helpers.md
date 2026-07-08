# E240 - Controller Split G7b: Reset Fall Helpers

## Status

Ready.

## Objective

Continue the `Controller.cpp` split by extracting the gameplay reset and
unsupported-ground fall helper cluster into a controller-owned helper file.
Keep jump advancement, wall-jump/traversal execution, command submission, and
target/outcome proof in `Controller.cpp`.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. E234 extracted `ControllerGroundQueries.*`. E235
extracted `ControllerJumpDashState.*`. E236 extracted
`ControllerWallQueries.*`. E237 extracted `ControllerWallRunEvaluation.*`. E238
extracted `ControllerTraversalProof.*`. E239 extracted
`ControllerPlayerAccess.*`.

After E239, `Controller.cpp` is roughly 1333 lines. The remaining reset/fall
cluster owns spawn/reset-zone anchor lookup, reset proof fields, fall-out
checks, and the transition into falling when the player no longer has nearby
ground. That is cohesive enough to move as the next slice, now that player
lookup and position mutation have a stable helper.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerResetFall.hpp`
- `src/app/iggy3d/gameplay/ControllerResetFall.cpp`

Move this reset/fall cluster out of
`src/app/iggy3d/gameplay/Controller.cpp`:

- `kGameplayResetBelowLowestFloorMeters`
- `kGameplayResetZoneRadiusMeters`
- `kGameplayResetZoneVerticalToleranceMeters`
- `horizontalDistanceSquared(...)`
- `findRoomAnchorByKind(...)`
- `findResetZoneAt(...)`
- `recordProductGameplayReset(...)`
- `resetProductPlayerToSpawn(...)`
- `applyProductGameplayResetIfNeeded(...)`
- `beginProductFallIfUnsupported(...)`

Suggested API shape:

- Export only:
  - `resetProductPlayerToSpawn(Session&, ProductAppWindowState&, std::string_view, const RoomAnchorAsset*)`
  - `applyProductGameplayResetIfNeeded(Session&, ProductAppWindowState&, const SpatialSurfaceSet*)`
  - `beginProductFallIfUnsupported(Session&, ProductAppWindowState&, const SpatialSurfaceSet*)`
- Keep `horizontalDistanceSquared(...)`, anchor lookup helpers, reset constants,
  and `recordProductGameplayReset(...)` file-local in `ControllerResetFall.cpp`.
- The header may forward-declare `ProductAppWindowState`, `RoomAnchorAsset`,
  `Session`, and `SpatialSurfaceSet`, and include `<string_view>`.
- The implementation may depend on `ProductRoomStore`, `ControllerGroundQueries`,
  `ControllerJumpDashState`, `ControllerPlayerAccess`, `ProductAppWindowState`,
  `RoomAsset`, and `Session` as needed.
- Preserve current behavior exactly: reset-zone radius/tolerance, fall-out
  threshold, spawn anchor fallback id `"spawn"`, source anchor fallback id
  `"none"`, reset jump-state clearing, `playerPositionChanged`, coyote timer
  setup on falling, and `gameplay_jump_falling` reason code.

## Non-Scope

Do not move or reshape:

- `beginProductJumpArc(...)`
- `tryProductCoyoteJump(...)`
- `tryProductWallJump(...)`
- `tryProductTraversalJump(...)`
- `advanceProductJump(...)`
- `submitProductJump(...)`
- `submitProductDash(...)`
- `applyProductLedgeFallMoveFallback(...)`
- `submitProductGameplayCommand(...)`
- `queryProductGameplayTarget(...)`
- target/outcome proof, command submission, movement proof, traversal proof,
  wall-run evaluation, wall-surface queries, ground-query implementation, or
  player access implementation
- receipt keys/order/values

Do not rename status strings, reason codes, HUD labels, receipt fields, room
anchor kinds, traversal tags, command kinds, target query status names,
rejection reason names, or movement state enum values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerResetFall.cpp` to the `iggy3d`
  library source list near the other controller split files.

No new test target is required for this slice; existing focused gameplay tests
are the behavior guard.

## Required Greps

After the move:

```sh
rg -n "kGameplayReset|horizontalDistanceSquared|findRoomAnchorByKind|findResetZoneAt|recordProductGameplayReset|resetProductPlayerToSpawn|applyProductGameplayResetIfNeeded|beginProductFallIfUnsupported" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerResetFall.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerResetFall.cpp
```

Classify results:

- reset constants and lower-level helper definitions should live only in
  `ControllerResetFall.cpp`.
- exported reset/fall helper declarations should live in
  `ControllerResetFall.hpp`.
- exported reset/fall helper definitions should live in
  `ControllerResetFall.cpp`.
- `Controller.cpp` should retain call sites only for exported helpers.
- no moved helper definition should remain in `Controller.cpp`.

Run an additional focused dependency grep over `ControllerResetFall.*` and
confirm it does not reference traversal execution, wall-jump mutation,
wall-run evaluation, command submission, target proof, outcome proof, movement
proof, or ledge-fall command fallback.

Allowed dependencies in `ControllerResetFall.*` are reset/fall-specific window
state, active-room anchors, ground queries, jump timing/position proof, player
access, session/world state through the player access helper, and collision
surfaces passed into the exported functions.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if the extraction requires moving jump advancement, traversal execution,
  wall-jump mutation, wall-run evaluation, command submission, target/outcome
  proof, movement proof, or ledge-fall command fallback.
- Stop if reset/fall status strings, anchor fallback ids, jump-state clearing,
  coyote setup, or state-hash behavior changes.
- Stop if focused gameplay controller tests expose any reset, fall, movement,
  jump, traversal, or command behavior drift.

No stage, commit, push, broad CTest, or window launch.
