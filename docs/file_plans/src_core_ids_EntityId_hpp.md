# `src/core/ids/EntityId.hpp`

Updated: 2026-06-20

Exact purpose: declare the stable entity identifier used by world, commands, save, replay, projection, and multiplayer packets.

## Build Position

- priority rank: 11
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

## Ownership

This file owns:

- invalid id sentinel
- value comparison
- stable serialization helpers

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- C++ standard library only unless explicitly justified
- no `src/runtime`, `src/content`, `src/projection`, or app includes

## Data Contract

- 64-bit value
- `Invalid` value 0
- monotonic world allocation starting at 1 for fixture-created entities

## Semantics

- ids are not pointers
- ids remain stable across save/load and replay
- world allocation is deterministic from fixture order

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- Comparison, validity checks, and next-id helper are O(1).

## Diagnostics And Errors

- Entity id helpers do not emit diagnostics.
- Owning systems such as `WorldState`, command admission, save/load, and
  projection report invalid ids with their own statuses/diagnostics.

## Save Replay Multiplayer Notes

- `EntityId` values are durable only when stored by authoritative owners such as
  world state, commands, saves, replay logs, and projection records.
- This header owns the value representation and invalid sentinel.
- Multiplayer authority uses `EntityId` for actor and target references, never
  vector indexes or renderer ids.

## Tests And Verification

- world-state and command tests assert invalid id behavior, deterministic
  allocation from fixture order, lookup, and save/replay references;
- direct id tests are needed only for helper functions;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/ids/EntityId.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned Types
Declare stable entity id wrapper:

```cpp
struct EntityId {
  std::uint64_t value = 0;
};

inline constexpr EntityId kInvalidEntityId{};
```

Required helpers:
- equality and inequality;
- less-than for deterministic ordering only;
- `isValid(EntityId)`;
- `nextEntityId(EntityId)` returning `EntityId{id.value + 1}`.

Required signatures:

```cpp
bool operator==(EntityId lhs, EntityId rhs);
bool operator!=(EntityId lhs, EntityId rhs);
bool operator<(EntityId lhs, EntityId rhs);
bool isValid(EntityId id);
EntityId nextEntityId(EntityId id);
std::uint64_t toUint64(EntityId id);
EntityId entityIdFromUint64(std::uint64_t value);
```

### Semantics
- value `0` is invalid.
- first fixture entity id is `1`.
- ids are stable for a session, save/load, replay, projection, diagnostics, and multiplayer packets.
- entity id is not a vector index and not a renderer id.

### Dependencies
Allowed: `<cstdint>` only.
Forbidden: world state, runtime systems, content loader, renderer.

### Save Replay Multiplayer
Entity ids are durable save truth and replay hash input. Multiplayer authority uses these ids for actor and target references.

### Tests
World state tests must prove ids remain stable across add/find/upsert and reset fixture load.
