# `src/runtime/world/EntityState.hpp`

Updated: 2026-06-20

Exact purpose: declare the authoritative per-entity state record stored by
`WorldState`, including stable identity, semantic kind, transform/bounds,
targetability, interaction metadata, item metadata, and objective references.

## Build Position

- priority rank: 45
- tier: Tier 3: Player World Authority
- module: `src/runtime/world`
- file kind: `header`

## Ownership

This file owns the entity value schema only. `WorldState` owns allocation,
lookup, and mutation. Gameplay systems may read `EntityState`, but must mutate
entity records through `WorldState` APIs.

This file must not own command admission, target query policy, inventory
mutation, objective completion, renderer ids, raw input, sockets, app paths, or
old `/Users/kogaryu/iggy` adapters.

## Required Header Shape

Repo path:

```text
src/runtime/world/EntityState.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

Do not include runtime systems, app code, projection, renderer, tests, network
transport, or old `iggy` headers.

## Required Enums

Declare:

```cpp
enum class EntityKind : std::uint8_t {
  Unknown,
  Player,
  Pickup,
  Door,
  Marker,
  Npc,
};

enum class TargetAction : std::uint8_t {
  Interact,
  Inspect,
  Move,
};
```

Semantics:

- `Unknown` is invalid for loaded runtime entities.
- `Player` is the controllable actor kind for first-room slot 0.
- `Pickup` may carry item metadata and a pickup interaction.
- `Marker` can be a tactical movement/selection target but is not collectible.
- `Npc` and `Door` are declared complete-build entity kinds; they must compile
  but need not appear in first-room data.
- Entity kinds describe broad categories only. Activation behavior is represented
  by target actions and canonical `InteractionDefinition`, not by an
  `EntityKind`.

## Required Metadata Types

Declare these exact value types:

```cpp
struct EntityTargeting {
  bool targetable = false;
  std::vector<TargetAction> actions;
};
```

`EntityTargeting::actions` stores actions in fixture/document order. Duplicate
actions are invalid during package loading/validation. Runtime helpers preserve
the vector order and do not sort it.

## Required `EntityState` Fields

Declare:

```cpp
struct EntityState {
  EntityId id;
  std::string stableName;
  EntityKind kind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 localBounds;
  bool active = true;
  bool persistent = true;
  EntityTargeting targeting;
  InteractionDefinition interaction;
};
```

Field semantics:

- `id`: stable runtime id; `EntityId{0}` is invalid.
- `stableName`: unique within `WorldState`; non-empty for package-loaded
  entities.
- `kind`: semantic gameplay kind, not renderer/model kind.
- `transform`: authoritative world transform.
- `localBounds`: local collision/debug bounds. Complete-build targeting and
  reach use the entity transform position for entity target points, not
  `localBounds`.
- `active`: inactive entities remain in the vector but are skipped by target
  queries and interaction execution.
- `persistent`: persisted in save/load. First-room entities are persistent.
- `targeting`: action-level targetability facts for targeting/admission.
- `interaction`: canonical interaction/effect metadata owned by
  `runtime/interaction/InteractionDefinition.hpp` and consumed by
  `InteractionSystem`, save/load, state hash, and replay.

## First-Room Entity Metadata

The fixture must seed these exact records after id allocation:

```text
EntityId(1)
stableName=player
kind=Player
position=(0.000,0.000,0.000)
active=true
persistent=true
targeting.targetable=false
interaction.kind=None
interaction.primaryEffect=None
interaction.itemId=""
interaction.itemCount=0
interaction.objectiveId=""
interaction.repeatable=false
interaction.deactivateTargetOnSuccess=false
```

```text
EntityId(2)
stableName=gold_key
kind=Pickup
position=(3.000,0.000,0.000)
active=true
persistent=true
targeting.targetable=true
targeting.actions=[Interact, Inspect]
interaction.kind=Pickup
interaction.primaryEffect=AddItemToInventory
interaction.itemId=gold_key
interaction.itemCount=1
interaction.objectiveId=collect_gold_key
interaction.repeatable=false
interaction.deactivateTargetOnSuccess=true
```

```text
EntityId(3)
stableName=tactical_marker_alpha
kind=Marker
position=(2.000,0.000,1.000)
active=true
persistent=true
targeting.targetable=true
targeting.actions=[Move, Inspect]
interaction.kind=None
interaction.primaryEffect=None
interaction.itemId=""
interaction.itemCount=0
interaction.objectiveId=""
interaction.repeatable=false
interaction.deactivateTargetOnSuccess=false
```

## Required Helpers

Declare pure helpers:

```cpp
bool isTargetActionSupported(const EntityTargeting& targeting, TargetAction action);
bool isValidEntityKind(EntityKind kind);
bool hasValidEntityIdentity(const EntityState& entity);
```

Helpers must not mutate, allocate runtime ids, or inspect global state.
`isTargetActionSupported` scans `targeting.actions` from first to last and
returns true on the first exact action match. If `targeting.targetable` is
false, it returns false even when the vector contains an action.

## Invariants

- Runtime entities must have valid ids, non-empty stable names, valid kinds,
  finite transforms, valid finite bounds, and unique stable names in
  `WorldState`.
- Inactive entities may be saved and replayed but cannot be discovered as
  active targets.
- Renderer/model asset ids do not live here.
- Interaction `itemId` and `objectiveId` strings are stable ids, not localized
  display text.

## Save Replay Multiplayer

`EntityState` fields are authoritative save truth except derived caches. Replay
and multiplayer command references use `EntityId`, not vector index or
`stableName`. State hash must include id, stable name, kind, transform, bounds,
active/persistent flags, targetability actions, and canonical interaction
metadata in deterministic field order.

## Tests

`tests/unit/world_state_tests.cpp` must assert first-room entity records contain
the exact metadata above after seed insertion through `WorldState`.

## Completion Criteria

The header gives a builder all enum names, fields, defaults, and helper
signatures needed to implement first-room entity authority without renderer or
old-iggy dependencies.
