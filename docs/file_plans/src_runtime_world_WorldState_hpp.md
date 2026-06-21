# `src/runtime/world/WorldState.hpp`

Updated: 2026-06-20

Exact purpose: declare the authoritative deterministic entity collection,
entity id allocation policy, lookup APIs, and validated world mutation API.

## Build Position

- priority rank: 46
- tier: Tier 3: Player World Authority
- module: `src/runtime/world`
- file kind: `header`

## Ownership

`WorldState` owns entity storage, allocation, lookup, and entity-field mutation.
Other runtime systems must request entity changes through this API. This file
does not own command admission, interaction effects, save encoding, projection,
renderer ids, raw input, sockets, app paths, or old `/Users/kogaryu/iggy`
adapters.

## Required Header Shape

Repo path:

```text
src/runtime/world/WorldState.hpp
```

Required includes:

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/world/EntityState.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Status Enum

Declare:

```cpp
enum class WorldStatus : std::uint8_t {
  Ok,
  InvalidEntityId,
  DuplicateStableName,
  MissingEntity,
  InvalidTransform,
  InvalidBounds,
  InvalidName,
  InvalidKind,
};
```

First failing validation reason wins for mutation APIs.

## Required Result Types

Declare:

```cpp
struct WorldEntityResult {
  WorldStatus status = WorldStatus::Ok;
  EntityId id;
  std::size_t index = 0;
};

struct WorldMutationResult {
  WorldStatus status = WorldStatus::Ok;
  EntityId id;
};
```

Do not wrap these result types in `Result<T>` for the first complete build.
World mutation legality is represented directly by `WorldStatus`.

## Required `WorldState` Shape

Declare this class:

```cpp
class WorldState {
public:
  WorldState();

  const std::vector<EntityState>& entities() const;
  EntityId nextEntityId() const;
  bool empty() const;
  std::size_t size() const;

  WorldEntityResult addEntity(EntityState entity);
  WorldEntityResult seedEntity(EntityState entity);
  WorldEntityResult upsertEntity(EntityState entity);

  const EntityState* findById(EntityId id) const;
  const EntityState* findByStableName(const std::string& stableName) const;

  WorldMutationResult updateTransform(EntityId id, const Transform3& transform);
  WorldMutationResult setActive(EntityId id, bool active);

  void clear();
  void resetFromBaseline(const WorldState& baseline);

private:
  std::vector<EntityState> entities_;
  EntityId nextEntityId_ = EntityId{1};
};
```

Private field names are exact for the first complete build. Canonical storage is
`entities_` in deterministic allocation order plus `nextEntityId_`.

## Id Allocation Policy

- `EntityId{0}` is invalid.
- `WorldState()` starts with `nextEntityId = EntityId{1}`.
- `addEntity` assigns `nextEntityId` when incoming id is invalid.
- `seedEntity` requires a valid incoming id and advances `nextEntityId` to one
  greater than the maximum seeded id.
- First-room scenario order must produce ids:
  - `1`: `player`
  - `2`: `gold_key`
  - `3`: `tactical_marker_alpha`
- Vector index is diagnostic only and never replaces `EntityId`.

## Validation Rules

Before mutating storage, APIs validate in this order:

1. id validity for APIs that require existing ids;
2. stable name non-empty;
3. duplicate stable name detection, excluding same-id upsert;
4. valid `EntityKind`;
5. finite valid transform;
6. finite valid bounds;
7. existing entity lookup for update APIs.

If validation fails, the `WorldState` object remains byte-for-byte unchanged
except for temporary local values.

## Mutation Semantics

- `addEntity`: validates and appends a new record. Duplicate id or stable name
  rejects without mutation.
- `seedEntity`: same as add but requires explicit valid id; intended for
  validated fixture/session creation.
- `upsertEntity`: replaces same-id entity after full validation; if id is
  invalid, behaves like `addEntity`.
- `updateTransform`: replaces transform only after id exists and transform is
  finite.
- `setActive`: flips active flag only after id exists.
- `resetFromBaseline`: replaces the entire vector and id cursor with a trusted
  baseline copy.
- No public write-through lookup exists in the first complete build. Systems that
  need to change entity fields must call `updateTransform`, `setActive`,
  `upsertEntity`, or a new validated mutation API documented here in the same
  patch.

## Save Replay Multiplayer

The entity vector order, ids, next id cursor, transforms, bounds, active flags,
and metadata are save truth and replay hash input. Future multiplayer commands
refer to entities by `EntityId`; stable names are package/debug facts.

## Tests

`tests/unit/world_state_tests.cpp` must cover add, seed, upsert, find, transform
update, active toggle, deterministic first-room ids, duplicate stable-name
rejection, invalid transform rejection, invalid bounds rejection, missing id
rejection, and no-mutation-on-failure.

## Completion Criteria

The header gives exact storage, status, result, and API contracts so builders do
not invent world authority semantics.
