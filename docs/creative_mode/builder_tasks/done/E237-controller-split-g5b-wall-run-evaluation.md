# E237 - Controller Split G5b: Wall Run Evaluation

## Status

Done.

## Objective

Continue the `Controller.cpp` split by extracting wall-run candidate/active
evaluation and wall-run proof publishing into a controller-owned helper file.
Keep wall-jump mutation, traversal execution, jump/dash orchestration,
reset/fall, command submission, and target/outcome proof in `Controller.cpp`.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. E234 extracted `ControllerGroundQueries.*`. E235
extracted `ControllerJumpDashState.*`. E236 extracted
`ControllerWallQueries.*`.

After E236, the wall-surface probes and wall-run direction helpers are no
longer in `Controller.cpp`. The next safe wall slice is the evaluation/publish
layer that consumes those helpers and writes only `gameplayWallRun` proof state.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp`
- `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.cpp`

Move these types/helpers out of `src/app/iggy3d/gameplay/Controller.cpp`:

- `ProductWallRunCandidateEvaluationResult`
- `ProductWallRunActiveEvaluationResult`
- `ProductWallRunEvaluationRequest`
- `ProductWallRunEvaluationResult`
- `productWallRunCandidateRejected(...)`
- `productWallRunActiveRejected(...)`
- `productWallRunActiveRecorded(...)`
- `publishProductWallRunCandidateEvaluation(...)`
- `publishProductWallRunActiveEvaluation(...)`
- `publishProductWallRunEvaluation(...)`
- `clearProductWallRunActiveProof(...)`
- `evaluateProductWallRunActiveWithoutJumpExit(...)`
- `evaluateProductWallRunCandidate(...)`
- `evaluateProductWallRun(...)`

Suggested API shape:

- Export the result/request structs plus:
  - `publishProductWallRunEvaluation(...)`
  - `clearProductWallRunActiveProof(...)`
  - `evaluateProductWallRun(...)`
- Keep rejected/recorded construction helpers and the candidate/active
  sub-evaluators file-local unless compile fallout proves otherwise.
- Avoid making the new helper depend on `Session`. Change
  `ProductWallRunEvaluationRequest` to carry an optional player position
  instead of `const Session&`. Preserve the current missing-player behavior by
  passing no player position from `Controller.cpp` when `productPlayerEntity(...)`
  returns null.
- The new helper may depend on `ProductAppWindowState`, `SpatialSurfaceSet`,
  `CollisionSurfaceView`, `Vec3`, `ControllerWallQueries.hpp`, and standard
  utilities.

## Non-Scope

Do not move or reshape:

- `productPlayerActor(...)`
- `productPlayerEntity(...)`
- `setProductPlayerPosition(...)`
- `recordProductWallJumpTraversalProof(...)`
- `tryProductWallJump(...)`
- `tryProductTraversalJump(...)`
- `advanceProductJump(...)`
- `submitProductJump(...)`
- `submitProductDash(...)`
- `submitProductGameplayCommand(...)`
- `resolveProductWallRunCandidatePhase(...)`
- `applyProductActiveMovementStatePhase(...)`
- `publishProductMovementProofPhase(...)`
- reset, fall, ledge fallback, command submission, target/outcome proof, or
  movement proof logic
- wall-surface query helpers from E236
- receipt keys/order/values

Do not rename status strings, reason codes, HUD labels, receipt fields, room
anchor kinds, traversal tags, wall-run side labels, or movement state enum
values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.cpp` to the `iggy3d`
  library source list near the other controller split files.

No new test target is required for this slice; existing focused gameplay tests
are the behavior guard.

## Required Greps

After the move:

```sh
rg -n "ProductWallRunCandidateEvaluationResult|ProductWallRunActiveEvaluationResult|ProductWallRunEvaluationRequest|ProductWallRunEvaluationResult|productWallRunCandidateRejected|productWallRunActiveRejected|productWallRunActiveRecorded|publishProductWallRunCandidateEvaluation|publishProductWallRunActiveEvaluation|publishProductWallRunEvaluation|clearProductWallRunActiveProof|evaluateProductWallRunActiveWithoutJumpExit|evaluateProductWallRunCandidate|evaluateProductWallRun" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerWallRunEvaluation.cpp
```

Classify results:

- file-local construction/sub-evaluation helpers should live only in
  `ControllerWallRunEvaluation.cpp`.
- exported request/result declarations and exported helper declarations should
  live in `ControllerWallRunEvaluation.hpp`.
- exported helper definitions should live in `ControllerWallRunEvaluation.cpp`.
- `Controller.cpp` should retain call sites only for exported helpers/types.
- no moved helper/type definition should remain in `Controller.cpp`.

Run an additional focused dependency grep over `ControllerWallRunEvaluation.*`
and confirm it does not reference `Session`, command submission, reset/fall,
target/outcome proof, traversal execution, wall-jump mutation, or active-room
ownership.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if the extraction requires moving wall-jump mutation or traversal
  execution into the new helper.
- Stop if the new helper starts depending on `Session`, command submission,
  reset/fall, target/outcome proof, traversal execution, wall-jump mutation, or
  active-room ownership.
- Stop if focused gameplay controller tests expose any wall-run, wall-jump,
  traversal, jump, dash, or movement behavior drift.

No stage, commit, push, broad CTest, or window launch.

## Completion Brief

- Card moved to done: yes, after this brief was appended.
- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp`
  - `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.cpp`
  - `docs/creative_mode/builder_tasks/done/E237-controller-split-g5b-wall-run-evaluation.md`
- Helper/API shape added:
  - `ControllerWallRunEvaluation.hpp/.cpp` owns `ProductWallRunCandidateEvaluationResult`, `ProductWallRunActiveEvaluationResult`, `ProductWallRunEvaluationRequest`, and `ProductWallRunEvaluationResult`.
  - Exported helper declarations/definitions are `publishProductWallRunEvaluation(...)`, `clearProductWallRunActiveProof(...)`, and `evaluateProductWallRun(...)`.
  - `productWallRunCandidateRejected(...)`, `productWallRunActiveRejected(...)`, `productWallRunActiveRecorded(...)`, `publishProductWallRunCandidateEvaluation(...)`, `publishProductWallRunActiveEvaluation(...)`, `evaluateProductWallRunActiveWithoutJumpExit(...)`, and `evaluateProductWallRunCandidate(...)` are file-local in `ControllerWallRunEvaluation.cpp`.
- Request shape change:
  - `ProductWallRunEvaluationRequest` now carries `std::optional<Vec3> playerPosition` instead of `const Session&`.
  - `Controller.cpp::resolveProductWallRunCandidatePhase(...)` still resolves the player entity locally and passes no player position when `productPlayerEntity(...)` returns null, preserving the existing `wall_run_missing_player` behavior.
- Controller migration:
  - `Controller.cpp` now includes `ControllerWallRunEvaluation.hpp` and retains only phase-level call sites/usages for the exported wall-run evaluation API.
  - Wall-jump mutation, traversal execution/proof writing, jump/dash orchestration, reset/fall, command submission, target/outcome proof, movement proof, and wall-surface query helpers were not moved.
- CMake:
  - Added `src/app/iggy3d/gameplay/ControllerWallRunEvaluation.cpp` beside the other controller split sources.
- Required grep classification:
  - File-local construction/sub-evaluation helpers live only in `ControllerWallRunEvaluation.cpp`.
  - Exported request/result declarations and exported helper declarations live in `ControllerWallRunEvaluation.hpp`.
  - Exported helper definitions live in `ControllerWallRunEvaluation.cpp`.
  - `Controller.cpp` retains call sites/usages only for exported helpers/types.
  - No moved helper/type definition remains in `Controller.cpp`.
  - Focused dependency grep over `ControllerWallRunEvaluation.*` returned no hits for `Session`, command submission, reset/fall, target/outcome proof, traversal execution, wall-jump mutation, or active-room ownership.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed.
  - Required wall-run evaluation `rg` classification was run.
  - Focused dependency grep over `ControllerWallRunEvaluation.*` was run.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card passed.
- Concerns/deferred:
  - None. No stage, commit, push, broad CTest, or window launch was performed.
