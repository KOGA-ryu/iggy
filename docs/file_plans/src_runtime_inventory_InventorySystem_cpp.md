# `src/runtime/inventory/InventorySystem.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic inventory add/remove/query operations.

## Build Position

- priority rank: 78
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/inventory`
- file kind: `source`

## Ownership

This file owns:

- add stack
- remove stack
- has item
- stable summary order

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

- mutates `InventoryState` only
- returns structured failure for missing item or invalid count

## Semantics

- no direct world pickup validation
- no UI slot layout
- same operations replay to same inventory order

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(item stack count) per operation in first build.
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- Inventory state is save/hash truth; operation results are transient and
  excluded from save/hash unless runtime events explicitly record them.
- Save/hash order is player inventory vector order, then stack vector order.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/inventory/InventorySystem.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Implementation Contract

Required repo path:

```text
src/runtime/inventory/InventorySystem.cpp
```

Include paired header first:

```cpp
#include "runtime/inventory/InventorySystem.hpp"
```

Implementation rules:

- Use linear scans over `InventoryState::players` and stack vectors.
- Preserve player inventory order and stack order.
- Do not use unordered containers for canonical storage.
- Do not allocate renderer/UI/asset data.
- Do not touch world, command log, objective, save codec, app paths, or old
  `iggy`.

`addItem` algorithm:

1. Reject null/invalid state shape only if detectable as `InvalidState`.
2. Reject invalid player slot as `InvalidPlayerSlot`.
3. Reject empty item id as `InvalidItemId`.
4. Reject count `0` as `InvalidCount`.
5. Find player inventory; missing player inventory returns
   `InvalidPlayerSlot`.
6. If stack exists, add count with deterministic overflow handling. Overflow
   rejects as `InvalidCount` and mutates nothing.
7. If stack missing, append new stack.
8. Return `Ok`, `mutated=true`, and final stack count.

`removeItem` algorithm:

1. Validate player slot, item id, and count.
2. Missing stack returns `MissingItem`.
3. Count greater than available returns `InsufficientCount`.
4. Subtract count.
5. Erase stack when final count becomes zero, preserving order of remaining
   stacks.
6. Return `Ok`, `mutated=true`, and final count.

Acceptance-sensitive behavior:

- `addItem(player0, "gold_key", 1)` requires a pre-created empty player 0
  inventory and then appends the `gold_key` stack so
  `hasItem(..., "gold_key", 1)` is true.
- A default constructed `InventoryState` without player inventories rejects that
  add as `InvalidPlayerSlot`.
- The system must not deactivate `gold_key`; that is `InteractionSystem`
  mutating `WorldState`.
- The system must not complete `collect_gold_key`; objective evaluation owns
  that.

Save/replay/hash:

- Inventory state is save/hash truth.
- Operation results are transient diagnostics and are saved only when runtime
  events explicitly record them.
