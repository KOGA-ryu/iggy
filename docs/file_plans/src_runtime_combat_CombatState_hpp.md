# `src/runtime/combat/CombatState.hpp`

Updated: 2026-06-20

Exact purpose: declare combat truth for health, faction, defeat, and deterministic damage resolution.

## Build Position

- priority rank: 84
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/combat`
- file kind: `header`

## Ownership

This file owns:

- health records or entity health fields if centralized
- defeated flags
- combat event ids

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

- hit points integer values
- faction ids
- defeated/inactive relationship

## Semantics

- no hidden random rolls
- combat state is save truth
- render animation state is excluded

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(combatant count).
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- Combat state is save/hash truth.
- Save/hash order is `CombatState::combatants` vector order.
- Combat result structs are transient and excluded from save/hash unless runtime
  events explicitly record them.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/combat/CombatState.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/combat/CombatState.hpp
```

Combat is required in the complete build surface but is not exercised by the
first-room pickup acceptance path. It has deterministic save-ready state and
fully defined attack mutation semantics.

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
```

Required value types:

```cpp
struct CombatantState {
  EntityId entity;
  std::uint32_t factionId = 0;
  std::int32_t hitPoints = 0;
  std::int32_t maxHitPoints = 0;
  bool defeated = false;
};

struct CombatState {
  std::vector<CombatantState> combatants;
};
```

Invariants:

- Combatants are ordered by seed/world order.
- `factionId == 0` means neutral/no-team and does not trigger friendly-fire
  blocking by itself.
- Matching nonzero `factionId` values are the complete-build friendly-fire team
  policy consumed by `CombatSystem`.
- `maxHitPoints >= 0`.
- `hitPoints` is clamped to `[0, maxHitPoints]` by `CombatSystem`.
- `defeated == true` implies `hitPoints == 0`.
- Combat state contains no animation, hit reaction, VFX, UI, renderer, random,
  or audio state.
- Empty combat state is valid for the first-room acceptance fixture.

Save/replay:

- Combat state is authoritative save/hash truth.
- Damage events/results are transient diagnostics unless runtime events persist
  them separately.
