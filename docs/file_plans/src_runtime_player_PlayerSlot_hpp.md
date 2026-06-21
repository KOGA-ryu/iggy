# `src/runtime/player/PlayerSlot.hpp`

Updated: 2026-06-20

Exact purpose: declare player slot identity, slot kind, and actor binding value
types used by `PlayerRoster`, command authority, save/load, replay, and
multiplayer-ready ordering.

## Build Position

- priority rank: 42
- tier: Tier 3: Player World Authority
- module: `src/runtime/player`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/runtime/player/PlayerSlot.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Types

Declare:

```cpp
using PlayerSlotId = std::uint32_t;

inline constexpr PlayerSlotId kInvalidPlayerSlotId =
    static_cast<PlayerSlotId>(UINT32_MAX);

enum class PlayerSlotKind : std::uint8_t {
  Unknown,
  Local,
  Remote,
  Ai,
  Observer,
};

struct PlayerSlot {
  PlayerSlotId id = kInvalidPlayerSlotId;
  PlayerSlotKind kind = PlayerSlotKind::Unknown;
  EntityId actor;
  std::string stableName;
};
```

Declare helpers:

```cpp
bool isValidPlayerSlotId(PlayerSlotId id);
bool isPlayableSlotKind(PlayerSlotKind kind);
bool isActorControllingSlotKind(PlayerSlotKind kind);
```

## Semantics

- Slot id `UINT32_MAX` is invalid.
- First-room slot id is `0`.
- First-room slot kind is `Local`.
- First-room slot actor is `EntityId{1}` (`player`).
- `Observer` exists without actor binding.
- `Remote` is a reserved multiplayer-ready value and does not imply socket
  ownership.
- `stableName` is diagnostic/debug identity, not a network credential.

## Ownership And Forbidden Data

Player slots do not own raw input devices, socket handles, UI focus, movement
state, or app user profiles. They own only deterministic authority-facing slot
facts.

## Save Replay Multiplayer

`PlayerSlot` is save truth and command-authority input. Replay and multiplayer
commands use slot id plus actor id; raw input source never enters this value.

## Completion Criteria

The header locks invalid slot rules, local slot 0, slot kinds, fields, and
helper signatures without depending on app, renderer, network, or old iggy code.
