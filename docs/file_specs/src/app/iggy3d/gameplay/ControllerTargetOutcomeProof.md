# File Spec

Files: `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp`, `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.cpp`

Verified at: `08813cf2`

## Owns

- Product target, reach-gate, command-kind, rejection, and interaction outcome proof helpers.
- Snapshot-before / proof-after comparison for interaction target activity, inventory count, objective completion, and transient event count.
- String names for product-visible command, target, entity-kind, and rejection status fields.

## Does Not Own

- Runtime command admission, inventory mutation, objective mutation, target query execution, session ticking, HUD rendering, or receipt serialization.

## Reads

- `Session::state().world`, inventory, objectives, and transient events.
- `TargetQueryResult`, `CommandKind`, `CommandRejectionReason`, player slot, and target entity data.
- `ProductAppWindowState::gameplay.gameplayCommand` tick/acceptance state when deciding outcome status.

## Writes / Mutates

- Clears or fills target proof and outcome proof fields on `ProductAppWindowState`.
- Does not mutate runtime world, inventory, objectives, command queues, or session transient events.

## Calls Out To / Wires Out To

- `findInventory(...)` and `objectiveComplete(...)` for before/after comparisons.
- `WorldState::findById(...)` for target activity, stable name, kind, and interaction metadata.
- `toUint64(...)` for receipt-friendly entity id proof.

## Called By / Entry Points

- Target action submission and generic gameplay command execution.
- Reset, move, jump, and dash actions clear target/outcome proof when they are not target commands.
- Grep proof: `rg -n "recordProductTargetProof|recordProductInteractionOutcomeProof|makeProductInteractionOutcomeSnapshot|clearProductTargetProof|clearProductOutcomeProof|reachGateName" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Target proof must not report entity id, distance, or command support when target discovery fails.
- Outcome proof compares against a pre-command snapshot, not just post-command state.
- Rejected or non-advanced commands must not report succeeded outcome.
- Reach gate reports only `pass`, `fail`, or `not_attempted`.
- Missing target entities after a found query are reported as a product proof status, not silently treated as success.

## Tests / Proof Commands

- `rg -n "product_gameplay_controller_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "gameplayTarget|gameplayOutcome|gameplayReachGate|targetDiscovered|interactionExecuted|attackExecuted" tests/unit/product_gameplay_controller_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/targeting/*` unless target query result semantics change.
- `src/runtime/inventory/*` and `src/runtime/objective/*` unless proof comparisons need new facts.
- `src/app/iggy3d/gameplay/ControllerTargetActions.*` unless target command flow changes.

## Update When

- Product target/outcome fields, command/rejection naming, reach gate mapping, snapshot fields, or success/failure status derivation changes.

## Do Not Update When

- Only command binding, target query implementation internals, renderer formatting, or receipt ordering changes without changing product proof semantics.
