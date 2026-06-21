# `src/runtime/combat/CombatSystem.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic combat command resolution for the complete runtime.

## Build Position

- priority rank: 86
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/combat`
- file kind: `source`

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

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

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

- `src/runtime/combat/CombatSystem.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Implementation Contract

Required repo path:

```text
src/runtime/combat/CombatSystem.cpp
```

Include paired header first:

```cpp
#include "runtime/combat/CombatSystem.hpp"
```

Implementation algorithm for `applyAttack`:

1. Validate combat state structure; invalid HP ranges or duplicate combatant
   entities return `InvalidCombatState`.
2. Validate attacker combatant record; missing attacker returns
   `InvalidAttacker`.
3. Validate target combatant record; missing target returns `InvalidTarget`.
4. Reject defeated attacker as `AttackerDefeated`.
5. Reject already defeated target as `TargetDefeated`.
6. Reject friendly fire as `FriendlyFireBlocked` when attacker `factionId != 0`
   and attacker/target `factionId` values match.
7. Reject damage `<= 0` as `InvalidDamage`.
8. Apply deterministic integer damage.
9. Clamp target hit points to `[0, maxHitPoints]`.
10. If hit points reach zero, set the target combatant `defeated=true`.
11. Return structured result with damage applied and target HP.

Atomicity:

- Invalid requests mutate nothing.
- `FriendlyFireBlocked` mutates nothing and reports attacker/target ids plus the
  attempted damage in result diagnostics.
- Successful damage mutates only `CombatState`.
- `CombatSystem` does not mutate `WorldState` active/inactive flags or any
  world-level defeated flag in this first complete build.

First-room implication:

- The first-room demo does not require combat execution. Combat tests should
  prove empty state is valid and no acceptance path depends on combat side
  effects.

Save/replay/multiplayer:

- Combat state is save/hash truth for combat defeat.
- World defeated/active flags are never written by combat in the complete build.
- Replayed combat command with same state/request must produce same HP and
  defeat result.
- Multiplayer commands enter through authority/admission before combat
  execution.
