# `tests/unit/inventory_system_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove inventory state operations behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 79
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

- add stack
- remove stack
- missing item fails
- stable summary order

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

- `tests/unit/inventory_system_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Test Contract

Required repo path:

```text
tests/unit/inventory_system_tests.cpp
```

Required tests:

- `default_inventory_state_has_no_player_inventories`: constructed state has no
  player inventories, no stacks, and no accidental item truth.
- `first_room_session_inventory_has_player_zero_empty`: helper creates
  `PlayerInventory{playerSlot=0, stacks={}}` and `hasItem(player0, anything, 1)`
  is false.
- `add_gold_key_to_player_zero`: create player 0 inventory first, add
  `gold_key:1`, assert status `Ok`, `mutated=true`, final count `1`, and
  `hasItem(player0,"gold_key",1)`.
- `missing_player_inventory_rejects_add`: default constructed state plus
  `addItem(player0,"gold_key",1)` returns `InvalidPlayerSlot` and mutates
  nothing.
- `adding_same_item_increments_existing_stack`: add twice and assert one stack
  with deterministic count/order.
- `adding_distinct_items_preserves_first_acquisition_order`: stack vector order
  is deterministic insertion order and is not sorted by item id.
- `invalid_player_slot_rejects`: invalid slot returns `InvalidPlayerSlot` and
  does not mutate.
- `empty_item_id_rejects`: returns `InvalidItemId`.
- `zero_count_rejects`: returns `InvalidCount`.
- `invalid_add_paths_mutate_nothing`: missing inventory, invalid slot, empty
  `itemId`, zero `count`, and overflow leave inventory byte-for-byte unchanged.
- `remove_existing_item_decrements_or_erases`: removing available count returns
  `Ok`; zero count stack policy is asserted.
- `remove_missing_item_fails`: returns `MissingItem`.
- `remove_too_many_fails`: returns `InsufficientCount`.
- `has_item_requires_available_count`: exact and over-count queries behave
  deterministically, and requested count `0` returns false.
- `inventory_operations_do_not_touch_world_or_objectives`: tests use no world
  mutation and no objective mutation.

Acceptance linkage:

- Interaction tests and acceptance depend on this file proving that successful
  pickup can add `inventory.player0=gold_key:1` exactly once.
