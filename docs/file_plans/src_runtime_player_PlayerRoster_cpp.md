# `src/runtime/player/PlayerRoster.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic player slot storage, lookup, actor
binding, and no-mutation-on-failure semantics.

## Build Position

- priority rank: 44
- tier: Tier 3: Player World Authority
- module: `src/runtime/player`
- file kind: `source`

## Required Include Order

```cpp
#include "runtime/player/PlayerRoster.hpp"

#include <algorithm>
#include <utility>
```

Do not include app, renderer, sockets, tests, or old iggy code.

## Validation Algorithm

For `addSlot` and `upsertSlot`, validate in this order:

1. invalid slot id -> `InvalidSlotId`;
2. invalid kind -> `InvalidSlotKind`;
3. invalid actor for actor-controlling kind -> `InvalidActor`;
4. duplicate slot id for add -> `DuplicateSlotId`;
5. same actor bound to a different actor-controlling slot -> `ActorAlreadyBound`.

For `rebindActor`, validate:

1. valid slot id;
2. existing slot;
3. slot kind controls actor;
4. new actor id valid;
5. new actor not bound to another controlling slot.

## Function Behavior

- `addSlot`: validates local copy, appends on success, preserves insertion
  order.
- `upsertSlot`: replaces same slot id at same index, or appends if missing.
- `findSlot`: const linear scan by slot id.
- `actorForSlot`: returns invalid entity id when missing or observer.
- `slotControlsActor`: returns true only for matching existing slot and actor.
- `clear`: empties vector.

## No-Mutation-On-Failure

Every failing operation leaves vector size, order, existing bindings, and
stable names unchanged.

## Compute Cost

All lookups and duplicate checks are O(slot count). First build has one local
slot. The first complete build uses deterministic vector scans only.

## Tests

`tests/unit/world_state_tests.cpp` must cover:

- add local slot 0 bound to `EntityId{1}`;
- duplicate slot id rejected;
- invalid actor rejected for local slot;
- observer accepts invalid actor;
- actor already bound rejected;
- rebind actor succeeds and preserves slot order;
- failed rebind leaves old actor.

## Completion Criteria

The implementation preserves deterministic slot order and exposes no app/input
or socket ownership.
