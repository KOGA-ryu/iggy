# `fixtures/demos/first_room/scenario.iggy3d.toml`

Updated: 2026-06-20

Exact purpose: define the deterministic first-room scenario fixture that seeds
player/world/clock/camera/objective state for the acceptance demo.

## Build Position

- priority rank: 38
- tier: Tier 2: Content And Configuration
- module: `fixtures`
- file kind: `fixture`

## Exact Repo Path

```text
fixtures/demos/first_room/scenario.iggy3d.toml
```

## Required TOML Content

```toml
[scenario]
id = "first_room.runtime_loop"

[defaults]
fixed_tick_rate_hz = 20
interaction_range_meters = 1.500
movement_distance_meters = 3.000
slow_time_scale = 0.250
initial_clock = "Normal"
default_realtime_camera = "ThirdPerson"
default_tactical_camera = "TacticalOverhead"

[[players]]
slot = 0
kind = "Local"
actor = "player"

[[entities]]
stable_name = "player"
kind = "Player"
active = true
persistent = true
position = [0.000, 0.000, 0.000]
bounds_min = [-0.250, 0.000, -0.250]
bounds_max = [0.250, 1.800, 0.250]
targetable = false
target_actions = []

[[entities]]
stable_name = "gold_key"
kind = "Pickup"
active = true
persistent = true
position = [3.000, 0.000, 0.000]
bounds_min = [-0.100, 0.000, -0.100]
bounds_max = [0.100, 0.100, 0.100]
targetable = true
target_actions = ["Interact", "Inspect"]
item_id = "gold_key"
item_count = 1
interaction = "Pickup"
deactivate_on_success = true
objective_ref = "collect_gold_key"

[[entities]]
stable_name = "tactical_marker_alpha"
kind = "Marker"
active = true
persistent = true
position = [2.000, 0.000, 1.000]
bounds_min = [-0.100, 0.000, -0.100]
bounds_max = [0.100, 0.100, 0.100]
targetable = true
target_actions = ["Move", "Inspect"]

[[objectives]]
id = "collect_gold_key"
initial_status = "Active"
condition = "InventoryContains"
player_slot = 0
item_id = "gold_key"
item_count = 1
complete_status = "Complete"
```

## Schema Rules

- Entity order is authoritative seed order and produces ids `1`, `2`, `3`.
- Positions and bounds must be finite.
- Bounds are local bounds relative to entity transform.
- `initial_clock = "Normal"` seeds initial `ClockState`; unsupported clock
  tokens are rejected by the scenario loader.
- Camera tokens seed runtime camera state through `FixtureScenarioSeed`, not
  through `RuntimeConfig`.
- `gold_key` metadata is required exactly as shown.
- `tactical_marker_alpha` is not collectible and has no item fields.
- Unknown keys reject unless the loader doc is updated with an explicit
  extension.
- This fixture uses the only accepted first-build scenario shape: inline
  `position`, `bounds_min`, `bounds_max`, `target_actions`, pickup fields, and
  objective condition fields on their containing rows.

## Acceptance Facts

This file must contain all acceptance scan facts:

- package scenario id `first_room.runtime_loop`;
- `gold_key`;
- `tactical_marker_alpha`;
- objective `collect_gold_key`;
- range `1.500`;
- movement `3.000`;
- camera `ThirdPerson`;
- camera `TacticalOverhead`.

## Tests

Package loader tests must parse this fixture and assert seed structs match the
TOML exactly.

## Completion Criteria

The scenario fixture is complete enough for content validation, session
creation, target discovery, OutOfRange rejection, movement, retry, pickup,
tactical mode, pause/step/resume, save/load, reset, replay, and summary proof.
