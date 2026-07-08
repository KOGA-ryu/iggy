# E236 - Controller Split G5a: Wall Surface Queries

## Status

Done.

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

## Completion Brief

- Card moved to done: yes, after this brief was appended.
- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/ControllerWallQueries.hpp`
  - `src/app/iggy3d/gameplay/ControllerWallQueries.cpp`
  - `docs/creative_mode/builder_tasks/done/E236-controller-split-g5a-wall-surface-queries.md`
- Helper/API shape added:
  - `ControllerWallQueries.hpp/.cpp` exports `findWallJumpSurface(...)`, `findWallRunSurface(...)`, `wallRunSideName(...)`, `productMovementDebugAlongWall(...)`, `wallRunProofNormal(...)`, `wallRunTangentDirectionFromNormal(...)`, and `wallRunTangentDirection(...)`.
  - `actorBlockingSurface(...)`, `hasTraversalTag(...)`, `horizontalNormal(...)`, `isNearVerticalSurface(...)`, and `isNearWallRunSurface(...)` are file-local in `ControllerWallQueries.cpp`.
  - Wall-query-only constants moved into `ControllerWallQueries.cpp`: `kPi`, `kMovementStateDistanceEpsilonMeters`, `kWallRunSurfaceVerticalSlackMeters`, and `kWallRunAlongWallDotThreshold`.
- Controller migration:
  - `Controller.cpp` now includes `ControllerWallQueries.hpp` and retains call sites only for the exported helpers.
  - Wall-run evaluation/result structs, wall-run publish helpers, wall-jump mutation, traversal proof writing, jump/dash orchestration, command submission, reset/fall, target/outcome proof, movement proof, and active-room ownership were not moved.
- CMake:
  - Added `src/app/iggy3d/gameplay/ControllerWallQueries.cpp` beside the other controller split sources.
- Required grep classification:
  - Lower-level private helpers live only in `ControllerWallQueries.cpp`.
  - Exported helper declarations/definitions live in `ControllerWallQueries.*`.
  - `Controller.cpp` retains call sites only for exported helpers.
  - No moved helper remains defined in `Controller.cpp`.
  - Focused dependency grep over `ControllerWallQueries.*` returned no hits for `Session`, command submission, reset/fall, target/outcome proof, traversal execution, or active-room ownership.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed.
  - Required wall-query `rg` classification was run.
  - Focused dependency grep over `ControllerWallQueries.*` was run.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card passed.
- Concerns/deferred:
  - None. No stage, commit, push, broad CTest, or window launch was performed.
