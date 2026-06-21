# `src/runtime/objective/ObjectiveSystem.cpp`

Updated: 2026-06-20

Exact purpose: implement objective evaluation and lifecycle outcome transitions.

## Build Position

- priority rank: 94
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/objective`
- file kind: `source`

## Ownership

This file owns:

- objective activation/completion rules
- win/loss evaluation
- outcome event emission

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core/config/runtime peer headers according to ownership
- content seed data only at session creation boundaries
- no app, projection, renderer, tests, or old iggy includes

## Data Contract

- reads inventory/world/combat as needed
- mutates `ObjectiveState` only
- returns lifecycle suggestions to session

## Semantics

- no rendering or UI notification
- same state yields same objective result
- demo completes when `gold_key` is in local player inventory

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(objective count plus referenced state lookups).
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- Objective state is save/hash truth; evaluation results are transient and
  excluded from save/hash unless runtime events explicitly record them.
- Save/hash order is `ObjectiveState::objectives` vector order.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/objective/ObjectiveSystem.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Implementation Contract

Required repo path:

```text
src/runtime/objective/ObjectiveSystem.cpp
```

Include paired header first:

```cpp
#include "runtime/objective/ObjectiveSystem.hpp"
```

`validateObjectiveEvaluationContext` algorithm:

1. Validate objective state structure without mutation: unique objective ids,
   valid objective statuses, non-empty ids, and valid condition payloads.
2. Validate inventory state structure without mutation: valid player slots,
   non-empty item ids, and positive stack counts.
3. Return `InvalidObjectiveState` or `InvalidInventoryState` for structural
   failures.
4. Return `Evaluated` when evaluation can run infallibly.

`evaluateObjectives` algorithm:

1. Iterate objective records in stored order.
2. Skip `Inactive`, `Complete`, and `Failed` objectives.
3. For `PlayerHasItem`, call `InventorySystem::hasItem`.
4. If condition is met, set objective status to `Complete`.
5. Count completions from this evaluation only.
6. If `collect_gold_key` completes, return suggested outcome
   `DemoComplete`.
7. Do not mutate inventory, world, command log, clock, camera, save,
   projection, renderer, or app state.

Failure behavior:

- Callers that need atomic interaction effects must call
  `validateObjectiveEvaluationContext` before mutating inventory or world.
- After validation succeeds, `evaluateObjectives` must not return invalid status
  for the same objective/inventory structure in the same tick.
- Structural invalid status mutates no objective records.

Acceptance-sensitive behavior:

- Before pickup, `collect_gold_key` remains `Active`.
- After inventory contains `gold_key:1`, evaluation completes it exactly once.
- Re-evaluation after completion is idempotent and does not increment
  `completedThisEvaluation` again.

Outcome suggestion:

- This file suggests `DemoComplete` when `collect_gold_key` completes.
- Session/summary code consumes the suggestion; `ObjectiveSystem` does not
  include or mutate `SessionState`.

Save/replay/hash:

- Objective status is save/hash truth.
- Evaluation result is transient and excluded from save/hash unless runtime
  events explicitly record it.
- Replay must produce the same objective transition at the same logical point.
