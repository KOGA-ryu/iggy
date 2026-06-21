# `src/runtime/objective/ObjectiveState.hpp`

Updated: 2026-06-20

Exact purpose: declare saved objective truth and session outcome facts.

## Build Position

- priority rank: 92
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/objective`
- file kind: `header`

## Ownership

This file owns:

- objective records
- completion flags
- failure flags
- current outcome

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

- objective id `collect_gold_key` for demo
- status inactive, active, complete, failed
- outcome playing, won, lost, reset

## Semantics

- objective state is save truth
- UI messages are derived diagnostics/projection
- objective evaluation is deterministic

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(objective count).
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- Objective state is save/hash truth.
- Save/hash order is `ObjectiveState::objectives` vector order.
- Objective evaluation results are transient and excluded from save/hash unless
  runtime events explicitly record them.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/objective/ObjectiveState.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/objective/ObjectiveState.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/player/PlayerSlot.hpp"
```

Required enums:

```cpp
enum class ObjectiveStatus : std::uint8_t {
  Inactive,
  Active,
  Complete,
  Failed,
};

enum class ObjectiveConditionKind : std::uint8_t {
  None,
  PlayerHasItem,
};
```

Required value types:

```cpp
struct ObjectiveCondition {
  ObjectiveConditionKind kind = ObjectiveConditionKind::None;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::uint32_t itemCount = 0;
};

struct ObjectiveRecord {
  std::string objectiveId;
  ObjectiveStatus status = ObjectiveStatus::Inactive;
  ObjectiveCondition condition;
};

struct ObjectiveState {
  std::vector<ObjectiveRecord> objectives;
};
```

Acceptance objective:

- id: `collect_gold_key`;
- initial status: `Active`;
- condition: player slot 0 has `gold_key:1`;
- final status after pickup: `Complete`.

Invariants:

- Objective ids are non-empty and unique.
- Objective vector order is deterministic seed order.
- Objective state stores gameplay truth only, not UI messages or renderer
  markers.
- `ObjectiveOutcomeSuggestion::DemoComplete` is suggested by `ObjectiveSystem`
  when `collect_gold_key` completes; session/summary code can consume that
  suggestion, but objective state does not own session lifecycle.
