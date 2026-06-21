# `tests/unit/world_state_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove `EntityState` and `WorldState` authority semantics,
including deterministic ids, lookup, mutation, reset, and no-mutation failure
behavior.

## Build Position

- priority rank: 48
- tier: Tier 3: Player World Authority
- module: `tests/unit`
- file kind: `test`

## Required Includes

```cpp
#include "runtime/world/WorldState.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
```

Use the project test assertion style once established. Do not use renderer,
app tools, filesystem, wall-clock timing, network, or old iggy code.

## Shared Fixture Setup

Define test helpers:

```cpp
EntityState makePlayer();
EntityState makeGoldKey();
EntityState makeMarker();
Transform3 transformAt(float x, float y, float z);
Aabb3 unitBounds();
```

Helpers must create the first-room metadata from
`EntityState.hpp` exactly:

- `player`: kind `Player`, position `(0,0,0)`, not targetable.
- `gold_key`: kind `Pickup`, position `(3,0,0)`, targetable interact/inspect,
  item `gold_key:1`, pickup interaction, deactivates on success, objective
  `collect_gold_key`.
- `tactical_marker_alpha`: kind `Marker`, position `(2,0,1)`, targetable move
  and inspect, not collectible.

## Required Test Cases

### `default_world_starts_empty_with_next_id_one`

Setup: default `WorldState`.

Assert:

- `empty() == true`;
- `size() == 0`;
- `nextEntityId() == EntityId{1}`.

### `add_entity_assigns_ids_in_order`

Setup: add player, gold key, marker with invalid ids.

Assert:

- statuses are `WorldStatus::Ok`;
- ids are `1`, `2`, `3`;
- vector order is player, gold_key, tactical_marker_alpha;
- next id is `4`.

### `seed_entity_preserves_explicit_fixture_ids`

Setup: seed player id 1, gold key id 2, marker id 3.

Assert same order and cursor `4`.

### `find_by_id_and_stable_name_return_expected_records`

Setup: seeded first-room world.

Assert:

- id 2 finds `gold_key`;
- stable name `gold_key` finds id 2;
- missing id returns null;
- missing stable name returns null.

### `upsert_replaces_same_id_without_reordering`

Setup: seeded world, copy gold key, set `active=false`, upsert.

Assert:

- status `Ok`;
- index of id 2 unchanged;
- `gold_key.active == false`;
- other entities unchanged.

### `update_transform_validates_before_mutation`

Setup: seeded world.

Assert:

- updating player to `(2,0,0)` succeeds;
- updating missing id returns `MissingEntity`;
- updating with NaN component returns `InvalidTransform`;
- failed updates leave previous transform unchanged.

### `set_active_rejects_missing_id_without_mutation`

Setup: seeded world.

Assert:

- setting gold key inactive succeeds;
- setting missing id returns `MissingEntity`;
- vector size and cursor unchanged after failure.

### `duplicate_stable_name_is_rejected`

Setup: add player, then add another entity named `player`.

Assert:

- second add returns `DuplicateStableName`;
- size remains `1`;
- next id remains unchanged from before failed add.

### `invalid_entity_data_is_rejected_before_insert`

Cases:

- empty stable name returns `InvalidName`;
- kind `Unknown` returns `InvalidKind`;
- non-finite transform returns `InvalidTransform`;
- invalid bounds returns `InvalidBounds`.

For each case assert size/cursor unchanged.

### `reset_from_baseline_restores_exact_copy`

Setup: baseline first-room world, mutated copy with moved player and inactive
gold key.

Act: `resetFromBaseline(baseline)`.

Assert:

- player position restored `(0,0,0)`;
- gold key active true;
- vector order/id/name metadata matches baseline;
- next id matches baseline.

## Completion Criteria

The tests lock the exact world authority behavior builders need before movement,
targeting, interaction, save/load, and replay systems are implemented.
