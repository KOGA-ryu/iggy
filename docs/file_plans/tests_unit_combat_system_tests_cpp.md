# `tests/unit/combat_system_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove deterministic combat resolution behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 87
- tier: Tier 5: Playable Gameplay Systems
- module: `unit tests`
- file kind: `test`

## Ownership

This file owns:

- test scenarios
- assertions
- fixture setup helpers local to this test file
- regression coverage for documented semantics

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- public iggy3d headers
- test framework chosen by `cmake/iggy3d_tests.cmake`
- fixtures under `/Users/kogaryu/iggy3d/fixtures`

## Data Contract

- damage subtracts hit points
- defeat state set at zero
- defeated actor cannot act
- no random variance

## Semantics

- tests must be deterministic
- tests must not require renderer or old iggy code
- tests should assert exact rejection/status codes when behavior is part of runtime contract

## Implementation Plan

1. include the test framework and only public `iggy3d` headers required for the scenario;
2. build test state through public APIs or fixture loaders;
3. assert the exact success, failure, rejection, and deterministic replay behavior named in this document;
4. keep the test independent of renderer, network services, wall-clock timing, and old `iggy` code.

## Compute Cost

- Test runtime should stay small; fixture-level tests may scan all demo entities and commands.
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- if this file owns save truth, it must define exact fields included in `SaveEnvelope`;
- if this file owns derived data, it must be regenerable and excluded from save truth;
- if this file affects commands, replay must reproduce the same result and state hash.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `tests/unit/combat_system_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Test Contract

Required repo path:

```text
tests/unit/combat_system_tests.cpp
```

Required tests:

- `empty_combat_state_is_valid_for_first_room`: no combatants required by the
  pickup acceptance fixture.
- `attack_reduces_hit_points_deterministically`: fixed damage subtracts exact
  integer amount.
- `defeat_sets_zero_hp_and_defeated_flag`: lethal damage clamps at zero and
  marks defeated.
- `defeat_does_not_mutate_world_flags`: lethal damage changes only
  `CombatState`; world active/inactive or defeated flags remain unchanged.
- `invalid_attacker_or_target_fails_without_mutation`.
- `invalid_combat_state_fails_without_mutation`: negative max HP, HP outside
  `[0,maxHitPoints]`, or duplicate combatant entity returns
  `InvalidCombatState`.
- `same_nonzero_faction_blocks_friendly_fire`: attacker and target with matching
  nonzero `factionId` return `FriendlyFireBlocked`, preserve HP/defeated flags,
  and report attacker/target plus attempted damage.
- `neutral_faction_does_not_block_by_itself`: `factionId == 0` on one or both
  combatants does not trigger `FriendlyFireBlocked`; valid damage proceeds.
- `different_factions_apply_damage`: different nonzero `factionId` values allow
  deterministic damage.
- `defeated_attacker_cannot_attack`.
- `already_defeated_target_rejects_as_target_defeated`.
- `invalid_damage_rejects`: zero/negative damage does not mutate.
- `combat_does_not_use_randomness_or_wall_clock`.
- `combat_does_not_touch_inventory_objectives_command_log_projection_renderer`.

Acceptance linkage:

- These tests ensure combat state can sit inside `SessionState` and save/hash
  without affecting the first-room pickup path.
