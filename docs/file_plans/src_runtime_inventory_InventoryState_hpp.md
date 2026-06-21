# `src/runtime/inventory/InventoryState.hpp`

Updated: 2026-06-20

Exact purpose: declare player inventory truth for saved runtime state and interaction effects.

## Build Position

- priority rank: 76
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/inventory`
- file kind: `header`

## Ownership

This file owns:

- item stack records
- owner slot or actor binding
- capacity policy if any

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

- item id string
- count
- owner player slot
- stable ordering by first acquisition

## Semantics

- inventory is save truth
- UI icons/assets are not stored here
- pickup effects mutate through `InventorySystem`

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(item stack count).
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- Inventory state is save/hash truth.
- Save/hash order is `InventoryState::players` vector order, then each
  `PlayerInventory::stacks` vector order.
- `InventorySystem` operation result structs are transient and excluded from
  save/hash unless runtime events explicitly record them.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/inventory/InventoryState.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/inventory/InventoryState.hpp
```

Inventory is authoritative save truth for item stacks owned by player slots.
It does not own pickups in the world; world entities own active/inactive pickup
presence.

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/player/PlayerSlot.hpp"
```

`runtime/player/PlayerSlot.hpp` owns `PlayerSlotId` and
`kInvalidPlayerSlotId`.

Required value types:

```cpp
struct InventoryStack {
  std::string itemId;
  std::uint32_t count = 0;
};

struct PlayerInventory {
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::vector<InventoryStack> stacks;
};

struct InventoryState {
  std::vector<PlayerInventory> players;
};
```

Invariants:

- Player inventories are ordered by ascending `playerSlot`.
- A default constructed `InventoryState` has no player inventories and is not a
  playable session inventory by itself.
- `Session::create` for the first-room scenario creates one
  `PlayerInventory{playerSlot=0, stacks={}}` before gameplay commands execute.
- Stacks inside a player inventory use deterministic first-acquisition order.
- Existing stack count increases in place.
- New item stack is appended after existing stacks. Stack order is deterministic
  first-acquisition order for the complete build.
- `itemId` must be non-empty for valid stacks.
- `count > 0` for valid stacks.
- UI icons, mesh assets, hotbar position, and renderer handles are forbidden
  here.

Acceptance facts:

- Initial first-room inventory contains player slot 0 with empty stacks.
- Successful pickup adds `gold_key:1` to player slot 0.
- Final summary must report `inventory.player0=gold_key:1`.

Save/replay/multiplayer:

- Save includes all player inventories and stacks in deterministic order.
- Replay observes inventory mutation through `InteractionSystem` and
  `InventorySystem`.
- Multiplayer uses player slot id as owner; no socket or remote identity data is
  stored here.
