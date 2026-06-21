# `src/runtime/combat/CombatSystem.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic combat command resolution for the complete runtime.

## Build Position

- priority rank: 85
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/combat`
- file kind: `header`

## Ownership

This file owns:

- attack range validation handoff
- damage application
- defeat handling
- combat events

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

- integer damage values from command/content defaults
- target health mutation
- combat defeat flag when health reaches zero

## Semantics

- no RNG in first build
- combat uses command/admission authority
- defeated combatants cannot be attack sources

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(combatant lookup) per combat command.
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- Combat state is save/hash truth; combat request/result values are transient and
  excluded from save/hash unless runtime events explicitly record them.
- Save/hash order is `CombatState::combatants` vector order.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/combat/CombatSystem.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/combat/CombatSystem.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/command/Command.hpp"
```

Required enum:

```cpp
enum class CombatStatus : std::uint8_t {
  Succeeded,
  InvalidCombatState,
  InvalidAttacker,
  InvalidTarget,
  AttackerDefeated,
  TargetDefeated,
  FriendlyFireBlocked,
  InvalidDamage,
};
```

Required request/result:

```cpp
struct CombatAttackRequest {
  EntityId attacker;
  EntityId target;
  std::int32_t damage = 0;
  CommandId sourceCommandId = kInvalidCommandId;
};

struct CombatAttackResult {
  CombatStatus status = CombatStatus::InvalidCombatState;
  EntityId attacker;
  EntityId target;
  std::int32_t damageApplied = 0;
  std::int32_t targetHitPoints = 0;
  bool targetDefeated = false;
  bool combatMutated = false;
};
```

Required API:

```cpp
CombatAttackResult applyAttack(
    CombatState& combat,
    const CombatAttackRequest& request);
```

Semantics:

- `applyAttack` is a complete deterministic API in this build. Tests call it
  directly; command integration is outside this file's contract and is not
  required by the first-room acceptance demo.
- No RNG in first build.
- No animation or renderer state.
- Defeat updates `CombatState` only.
- `applyAttack` does not mutate `WorldState`, active flags, defeated flags
  outside `CombatState`, command log, objectives, inventory, projection, or
  renderer state.
- Combat defeat never writes world active/inactive truth in the complete build.
- Missing attacker returns `InvalidAttacker`.
- Missing target returns `InvalidTarget`.
- Friendly fire is blocked when attacker `factionId != 0` and
  `attacker.factionId == target.factionId`.
- `factionId == 0` is neutral/no-team and does not trigger friendly-fire blocking
  by itself.
- `FriendlyFireBlocked` mutates no hit points or defeated flags and reports the
  attacker, target, and attempted damage through the result fields.
- Invalid combatant structure, including negative `maxHitPoints` or
  `hitPoints` outside `[0, maxHitPoints]` before applying damage, returns
  `InvalidCombatState`.
- Non-positive damage returns `InvalidDamage`.
- Damage clamps target hit points to `[0, maxHitPoints]`.
- First-room fixture has no combatants; empty combat state is valid.
