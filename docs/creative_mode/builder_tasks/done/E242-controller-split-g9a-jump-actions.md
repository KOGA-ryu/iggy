# E242 - Controller Split G9a: Jump Actions

## Status

Ready.

## Objective

Continue the `Controller.cpp` split by extracting the gameplay jump action and
jump-advance cluster into a controller-owned helper file. Keep input-intent
sampling, dash handling, movement submission, target actions, reset action, and
command submission in `Controller.cpp`.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. E234 extracted `ControllerGroundQueries.*`. E235
extracted `ControllerJumpDashState.*`. E236 extracted
`ControllerWallQueries.*`. E237 extracted `ControllerWallRunEvaluation.*`. E238
extracted `ControllerTraversalProof.*`. E239 extracted
`ControllerPlayerAccess.*`. E240 extracted `ControllerResetFall.*`. E241
extracted `ControllerTargetOutcomeProof.*`.

After E241, `Controller.cpp` is roughly 954 lines. The jump-specific cluster is
still the largest behavior island near the top of the file: normal jumps,
coyote/buffered jumps, wall jumps, traversal jump attempts, vertical jump
advance, landing, and jump submit proof fields. This slice moves that cluster
without moving input orchestration or command submission.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerJumpActions.hpp`
- `src/app/iggy3d/gameplay/ControllerJumpActions.cpp`

Move this jump cluster out of `src/app/iggy3d/gameplay/Controller.cpp`:

- `beginProductJumpArc(...)`
- `tryProductCoyoteJump(...)`
- `tryProductWallJump(...)`
- `tryProductTraversalJump(...)`
- `advanceProductJump(...)`
- `submitProductJump(...)`

Suggested API shape:

- Export only:
  - `void advanceProductJump(Session&, ProductAppWindowState&, const SpatialSurfaceSet*)`
  - `void submitProductJump(Session&, ProductAppWindowState&, std::string_view source)`
- Keep `beginProductJumpArc(...)`, `tryProductCoyoteJump(...)`,
  `tryProductWallJump(...)`, and `tryProductTraversalJump(...)` file-local in
  `ControllerJumpActions.cpp`.
- The header may forward-declare `ProductAppWindowState`, `Session`, and
  `SpatialSurfaceSet`, and include `<string_view>`.
- The implementation may depend on active-room collision/state, ground queries,
  kinematics, jump/dash state helpers, player access, reset/fall helpers,
  traversal proof helpers, wall queries, wall-run proof publishing helpers,
  movement traversal execution, state hashing, and session/world state as
  needed by the moved jump code.
- Preserve current recursion/ordering exactly: accepted jump calls
  `advanceProductJump(...)` immediately, traversal jump is tried before
  wall jump, wall jump is tried before coyote/buffer/normal jump, buffered jump
  can fire on landing, reset/fall checks stay inside jump advancement, and
  `clearProductWallRunActiveProof(window, "wall_run_landed")` remains on
  landing.

## Non-Scope

Do not move or reshape:

- `updateProductJumpTimingPhase(...)`
- `sampleProductGameplayInputIntent(...)`
- `ProductGameplayInputIntent`
- `submitProductDash(...)`
- `horizontalVelocityActive(...)`
- `updateProductGroundMovementVelocity(...)`
- `submitProductAirborneMove(...)`
- `submitProductMove(...)`
- `submitProductTargetCommand(...)`
- `submitProductGameplayCommand(...)`
- `tickProductGameplayCommand(...)`
- `commandMovementHasPhysicsFrameStats(...)`
- `applyProductLedgeFallMoveFallback(...)`
- `applyProductDashPhase(...)`
- `updateProductRetainedHorizontalVelocityPhase(...)`
- target/outcome proof implementation, movement proof implementation,
  reset/fall implementation, ground-query implementation, wall-run evaluation,
  wall-surface query implementation, player access implementation, or
  active-room ownership
- receipt keys/order/values

Do not rename jump status/reason strings, traversal statuses, wall-run reason
strings, movement state enum values, command strings, HUD labels, or receipt
fields.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerJumpActions.cpp` to the `iggy3d`
  library source list near the other controller split files.

No new test target is required for this slice; existing focused gameplay tests
and receipt key-order oracle are the behavior guard.

## Required Greps

After the move:

```sh
rg -n "beginProductJumpArc|tryProductCoyoteJump|tryProductWallJump|tryProductTraversalJump|advanceProductJump|submitProductJump" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerJumpActions.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerJumpActions.cpp
```

Classify results:

- exported declarations should live in `ControllerJumpActions.hpp`.
- exported definitions should live in `ControllerJumpActions.cpp`.
- file-local jump helper definitions should live only in
  `ControllerJumpActions.cpp`.
- `Controller.cpp` should retain call sites only for `advanceProductJump(...)`
  and `submitProductJump(...)`.
- no moved helper definition should remain in `Controller.cpp`.

Run an additional focused dependency grep over `ControllerJumpActions.*` and
confirm it does not reference dash submit, move submit, target action submit,
gameplay command submission, session ticking, ledge-fall command fallback,
input-intent sampling, target/outcome proof implementation, or movement proof
implementation.

Allowed dependencies in `ControllerJumpActions.*` are jump-specific window
state, active-room collision/state, ground queries, reset/fall helpers, player
access, wall query/proof helpers, traversal intent execution/proof helpers,
kinematics, state hashing, and jump/dash state helpers.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if the extraction requires moving input-intent sampling, dash submit,
  movement submit, target submit, gameplay command submission, session ticking,
  ledge-fall command fallback, target/outcome proof implementation, or movement
  proof implementation.
- Stop if any jump/traversal/wall-run status strings, reason codes, state hash
  updates, immediate jump advancement, buffered landing jump behavior, or
  receipt output changes.
- Stop if focused gameplay controller tests or the receipt key-order oracle
  expose behavior drift.

No stage, commit, push, broad CTest, or window launch.

## Completion Brief

Files changed:

- `CMakeLists.txt`
- `src/app/iggy3d/gameplay/Controller.cpp`
- `src/app/iggy3d/gameplay/ControllerJumpActions.hpp`
- `src/app/iggy3d/gameplay/ControllerJumpActions.cpp`
- `docs/creative_mode/builder_tasks/done/E242-controller-split-g9a-jump-actions.md`

Exact APIs moved/added:

- Added `ControllerJumpActions.hpp/.cpp`.
- Exported `advanceProductJump(Session&, ProductAppWindowState&, const SpatialSurfaceSet*)`.
- Exported `submitProductJump(Session&, ProductAppWindowState&, std::string_view)`.

Exact file-local helpers moved:

- `beginProductJumpArc(...)`
- `tryProductCoyoteJump(...)`
- `tryProductWallJump(...)`
- `tryProductTraversalJump(...)`

Behavior preservation:

- Accepted jumps still call `advanceProductJump(...)` immediately.
- Traversal jump is still attempted before wall jump.
- Wall jump is still attempted before coyote, buffered, and normal jump handling.
- Buffered jump can still fire on landing.
- Reset/fall checks remain inside jump advancement.
- Landing still calls `clearProductWallRunActiveProof(window, "wall_run_landed")`.
- Jump, traversal, wall-run, and mutation status/reason strings were not changed.

Scope notes:

- `Controller.cpp` now includes `app/iggy3d/gameplay/ControllerJumpActions.hpp` and retains call sites only for `submitProductJump(...)` and `advanceProductJump(...)`.
- `CMakeLists.txt` now registers `src/app/iggy3d/gameplay/ControllerJumpActions.cpp` next to the other controller split files.
- Input-intent sampling, dash submit, movement submit, target submit, gameplay command submission, session ticking, ledge-fall command fallback, target/outcome proof implementation, movement proof implementation, receipt keys/order/values, staging, commit, push, broad CTest, and window launch were not changed.

Required grep classification:

- `rg -n "beginProductJumpArc|tryProductCoyoteJump|tryProductWallJump|tryProductTraversalJump|advanceProductJump|submitProductJump" ...` shows exported declarations in `ControllerJumpActions.hpp`.
- The same grep shows exported definitions and file-local helper definitions in `ControllerJumpActions.cpp`.
- The same grep shows `Controller.cpp` retains call sites only for `advanceProductJump(...)` and `submitProductJump(...)`.
- Focused dependency grep over `ControllerJumpActions.*` returned no forbidden dependency hits for dash submit, move submit, target action submit, gameplay command submission, session ticking, ledge-fall command fallback, input-intent sampling, target/outcome proof implementation, or movement proof implementation.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 3/3 tests.
- `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over touched files and this card passed.
