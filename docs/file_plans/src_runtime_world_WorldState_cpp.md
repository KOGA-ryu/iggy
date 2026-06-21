# `src/runtime/world/WorldState.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic entity allocation, lookup, validation,
and mutation for `WorldState`.

## Build Position

- priority rank: 47
- tier: Tier 3: Player World Authority
- module: `src/runtime/world`
- file kind: `source`

## Required Include Order

```cpp
#include "runtime/world/WorldState.hpp"

#include <algorithm>
#include <utility>
```

Do not include app, projection, renderer, networking, tests, or old `iggy`
headers.

## Internal Helper Algorithms

Implement these unnamed-namespace helpers:

```cpp
bool isValidName(const std::string& name);
WorldStatus validateEntityForInsert(
    const std::vector<EntityState>& entities,
    const EntityState& entity,
    EntityId existingIdToIgnore);
std::vector<EntityState>::iterator findEntityIterator(
    std::vector<EntityState>& entities,
    EntityId id);
std::vector<EntityState>::const_iterator findEntityIterator(
    const std::vector<EntityState>& entities,
    EntityId id);
EntityId nextAfter(EntityId id);
```

`existingIdToIgnore` is invalid for add/seed and the current entity id for
upsert replacement.

## Validation Order

Every mutating function validates before mutation:

1. entity id validity when required by the API;
2. non-empty stable name;
3. duplicate stable name;
4. duplicate explicit id for add/seed;
5. valid entity kind;
6. finite valid transform;
7. finite valid bounds;
8. existing entity lookup for update APIs.

Return the corresponding `WorldStatus` for the first failure.

## Function Behavior

### Constructor

Initializes:

```text
entities_ = empty
nextEntityId_ = EntityId{1}
```

### `addEntity`

Algorithm:

1. copy incoming entity locally;
2. if id is invalid, assign `nextEntityId_` to the local copy;
3. validate local copy against current vector;
4. on failure return status and leave vector/cursor unchanged;
5. append local copy;
6. advance `nextEntityId_` to max(current cursor, local id + 1);
7. return `Ok`, id, and appended index.

### `seedEntity`

Algorithm:

1. reject invalid incoming id as `InvalidEntityId`;
2. validate duplicate id/name/kind/transform/bounds;
3. append local copy;
4. advance cursor beyond seeded id;
5. return `Ok`.

### `upsertEntity`

Algorithm:

1. if incoming id is invalid, call `addEntity`;
2. find existing entity by id;
3. if missing, validate and append as explicit-id seed;
4. if found, validate stable-name uniqueness ignoring that id;
5. replace the entity at the same vector index;
6. keep vector order stable.

### `findById` / `findByStableName`

Use linear scan in vector order. Return `nullptr` when not found. Do not mutate
or allocate. These functions return const pointers only.

### `updateTransform`

Validate id exists and transform is finite before assignment. Return
`InvalidTransform` before `MissingEntity` only if the transform itself is
invalid; otherwise missing id returns `MissingEntity`.

### `setActive`

Find by id, set flag, return `Ok`. Missing id returns `MissingEntity`.

### `resetFromBaseline`

Copy baseline vector and cursor exactly. No partial reset behavior.

## Compute Cost

- Add/seed/upsert validation is O(entity count).
- Lookup is O(entity count).
- Reset is O(entity count).
- The first complete build uses vector scans only. Index caches are not part of
  this file plan.

## No-Mutation-On-Failure

Unit tests must snapshot size, cursor, and relevant entity fields before each
failing mutation and assert they are unchanged after failure.

## Completion Criteria

The implementation is deterministic, side-effect free outside `WorldState`,
and passes the world-state unit tests without old-iggy or renderer code.
