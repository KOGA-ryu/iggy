# `src/runtime/objective/ObjectiveSystem.hpp`

Updated: 2026-06-20

Exact purpose: declare objective evaluation and lifecycle outcome transitions.

## Build Position

- priority rank: 93
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/objective`
- file kind: `header`

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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/runtime/objective/ObjectiveSystem.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/objective/ObjectiveSystem.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>

#include "runtime/inventory/InventoryState.hpp"
#include "runtime/objective/ObjectiveState.hpp"
```

Do not include `SessionState.hpp` from this header. `ObjectiveSystem` mutates
only `ObjectiveState` and reports a local outcome suggestion that session or
summary code can consume.

Required enum:

```cpp
enum class ObjectiveEvaluationStatus : std::uint8_t {
  Evaluated,
  InvalidObjectiveState,
  InvalidInventoryState,
};

enum class ObjectiveOutcomeSuggestion : std::uint8_t {
  None,
  DemoComplete,
  Victory,
  Defeat,
};
```

Required result:

```cpp
struct ObjectiveEvaluationResult {
  ObjectiveEvaluationStatus status = ObjectiveEvaluationStatus::Evaluated;
  std::uint32_t evaluatedCount = 0;
  std::uint32_t completedThisEvaluation = 0;
  bool outcomeChanged = false;
  ObjectiveOutcomeSuggestion suggestedOutcome =
      ObjectiveOutcomeSuggestion::None;
};
```

Required API:

```cpp
ObjectiveEvaluationStatus validateObjectiveEvaluationContext(
    const ObjectiveState& objectives,
    const InventoryState& inventory);

ObjectiveEvaluationResult evaluateObjectives(
    ObjectiveState& objectives,
    const InventoryState& inventory);

bool objectiveComplete(
    const ObjectiveState& objectives,
    const std::string& objectiveId);
```

Semantics:

- Evaluation is deterministic and scans objective vector order.
- `validateObjectiveEvaluationContext` performs only structural checks and
  mutates nothing.
- For a context that passes `validateObjectiveEvaluationContext`,
  `evaluateObjectives` cannot return `InvalidObjectiveState` or
  `InvalidInventoryState` in the first build.
- Completed objectives stay complete.
- Failed objectives remain failed in the complete build unless a current
  explicit reset API replaces or resets the objective state.
- Every active objective whose `PlayerHasItem` condition is met transitions to
  `Complete` exactly once while scanning objective vector order.
- Completing `collect_gold_key` suggests `ObjectiveOutcomeSuggestion::DemoComplete`.
- Session owns applying lifecycle/outcome; objective system does not own the
  whole session.
