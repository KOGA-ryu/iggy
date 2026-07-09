# Objective System

File:

- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveSystem.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveSystem.cpp`

Verified at: `e5586141`

## Owns

- Runtime objective packet shape: `ObjectiveStatus`, `ObjectiveConditionKind`, `ObjectiveCondition`, `ObjectiveRecord`, and `ObjectiveState`.
- Objective evaluation request/result/status contracts.
- Validation of objective state and inventory shape for evaluation.
- Completion of active objectives whose conditions are met by inventory.
- Objective completion query by id.

## Does Not Own

- Objective-to-session-outcome rule table; that belongs to `ObjectiveOutcome`.
- Inventory mutation.
- Interaction effect routing that triggers objective checks.
- Save/load serialization or state hash field selection.
- App HUD/receipt projection of objectives.

## Reads

- `ObjectiveState::objectives`, objective ids, statuses, and conditions.
- `InventoryState` player inventories and stacks.
- Inventory `hasItem(...)` for `PlayerHasItem` conditions.

## Writes / Mutates

- `evaluateObjectives` mutates active objective rows to `Complete` when conditions are met.
- `validateObjectiveEvaluationContext` and `objectiveComplete` are read-only.

## Calls Out To / Wires Out To

- Calls `hasItem(...)` from runtime inventory.
- `InteractionSystem` uses validation and evaluation around pickup/objective effects.
- `SessionTick` evaluates objective outcome after tick execution.
- Runtime diagnostics and projection read objective state.

## Called By / Entry Points

- `validateObjectiveEvaluationContext(...)`
- `evaluateObjectives(...)`
- `objectiveComplete(...)`

## Invariants

- Objective ids must be non-empty and unique for evaluation.
- Objective statuses must be known enum values.
- `None` conditions are valid but do not auto-complete.
- `PlayerHasItem` conditions require valid player slot, non-empty item id, and nonzero item count.
- Invalid inventory structure blocks evaluation.
- Only active objectives can transition to complete.
- Completion of `collect_gold_key` suggests `DemoComplete`; terminal outcome rule ownership is separate.

## Tests / Proof Commands

- `rg -n "objective_system_tests|validateObjectiveEvaluationContext|evaluateObjectives|objectiveComplete" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `objective_system_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveOutcome.*`
- `/Users/kogaryu/iggy3d/src/runtime/inventory/InventorySystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/interaction/InteractionSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveCodec.*`

## Update When

- Objective packet fields, validation rules, condition kinds, evaluation mutation policy, or evaluation result fields change.

## Do Not Update When

- Only outcome rule ordering, inventory stack internals, save formatting, or app presentation changes.
