# E241 - Controller Split G8a: Target Outcome Proof

## Status

Ready.

## Objective

Continue the `Controller.cpp` split by extracting command proof stringifiers and
target/outcome proof writers into a controller-owned helper file. Keep target
querying, reach checks, command construction, command submission, ticking, and
movement/jump handling in `Controller.cpp`.

## Context

E232 extracted `ControllerKinematics.*`. E233 extracted
`ControllerMovementProof.*`. E234 extracted `ControllerGroundQueries.*`. E235
extracted `ControllerJumpDashState.*`. E236 extracted
`ControllerWallQueries.*`. E237 extracted `ControllerWallRunEvaluation.*`. E238
extracted `ControllerTraversalProof.*`. E239 extracted
`ControllerPlayerAccess.*`. E240 extracted `ControllerResetFall.*`.

After E240, `Controller.cpp` is roughly 1184 lines. The remaining target and
outcome proof code is still mixed with command submission. This card moves only
the proof/text layer, not the gameplay command execution path.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp`
- `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.cpp`

Move this proof/text cluster out of
`src/app/iggy3d/gameplay/Controller.cpp`:

- `ProductInteractionOutcomeSnapshot`
- `commandKindName(...)`
- `targetQueryStatusName(...)`
- `entityKindName(...)`
- `commandRejectionReasonName(...)`
- `reachGateName(...)`
- `clearProductTargetProof(...)`
- `clearProductOutcomeProof(...)`
- `inventoryItemCount(...)`
- `makeProductInteractionOutcomeSnapshot(...)`
- `recordProductInteractionOutcomeProof(...)`
- `recordProductTargetProof(...)`

Suggested API shape:

- Export from `ControllerTargetOutcomeProof.hpp`:
  - `ProductInteractionOutcomeSnapshot`
  - `std::string commandKindName(CommandKind)`
  - `std::string commandRejectionReasonName(CommandRejectionReason)`
  - `std::string reachGateName(CommandRejectionReason)`
  - `void clearProductTargetProof(ProductAppWindowState&)`
  - `void clearProductOutcomeProof(ProductAppWindowState&)`
  - `ProductInteractionOutcomeSnapshot makeProductInteractionOutcomeSnapshot(const Session&, PlayerSlotId, EntityId)`
  - `void recordProductInteractionOutcomeProof(const Session&, ProductAppWindowState&, const ProductInteractionOutcomeSnapshot&)`
  - `void recordProductTargetProof(const Session&, ProductAppWindowState&, CommandKind, const TargetQueryResult&)`
- Keep `targetQueryStatusName(...)`, `entityKindName(...)`, and
  `inventoryItemCount(...)` file-local in `ControllerTargetOutcomeProof.cpp`
  unless compilation proves a direct caller needs them.
- The header may include the small type headers needed for `CommandKind`,
  `CommandRejectionReason`, `EntityId`, `PlayerSlotId`, and
  `TargetQueryResult`, and may forward-declare `ProductAppWindowState` and
  `Session`.
- The implementation may depend on inventory/objective/world/session state as
  needed by the moved proof functions.

## Non-Scope

Do not move or reshape:

- `queryProductGameplayTarget(...)`
- `submitProductGameplayCommand(...)`
- `submitProductMove(...)`
- `submitProductTargetCommand(...)`
- `tickProductGameplayCommand(...)`
- `commandMovementHasPhysicsFrameStats(...)`
- `applyProductLedgeFallMoveFallback(...)`
- reach checks, command construction, command submission, session ticking,
  movement proof, reset/fall, jump/dash, wall-run, traversal, ground queries,
  player access, or active-room ownership
- receipt keys/order/values

Do not rename command kinds, target statuses, entity kinds, rejection reason
strings, reach-gate strings, outcome statuses, receipt fields, HUD labels, or
movement state enum values.

## CMake

- Add `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.cpp` to the
  `iggy3d` library source list near the other controller split files.

No new test target is required for this slice; existing focused gameplay tests
and receipt key-order oracle are the behavior guard.

## Required Greps

After the move:

```sh
rg -n "ProductInteractionOutcomeSnapshot|commandKindName|targetQueryStatusName|entityKindName|commandRejectionReasonName|reachGateName|clearProductTargetProof|clearProductOutcomeProof|inventoryItemCount|makeProductInteractionOutcomeSnapshot|recordProductInteractionOutcomeProof|recordProductTargetProof" /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.cpp
```

Classify results:

- moved declarations/definitions should live in
  `ControllerTargetOutcomeProof.*`.
- `targetQueryStatusName(...)`, `entityKindName(...)`, and
  `inventoryItemCount(...)` should be file-local definitions in
  `ControllerTargetOutcomeProof.cpp`.
- `Controller.cpp` should retain call sites/usages only for exported
  stringifiers, clear/record helpers, and `ProductInteractionOutcomeSnapshot`.
- no moved helper/type definition should remain in `Controller.cpp`.

Run an additional focused dependency grep over `ControllerTargetOutcomeProof.*`
and confirm it does not reference command submission, session ticking,
movement proof, reset/fall, jump/dash, traversal execution, wall-run
evaluation, wall-surface queries, ground queries, player position mutation, or
active-room ownership.

Allowed dependencies in `ControllerTargetOutcomeProof.*` are command/target
types, product window receipt state, session/world/inventory/objective state,
and target query result data needed to write proof fields.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

- Stop if the extraction requires moving target querying, reach checks, command
  construction/submission, session ticking, movement proof, reset/fall,
  jump/dash, traversal, wall-run, or active-room ownership.
- Stop if any proof strings, status strings, rejection reason names, reach-gate
  names, or receipt key/order/value output changes.
- Stop if focused gameplay controller tests or the receipt key-order oracle
  expose behavior drift.

No stage, commit, push, broad CTest, or window launch.

## Completion Brief

Files changed:

- `CMakeLists.txt`
- `src/app/iggy3d/gameplay/Controller.cpp`
- `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp`
- `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.cpp`
- `docs/creative_mode/builder_tasks/done/E241-controller-split-g8a-target-outcome-proof.md`

Exact APIs/types moved/added:

- Added `ControllerTargetOutcomeProof.hpp/.cpp`.
- Exported `ProductInteractionOutcomeSnapshot`.
- Exported `commandKindName(CommandKind)`.
- Exported `commandRejectionReasonName(CommandRejectionReason)`.
- Exported `reachGateName(CommandRejectionReason)`.
- Exported `clearProductTargetProof(ProductAppWindowState&)`.
- Exported `clearProductOutcomeProof(ProductAppWindowState&)`.
- Exported `makeProductInteractionOutcomeSnapshot(const Session&, PlayerSlotId, EntityId)`.
- Exported `recordProductInteractionOutcomeProof(const Session&, ProductAppWindowState&, const ProductInteractionOutcomeSnapshot&)`.
- Exported `recordProductTargetProof(const Session&, ProductAppWindowState&, CommandKind, const TargetQueryResult&)`.

Exact private helpers moved:

- `targetQueryStatusName(...)` remains file-local in `ControllerTargetOutcomeProof.cpp`.
- `entityKindName(...)` remains file-local in `ControllerTargetOutcomeProof.cpp`.
- `inventoryItemCount(...)` remains file-local in `ControllerTargetOutcomeProof.cpp`.

Scope notes:

- `Controller.cpp` now includes `app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp` and retains usages only for the exported stringifiers, clear/record helpers, and `ProductInteractionOutcomeSnapshot`.
- `CMakeLists.txt` now registers `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.cpp` next to the other controller split files.
- Target querying, reach checks, command construction/submission, session ticking, movement proof, reset/fall, jump/dash, traversal, wall-run, active-room ownership, receipt keys/order/values, staging, commit, push, broad CTest, and window launch were not changed.

Required grep classification:

- `rg -n "ProductInteractionOutcomeSnapshot|commandKindName|targetQueryStatusName|entityKindName|commandRejectionReasonName|reachGateName|clearProductTargetProof|clearProductOutcomeProof|inventoryItemCount|makeProductInteractionOutcomeSnapshot|recordProductInteractionOutcomeProof|recordProductTargetProof" ...` shows moved declarations/definitions in `ControllerTargetOutcomeProof.*`.
- The same grep shows `targetQueryStatusName(...)`, `entityKindName(...)`, and `inventoryItemCount(...)` only as file-local definitions/usages in `ControllerTargetOutcomeProof.cpp`.
- The same grep shows `Controller.cpp` retains usages only for exported helpers/types.
- Focused dependency grep over `ControllerTargetOutcomeProof.*` returned no forbidden dependency hits for command submission, session ticking, movement proof, reset/fall, jump/dash, traversal execution, wall-run evaluation, wall-surface queries, ground queries, player position mutation, or active-room ownership.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 3/3 tests.
- `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over touched files and this card passed.
