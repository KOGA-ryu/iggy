# Objective Outcome

File:

- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveOutcome.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveOutcome.cpp`

Verified at: `e5586141`

## Owns

- Data table shape that maps completed objective ids to terminal `SessionOutcome` values.
- Default objective outcome rule ordering.
- Exact/prefix match descriptors for objective outcome rules.

## Does Not Own

- Objective state mutation or condition evaluation.
- Session lifecycle/outcome application.
- Save/hash persistence of objective outcome rules.
- Content/deck validation that may later produce rule tables.

## Reads

- No runtime state.
- Static default mapping facts encoded in `buildObjectiveOutcomeTable`.

## Writes / Mutates

- Returns a new `ObjectiveOutcomeTable`.
- Does not mutate session or objective state.

## Calls Out To / Wires Out To

- `Session` rebuilds the table during create/load.
- `SessionTick` reads `SessionState::outcomeTable` when applying completed objective outcomes.

## Called By / Entry Points

- `buildObjectiveOutcomeTable(...)`

## Invariants

- Rule order is load-bearing.
- The `exit_` prefix rule maps to `Victory` and is evaluated before the gold-key rule.
- The exact `collect_gold_key` rule maps to `DemoComplete`.
- The table is transient session-level data and is rebuilt at create/load rather than saved or hashed.

## Tests / Proof Commands

- `rg -n "objective_outcome_tests|buildObjectiveOutcomeTable|ObjectiveOutcomeTable|outcomeTable" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `objective_outcome_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/SessionTick.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/SessionState.hpp`

## Update When

- Objective-outcome rule shape, default rule set, rule ordering, or transient/persistence boundary changes.

## Do Not Update When

- Only objective condition evaluation, interaction effects, or session outcome enum values change without table contract changes.
