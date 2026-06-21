# `src/runtime/ai/AiState.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic AI truth needed for complete-build
simulation and multiplayer-safe ordering.

## Build Position

- priority rank: 88
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/ai`
- file kind: `header`

## Ownership

This file owns:

- AI enabled flags
- last decision tick
- behavior kind
- target memory as ids

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

- idle, guard, pursue, interact behavior kinds
- per-entity AI records if not stored in EntityState

## Semantics

- AI state is save truth
- AI produces command proposals only
- AI never mutates world directly

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(ai actor count).
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- AI state is save/hash truth.
- Save/hash order is `AiState::actors` vector order.
- AI proposal/result values are transient and excluded from save/hash unless
  admitted/logged as `CommandRecord` values or explicitly recorded as runtime
  events.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/ai/AiState.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/ai/AiState.hpp
```

AI state is authoritative runtime truth for deterministic AI bookkeeping. AI
does not mutate world directly; proposal output is transient until an external
authority/admission path accepts and logs a command.

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <vector>

#include "core/ids/EntityId.hpp"
```

Required value types:

```cpp
struct AiActorState {
  EntityId actor;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  bool enabled = true;
};

struct AiState {
  std::vector<AiActorState> actors;
};
```

Invariants:

- AI actor records are stable in seed/world order.
- `deterministicPolicy == 0` means dormant no-proposal policy.
- Nonzero policy ids are reserved values and also produce no proposals in the
  complete build.
- No unseeded random values.
- No raw perception buffers that depend on renderer, UI, wall-clock, or
  unordered traversal.
- First-room fixture has no AI actors; empty AI state is valid.

Save/replay:

- AI state is save/hash truth when AI actors exist.
- Command proposals are transient until admitted/logged as command records.
