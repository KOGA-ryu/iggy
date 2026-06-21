# `src/runtime/inventory/InventorySystem.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic inventory add/remove/query operations.

## Build Position

- priority rank: 77
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/inventory`
- file kind: `header`

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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/runtime/inventory/InventorySystem.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/inventory/InventorySystem.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>

#include "runtime/inventory/InventoryState.hpp"
#include "runtime/player/PlayerSlot.hpp"
```

Required enum:

```cpp
enum class InventoryStatus : std::uint8_t {
  Ok,
  InvalidState,
  InvalidPlayerSlot,
  InvalidItemId,
  InvalidCount,
  MissingItem,
  InsufficientCount,
};
```

Required request/result values:

```cpp
struct InventoryItemRequest {
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::uint32_t count = 0;
};

struct InventoryOperationResult {
  InventoryStatus status = InventoryStatus::InvalidState;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::uint32_t requestedCount = 0;
  std::uint32_t finalCount = 0;
  bool mutated = false;
};
```

Required API:

```cpp
InventoryOperationResult addItem(
    InventoryState& state,
    const InventoryItemRequest& request);

InventoryOperationResult removeItem(
    InventoryState& state,
    const InventoryItemRequest& request);

bool hasItem(
    const InventoryState& state,
    PlayerSlotId playerSlot,
    const std::string& itemId,
    std::uint32_t count);

const PlayerInventory* findInventory(
    const InventoryState& state,
    PlayerSlotId playerSlot);
```

Mutable lookup helpers are private to this subsystem when implemented.

Semantics:

- `addItem` requires an existing `PlayerInventory` for the requested player slot.
  Missing inventory returns `InvalidPlayerSlot` and mutates nothing.
- `Session::create` owns creating the first-room player slot 0 inventory with
  empty stacks before gameplay starts.
- `addItem` succeeds by increasing an existing stack count in place or appending
  a new stack at the end of the stack vector.
- Empty `itemId` and `count == 0` are invalid and mutate nothing.
- `removeItem` removes from an existing stack and erases the stack when final
  count becomes zero.
- `hasItem(..., count=0)` returns false. Mutating operations reject zero count as
  `InvalidCount`.

This system owns inventory mutation only. It does not validate pickup reach,
deactivate world entities, complete objectives, or format UI slots.
