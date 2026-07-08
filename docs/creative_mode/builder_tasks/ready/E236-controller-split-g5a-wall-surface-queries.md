# E236 - Controller Split G5a: Wall Surface Queries

## Status

Ready.

## Objective

Continue the `Controller.cpp` split by extracting the wall-surface query and
wall-run direction helpers into a controller-owned helper file. Keep wall-run
evaluation, wall-jump application, traversal proof writing, jump orchestration,
and command submission in `Controller.cpp`.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. E234 extracted `ControllerGroundQueries.*`. E235
extracted `ControllerJumpDashState.*`.

`Controller.cpp` is now roughly 1934 lines. The remaining wall cluster still
contains lower-level collision-surface probes mixed with higher-level wall-run
evaluation and wall-jump mutation. This card moves only the lower-level
wall-surface and wall-direction helpers.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerWallQueries.hpp`
- `src/app/iggy3d/gameplay/ControllerWallQueries.cpp`

Move these helpers out of `src/app/iggy3d/gameplay/Controller.cpp`:

- `actorBlockingSurface(...)`
- `hasTraversalTag(...)`
- `horizontalNormal(...)`
- `isNearVerticalSurface(...)`
- `findWallJumpSurface(...)`
- `isNearWallRunSurface(...)`
- `findWallRunSurface(...)`
- `wallRunSideName(...)`
- `productMovementDebugAlongWall(...)`
- `wallRunProofNormal(...)`
- `wallRunTangentDirectionFromNormal(...)`
- `wallRunTangentDirection(...)`

Suggested API shape:

- Keep lower-level helpers file-local in `ControllerWallQueries.cpp`:
  - `actorBlockingSurface(...)`
  - `hasTraversalTag(...)`
  - `horizontalNormal(...)`
  - `isNearVerticalSurface(...)`
  - `isNearWallRunSurface(...)`
- Export only the helpers currently needed by `Controller.cpp`:
  - `findWallJumpSurface(...)`
  - `findWallRunSurface(...)`
  - `wallRunSideName(...)`
  - `productMovementDebugAlongWall(...)`
  - `wallRunProofNormal(...)`
  - `wallRunTangentDirectionFromNormal(...)`
  - `wallRunTangentDirection(...)`
- Move file-local constants used only by this helper cluster into the new
  source, such as the wall-run side/direction constants. Do not silently
  duplicate a constant that remains used by `Controller.cpp`.
- The new helper may depend on `ProductAppWindowState`,
  `ProductGameplayMovementTuning`, `SpatialSurfaceSet`, `CollisionSurfaceView`,
  `Vec3`, and `ControllerKinematics.hpp`.

## Non-Scope

Do not move or reshape:

- `ProductWallRunCandidateEvaluationResult`
- `ProductWallRunActiveEvaluationResult`
- `ProductWallRunEvaluationResult`
- `ProductWallRunEvaluationRequest`
- `productWallRunCandidateRejected(...)`
- `productWallRunActiveRejected(...)`
- `productWallRunActiveRecorded(...)`
- `publishProductWallRunCandidateEvaluation(...)`
- `publishProductWallRunActiveEvaluation(...)`
- `publishProductWallRunEvaluation(...)`
- `clearProductWallRunActiveProof(...)`
- `evaluateProductWallRunCandidate(...)`
- `evaluateProductWallRunActiveWithoutJumpExit(...)`
- `evaluateProductWallRun(...)`
- `recordProductWallJumpTraversalProof(...)`
- `tryProductWallJump(...)`
- `tryProductTraversalJump(...)`
- `advanceProductJump(...)`
- `submitProductJump(...)`
- `submitProductDash(...)`
- reset, fall, ledge fallback, command submission, target/outcome proof, or
  movement proof logic
- receipt keys/order/values

Do not rename status strings, reason codes, HUD labels, receipt fields, room
anchor kinds, traversal tags, wall-run side labels, or movement state enum
values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerWallQueries.cpp` to the `iggy3d`
  library source list near the other controller split files.

No new test target is required for this slice; existing focused gameplay tests
are the behavior guard.

## Required Greps

After the move:

```sh
rg -n "actorBlockingSurface|hasTraversalTag|horizontalNormal|isNearVerticalSurface|findWallJumpSurface|isNearWallRunSurface|findWallRunSurface|wallRunSideName|productMovementDebugAlongWall|wallRunProofNormal|wallRunTangentDirectionFromNormal|wallRunTangentDirection" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerWallQueries.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerWallQueries.cpp
```

Classify results:

- lower-level private helpers should live only in
  `ControllerWallQueries.cpp`.
- exported helper declarations/definitions should live in
  `ControllerWallQueries.*`.
- `Controller.cpp` should retain call sites only for exported helpers.
- no moved helper should remain defined in `Controller.cpp`.

Run an additional focused dependency grep over `ControllerWallQueries.*` and
confirm it does not reference `Session`, command submission, reset/fall,
target/outcome proof, traversal execution, or active-room ownership.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if the extraction requires moving wall-run evaluation or wall-jump
  mutation into the new helper.
- Stop if the new helper starts depending on `Session`, command submission,
  reset/fall, target/outcome proof, traversal execution, or active-room
  ownership.
- Stop if focused gameplay controller tests expose any wall-jump, wall-run,
  traversal, jump, dash, or movement behavior drift.

No stage, commit, push, broad CTest, or window launch.
