# `src/runtime/player/PlayerRoster.hpp`

Updated: 2026-06-20

Exact purpose: declare the authoritative deterministic collection of player
slots and player-to-actor bindings.

## Build Position

- priority rank: 43
- tier: Tier 3: Player World Authority
- module: `src/runtime/player`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/runtime/player/PlayerRoster.hpp
```

Required includes:

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "runtime/player/PlayerSlot.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Status Enum

Declare:

```cpp
enum class PlayerRosterStatus : std::uint8_t {
  Ok,
  InvalidSlotId,
  DuplicateSlotId,
  MissingSlot,
  InvalidSlotKind,
  InvalidActor,
  ActorAlreadyBound,
};
```

## Required Result Types

Declare:

```cpp
struct PlayerRosterResult {
  PlayerRosterStatus status = PlayerRosterStatus::Ok;
  PlayerSlotId slotId = kInvalidPlayerSlotId;
  std::size_t index = 0;
};
```

## Required Class API

Declare this class:

```cpp
class PlayerRoster {
public:
  const std::vector<PlayerSlot>& slots() const;
  bool empty() const;
  std::size_t size() const;

  PlayerRosterResult addSlot(PlayerSlot slot);
  PlayerRosterResult upsertSlot(PlayerSlot slot);
  PlayerRosterResult rebindActor(PlayerSlotId slotId, EntityId actor);

  const PlayerSlot* findSlot(PlayerSlotId slotId) const;
  EntityId actorForSlot(PlayerSlotId slotId) const;
  bool slotControlsActor(PlayerSlotId slotId, EntityId actor) const;

  void clear();
};
```

Canonical storage is `std::vector<PlayerSlot>` in deterministic slot order.

## Validation Rules

Before mutation:

1. slot id must be valid;
2. slot kind must not be `Unknown`;
3. actor id must be valid for `Local`, `Remote`, and `Ai`;
4. actor id may be invalid for `Observer`;
5. duplicate slot id rejects;
6. actor already bound to another actor-controlling slot rejects.

Failure leaves the roster unchanged.

No public write-through lookup exists in the first complete build. Systems that need
to change roster fields must call `addSlot`, `upsertSlot`, `rebindActor`, or a
new validated mutation API documented here in the same patch.

## First-Room Roster

The first-room seed must create exactly:

```text
slot id=0
kind=Local
actor=EntityId(1)
stableName=player0
```

`slotControlsActor(0, EntityId{1})` must return true.

## Save Replay Multiplayer

Roster vector order, slot ids, slot kinds, actor ids, and stable names are save
truth and replay hash input. Future multiplayer uses these same slot ids for
authority checks but transport/socket state remains outside the roster.

## Tests

World/session tests must prove local slot 0 is bound to player id 1. Command
admission tests use `slotControlsActor` for actor authority checks.

## Completion Criteria

The header gives exact status values, storage, APIs, and first-room slot facts.
