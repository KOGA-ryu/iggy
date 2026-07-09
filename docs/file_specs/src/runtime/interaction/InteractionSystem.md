# Interaction System

File:

- `/Users/kogaryu/iggy3d/src/runtime/interaction/InteractionDefinition.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/interaction/InteractionSystem.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/interaction/InteractionSystem.cpp`

Verified at: `75e85fb5`

## Owns

- Runtime interaction definition packet: kind, primary effect, item/objective fields, required-item fields, repeatability, and target deactivation flag.
- Interaction execution request/result/status contract.
- Applying accepted interact commands to inventory, objective, and target active-state effects.
- Interaction-specific validation for pickup, emit-only, objective-completion, inspect, and required-item gates.

## Does Not Own

- Command admission before interaction execution.
- Generic inventory stack behavior.
- Objective condition/evaluation rules beyond calling objective APIs.
- World entity storage or target lookup semantics.
- App input routing, receipts, or UI presentation.

## Reads

- Accepted `CommandRecord` interact command with actor and entity target.
- Mutable `WorldState`, `InventoryState`, and `ObjectiveState` from `InteractionSystemContext`.
- Target entity `InteractionDefinition`.

## Writes / Mutates

- Mutates inventory through `addItem` for pickup interactions.
- Mutates objective state for complete-objective effects or post-pickup objective evaluation.
- Mutates world target active flag through `WorldState::setActive` when the interaction requests deactivation.
- Returns `InteractionResult` with copied interaction facts and mutation flags.

## Calls Out To / Wires Out To

- Calls `WorldState::findById` and `WorldState::setActive`.
- Calls `hasItem` and `addItem` from inventory.
- Calls `validateObjectiveEvaluationContext`, `evaluateObjectives`, and local objective-completion logic.
- `SessionTick` constructs `InteractionSystemContext` and executes accepted interact commands.

## Called By / Entry Points

- `executeInteraction(...)`

## Invariants

- Only accepted `Interact` commands with valid actor and entity target execute.
- Missing world, inventory, or objective state fails before mutation.
- Actor must exist and be active; target must exist and be active.
- Required item id/count must be both present or both absent, and required items must be present before effects run.
- Inspect succeeds without mutation.
- Emit-only interactions may deactivate target but do not mutate inventory/objectives.
- Pickup requires add-item shape and successful objective evaluation context before inventory mutation.
- Result mutation flags must reflect inventory, objective, and target mutations separately.

## Tests / Proof Commands

- `rg -n "interaction_system_tests|executeInteraction|InteractionStatus|InteractionDefinition" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `interaction_system_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/inventory/InventorySystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/world/WorldState.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/SessionTick.*`

## Update When

- Interaction definition fields, supported effect shapes, required-item semantics, mutation ordering, status meanings, or result fields change.

## Do Not Update When

- Only command admission reach/target checks, inventory internals, objective condition rules, or app feedback copy changes.
