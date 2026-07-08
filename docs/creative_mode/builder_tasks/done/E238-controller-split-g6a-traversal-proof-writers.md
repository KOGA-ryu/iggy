# E238 - Controller Split G6a: Traversal Proof Writers

## Status

Done.

## Objective

Continue the `Controller.cpp` split by extracting traversal proof writer helpers
into a controller-owned helper file. Keep traversal execution, wall-jump
mutation, jump orchestration, reset/fall, command submission, and
target/outcome proof in `Controller.cpp`.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. E234 extracted `ControllerGroundQueries.*`. E235
extracted `ControllerJumpDashState.*`. E236 extracted
`ControllerWallQueries.*`. E237 extracted `ControllerWallRunEvaluation.*`.

After E237, `Controller.cpp` is roughly 1440 lines. The remaining traversal
cluster still has proof-writing helpers next to traversal execution and
wall-jump mutation. This card moves only the proof writers.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerTraversalProof.hpp`
- `src/app/iggy3d/gameplay/ControllerTraversalProof.cpp`

Move only these helpers out of `src/app/iggy3d/gameplay/Controller.cpp`:

- `clearProductTraversalProof(...)`
- `recordProductTraversalProof(...)`
- `recordProductWallJumpTraversalProof(...)`

Use the new helper from `Controller.cpp`.

Suggested API shape:

- Export all three moved helpers from `ControllerTraversalProof.hpp`.
- The new helper may depend on `ProductAppWindowState`, `TraversalIntentResult`,
  `CollisionSurfaceView`, `Vec3`, and standard utilities.
- Keep traversal status/mechanic string mapping through the existing runtime
  movement traversal stringifiers. Do not introduce duplicate string tables.
- Keep `recordProductWallJumpTraversalProof(...)` as a proof writer only; do not
  move wall-jump movement/mutation policy with it.

## Non-Scope

Do not move or reshape:

- `tryProductTraversalJump(...)`
- `tryProductWallJump(...)`
- `beginProductJumpArc(...)`
- `advanceProductJump(...)`
- `submitProductJump(...)`
- `submitProductDash(...)`
- `submitProductGameplayCommand(...)`
- `setProductPlayerPosition(...)`
- `productPlayerActor(...)`
- `productPlayerEntity(...)`
- reset, fall, ledge fallback, wall-run evaluation, wall-surface queries,
  command submission, target/outcome proof, or movement proof logic
- receipt keys/order/values

Do not rename status strings, reason codes, HUD labels, receipt fields, room
anchor kinds, traversal tags, traversal mechanic names, or movement state enum
values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerTraversalProof.cpp` to the `iggy3d`
  library source list near the other controller split files.

No new test target is required for this slice; existing focused gameplay tests
are the behavior guard.

## Required Greps

After the move:

```sh
rg -n "clearProductTraversalProof|recordProductTraversalProof|recordProductWallJumpTraversalProof" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerTraversalProof.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerTraversalProof.cpp
```

Classify results:

- moved helper declarations/definitions should live in
  `ControllerTraversalProof.*`.
- `Controller.cpp` should retain call sites only.
- no moved helper definition should remain in `Controller.cpp`.

Run an additional focused dependency grep over `ControllerTraversalProof.*` and
confirm it does not reference `Session`, `executeTraversalIntent`, command
submission, reset/fall, wall-jump mutation, active-room ownership, target proof,
or outcome proof.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if the extraction requires moving traversal execution or wall-jump
  mutation into the new helper.
- Stop if the new helper starts depending on `Session`, command submission,
  reset/fall, active-room ownership, target proof, or outcome proof.
- Stop if focused gameplay controller tests expose any traversal, wall-jump,
  jump, dash, or movement behavior drift.

No stage, commit, push, broad CTest, or window launch.

## Completion Brief

- Card moved to done: yes, after this brief was appended.
- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/ControllerTraversalProof.hpp`
  - `src/app/iggy3d/gameplay/ControllerTraversalProof.cpp`
  - `docs/creative_mode/builder_tasks/done/E238-controller-split-g6a-traversal-proof-writers.md`
- Helper/API shape added:
  - `ControllerTraversalProof.hpp/.cpp` exports `clearProductTraversalProof(...)`, `recordProductTraversalProof(...)`, and `recordProductWallJumpTraversalProof(...)`.
  - The helper uses the existing runtime traversal stringifiers from `runtime/movement/MovementTraversal.hpp`; no duplicate traversal string table was introduced.
  - `recordProductWallJumpTraversalProof(...)` remains only a proof writer and does not move wall-jump movement/mutation policy.
- Controller migration:
  - `Controller.cpp` now includes `ControllerTraversalProof.hpp` and retains call sites only for the moved helpers.
  - Traversal execution, wall-jump mutation, jump/dash orchestration, reset/fall, command submission, target/outcome proof, movement proof, wall-run evaluation, and wall-surface queries were not moved.
- CMake:
  - Added `src/app/iggy3d/gameplay/ControllerTraversalProof.cpp` beside the other controller split sources.
- Required grep classification:
  - Moved helper declarations/definitions live in `ControllerTraversalProof.*`.
  - `Controller.cpp` retains call sites only.
  - No moved helper definition remains in `Controller.cpp`.
  - Focused dependency grep over `ControllerTraversalProof.*` returned no hits for `Session`, `executeTraversalIntent`, command submission, reset/fall, wall-jump mutation, active-room ownership, target proof, or outcome proof.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed.
  - Required traversal proof `rg` classification was run.
  - Focused dependency grep over `ControllerTraversalProof.*` was run.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card passed.
- Concerns/deferred:
  - None. No stage, commit, push, broad CTest, or window launch was performed.
