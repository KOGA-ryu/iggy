# E235 - Controller Split G4a: Jump Dash State Helpers

## Status

Done.

## Objective

Continue the `Controller.cpp` split with a narrow jump/dash helper extraction.
Move only the small state-writer and timing helpers that do not own command
submission, reset/fall policy, traversal, wall-jump, or wall-run behavior.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. E234 extracted `ControllerGroundQueries.*`.

The original E231 G4 wording said "Jump/Dash Submit", but moving
`submitProductJump(...)` or `submitProductDash(...)` now would pull in
command submission, target/outcome proof, traversal, wall-jump, reset/fall, and
active-room collision seams. Split the jump/dash lane first by extracting only
the simple state helpers.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerJumpDashState.hpp`
- `src/app/iggy3d/gameplay/ControllerJumpDashState.cpp`

Move only these helpers out of `src/app/iggy3d/gameplay/Controller.cpp`:

- `recordProductJumpPosition(...)`
- `clearProductJumpTiming(...)`
- `rejectProductJump(...)`
- `productJumpBufferLive(...)`
- `bufferProductJump(...)`
- `applyProductJumpReleaseCut(...)`
- `advanceProductDashCooldown(...)`
- `rejectProductDash(...)`

Use the new helper from `Controller.cpp`.

Suggested API shape:

- Export all moved helpers from `ControllerJumpDashState.hpp`.
- Keep the helper implementation free of `Session`, `SpatialSurfaceSet`,
  command submission, collision, traversal, wall-run, wall-jump, reset, and
  target/outcome proof dependencies.
- It is acceptable for the helper header to include/forward-declare only what is
  needed for `ProductAppWindowState` and `std::string_view`.

## Non-Scope

Do not move or reshape:

- `beginProductJumpArc(...)`
- `tryProductCoyoteJump(...)`
- `tryProductTraversalJump(...)`
- `tryProductWallJump(...)`
- `advanceProductJump(...)`
- `submitProductJump(...)`
- `submitProductDash(...)`
- `submitProductGameplayCommand(...)`
- `clearProductTargetProof(...)`
- `clearProductOutcomeProof(...)`
- `setProductPlayerPosition(...)`
- reset, fall, ledge fallback, wall-run, wall-jump, traversal, target/outcome,
  or command submission logic
- movement proof writer helpers from E233
- ground-query helpers from E234
- receipt keys/order/values

Do not rename status strings, reason codes, HUD labels, receipt fields, room
anchor kinds, traversal tags, or movement state enum values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerJumpDashState.cpp` to the `iggy3d`
  library source list near the other controller split files.

No new test target is required for this slice; existing focused gameplay tests
are the behavior guard.

## Required Greps

After the move:

```sh
rg -n "recordProductJumpPosition|clearProductJumpTiming|rejectProductJump|productJumpBufferLive|bufferProductJump|applyProductJumpReleaseCut|advanceProductDashCooldown|rejectProductDash" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerJumpDashState.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerJumpDashState.cpp
```

Classify results:

- moved helper declarations/definitions should live in
  `ControllerJumpDashState.*`.
- `Controller.cpp` should retain call sites only.
- no moved helper should remain defined in `Controller.cpp`.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if the extraction requires moving submit/advance orchestration into the
  new helper.
- Stop if the new helper starts depending on `Session`, collision surfaces,
  command submission, traversal, reset/fall, wall-jump, wall-run, or
  target/outcome proof code.
- Stop if focused gameplay controller tests expose any jump, dash, reset/fall,
  traversal, or movement behavior drift.

No stage, commit, push, broad CTest, or window launch.

## Completion Brief

- Card moved to done: yes, after this brief was appended.
- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/ControllerJumpDashState.hpp`
  - `src/app/iggy3d/gameplay/ControllerJumpDashState.cpp`
  - `docs/creative_mode/builder_tasks/done/E235-controller-split-g4a-jump-dash-state-helpers.md`
- Helper/API shape added:
  - `ControllerJumpDashState.hpp/.cpp` exports `recordProductJumpPosition(...)`, `clearProductJumpTiming(...)`, `rejectProductJump(...)`, `productJumpBufferLive(...)`, `bufferProductJump(...)`, `applyProductJumpReleaseCut(...)`, `advanceProductDashCooldown(...)`, and `rejectProductDash(...)`.
  - The helper header forward-declares `ProductAppWindowState` and includes only `std::string_view`.
  - The helper implementation depends on `ProductAppWindowState` and standard library utilities only.
- Controller migration:
  - `Controller.cpp` now includes `ControllerJumpDashState.hpp` and retains call sites only for the moved helpers.
  - Jump submit/advance orchestration, dash submit orchestration, command submission, traversal, wall-jump, wall-run, reset/fall, target/outcome proof, movement proof, and ground-query helpers were not moved.
- CMake:
  - Added `src/app/iggy3d/gameplay/ControllerJumpDashState.cpp` beside the other controller split sources.
- Required grep classification:
  - Moved helper declarations/definitions live in `ControllerJumpDashState.*`.
  - `Controller.cpp` retains call sites only.
  - No moved helper remains defined in `Controller.cpp`.
  - A focused dependency grep over `ControllerJumpDashState.*` found no `Session`, `SpatialSurfaceSet`, command, collision, traversal, wall-run, wall-jump, reset/fall, active-room, target, or outcome dependencies.
- Receipt golden result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed.
  - Required jump/dash state `rg` classification was run.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card passed.
- Concerns/deferred:
  - None. No stage, commit, push, broad CTest, or window launch was performed.
