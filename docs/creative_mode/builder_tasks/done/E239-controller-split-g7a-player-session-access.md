# E239 - Controller Split G7a: Player Session Access

## Status

Ready.

## Objective

Continue the `Controller.cpp` split by extracting the small player/session access
helpers used across reset, jump, traversal, movement, wall-run, and command
paths. Keep all behavior policy in `Controller.cpp`; this slice only gives
shared player lookup and position mutation a controller-owned helper file.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. E234 extracted `ControllerGroundQueries.*`. E235
extracted `ControllerJumpDashState.*`. E236 extracted
`ControllerWallQueries.*`. E237 extracted `ControllerWallRunEvaluation.*`. E238
extracted `ControllerTraversalProof.*`.

After E238, `Controller.cpp` is roughly 1359 lines. The remaining file-local
`productPlayerActor(...)`, `productPlayerEntity(...)`, and
`setProductPlayerPosition(...)` helpers are now shared plumbing for several
future seams. Moving them first reduces coupling before reset/fall, wall-jump,
jump advance, command submission, or target/outcome proof is split.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerPlayerAccess.hpp`
- `src/app/iggy3d/gameplay/ControllerPlayerAccess.cpp`

Move only these helpers out of `src/app/iggy3d/gameplay/Controller.cpp`:

- `productPlayerActor(const Session& session)`
- `productPlayerEntity(const Session& session)`
- `setProductPlayerPosition(Session& session, EntityId actor, const Vec3& position)`

Use the new helper from `Controller.cpp`.

Suggested API shape:

- Export all three moved helpers from `ControllerPlayerAccess.hpp`.
- The header may include `core/ids/EntityId.hpp` and `core/math/Vec3.hpp`, and
  may forward-declare `EntityState` and `Session`.
- The implementation may depend on `Session`, `SessionState`, `WorldState`,
  `Transform3`, `WorldMutationResult`, and `computeStateHash(...)`.
- Preserve the exact mutation behavior: find actor, copy existing transform,
  replace only `transform.position`, call `updateTransform(...)`, recompute
  `state.currentStateHash` only after a successful mutation, and return `false`
  for missing entity or failed world mutation.

## Non-Scope

Do not move or reshape:

- `queryProductGameplayTarget(...)`
- `findRoomAnchorByKind(...)`
- `findResetZoneAt(...)`
- `recordProductGameplayReset(...)`
- `resetProductPlayerToSpawn(...)`
- `applyProductGameplayResetIfNeeded(...)`
- `beginProductFallIfUnsupported(...)`
- `beginProductJumpArc(...)`
- `tryProductWallJump(...)`
- `tryProductTraversalJump(...)`
- `advanceProductJump(...)`
- `submitProductGameplayCommand(...)`
- target/outcome proof, movement proof, traversal proof, wall-run evaluation,
  wall-surface queries, ground queries, jump/dash state, command submission, or
  active-room ownership
- receipt keys/order/values

Do not rename status strings, reason codes, HUD labels, receipt fields, room
anchor kinds, traversal tags, command kinds, target query status names,
rejection reason names, or movement state enum values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerPlayerAccess.cpp` to the `iggy3d`
  library source list near the other controller split files.

No new test target is required for this slice; existing focused gameplay tests
are the behavior guard.

## Required Greps

After the move:

```sh
rg -n "productPlayerActor|productPlayerEntity|setProductPlayerPosition" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerPlayerAccess.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerPlayerAccess.cpp
```

Classify results:

- moved helper declarations/definitions should live in
  `ControllerPlayerAccess.*`.
- `Controller.cpp` should retain call sites only.
- no moved helper definition should remain in `Controller.cpp`.

Run an additional focused dependency grep over `ControllerPlayerAccess.*` and
confirm it does not reference `ProductAppWindowState`, `SpatialSurfaceSet`,
`activeRoom`, reset/fall helpers, jump/dash helpers, traversal execution, wall
query/evaluation helpers, command submission, target proof, outcome proof, or
movement proof.

Allowed dependencies in `ControllerPlayerAccess.*` are the session/world/hash
types needed to perform player lookup and transform mutation.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if the extraction requires moving reset/fall, jump, traversal, wall-run,
  command submission, target/outcome proof, or movement proof policy.
- Stop if the new helper needs `ProductAppWindowState`, active-room state, or
  collision surfaces.
- Stop if state hash recomputation behavior changes or focused gameplay
  controller tests expose any movement, jump, traversal, reset, or command
  behavior drift.

No stage, commit, push, broad CTest, or window launch.

## Completion Brief

Files changed:

- `CMakeLists.txt`
- `src/app/iggy3d/gameplay/Controller.cpp`
- `src/app/iggy3d/gameplay/ControllerPlayerAccess.hpp`
- `src/app/iggy3d/gameplay/ControllerPlayerAccess.cpp`
- `docs/creative_mode/builder_tasks/claimed/E239-controller-split-g7a-player-session-access.md`

Exact APIs moved/added:

- Added `ControllerPlayerAccess.hpp/.cpp`.
- Moved `productPlayerActor(const Session&)`.
- Moved `productPlayerEntity(const Session&)`.
- Moved `setProductPlayerPosition(Session&, EntityId, const Vec3&)`.

Behavior preservation:

- `productPlayerActor(...)` still returns `session.state().players.actorForSlot(0)`.
- `productPlayerEntity(...)` still resolves the actor through `session.state().world.findById(actor)`.
- `setProductPlayerPosition(...)` still mutates only the copied transform position, calls `updateTransform(...)`, returns `false` for missing entity or failed mutation, and recomputes `state.currentStateHash` only after successful mutation.

Scope notes:

- `Controller.cpp` now includes `app/iggy3d/gameplay/ControllerPlayerAccess.hpp` and retains call sites only for the moved helpers.
- `CMakeLists.txt` now registers `src/app/iggy3d/gameplay/ControllerPlayerAccess.cpp` next to the other controller split files.
- No reset/fall, jump, traversal, wall-run, command submission, target/outcome proof, movement proof, active-room ownership, receipt, staging, commit, push, broad CTest, or window launch changes were made.

Required grep classification:

- `rg -n "productPlayerActor|productPlayerEntity|setProductPlayerPosition" ...` shows declarations/definitions in `ControllerPlayerAccess.*`.
- The same grep shows `Controller.cpp` retains call sites only.
- Focused dependency grep over `ControllerPlayerAccess.*` returned no forbidden dependency hits for `ProductAppWindowState`, `SpatialSurfaceSet`, `activeRoom`, reset/fall, jump/dash, traversal execution, wall query/evaluation, command submission, target/outcome proof, or movement proof.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 3/3 tests.
- `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over touched files and this card passed.
