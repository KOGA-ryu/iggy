# `tests/unit/package_loader_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove package manifest parsing, scenario seed parsing, package
validation statuses, first-room fixture facts, and loader failure behavior.

## Build Position

- priority rank: 41
- tier: Tier 2: Content And Configuration
- module: `tests/unit`
- file kind: `test`

## Required Includes

```cpp
#include "content/PackageLoader.hpp"
#include "content/PackageValidator.hpp"
```

Tests may use string literals for TOML and fixture paths under
`fixtures/demos/first_room`. Do not use renderer, wall-clock timing, network, or
old iggy code.

## Shared Valid TOML

Use the exact package and scenario TOML from the fixture docs as string
fixtures. The valid parse must include:

- `iggy3d.first_room`;
- `first_room.runtime_loop`;
- `gold_key`;
- `tactical_marker_alpha`;
- `collect_gold_key`;
- `1.500`;
- `3.000`;
- `ThirdPerson`;
- `TacticalOverhead`.

## Required Test Cases

### `parse_valid_package_and_scenario_text`

Call `parsePackageText(validPackage, validScenario, "fixtures/demos/first_room")`.

Assert:

- load status Ok;
- manifest package id `iggy3d.first_room`;
- scenario path `scenario.iggy3d.toml`;
- scenario id `first_room.runtime_loop`;
- three entities in order;
- one objective.

### `scenario_seed_contains_first_room_defaults`

Assert config fields: tick 20, range `1.500`, movement `3.000`, slow scale
`0.250`. Assert scenario seed initial modes: `initialClockMode=Normal`,
`defaultRealtimeCamera=ThirdPerson`, and
`defaultTacticalCamera=TacticalOverhead`.

### `scenario_seed_contains_player_gold_key_and_marker`

Assert:

- player seed stable name `player`, kind Player, position `(0,0,0)`;
- gold key seed kind Pickup, position `(3,0,0)`, target actions
  Interact/Inspect, interaction `kind=Pickup`,
  `primaryEffect=AddItemToInventory`, `itemId=gold_key`, `itemCount=1`,
  `objectiveId=collect_gold_key`, `repeatable=false`,
  `deactivateTargetOnSuccess=true`;
- marker seed stable name `tactical_marker_alpha`, kind Marker, position
  `(2,0,1)`, target actions Move/Inspect, no item id.

### `validator_accepts_valid_first_room`

Call `validatePackage` and assert status Ok and no error diagnostics.

### `validator_rejects_missing_package_id`

Remove package id. Assert `MissingPackageId` and diagnostic
`package.missing_id`.

### `validator_rejects_wrong_scenario_id`

Change scenario id. Assert `WrongScenarioId`.

### `validator_rejects_duplicate_stable_name`

Duplicate `gold_key` stable name on a later entity row. Assert
`DuplicateStableName`, diagnostic `scenario.duplicate_stable_name`, diagnostic
location identifying the later duplicate row/index, and message text containing
`gold_key`.

### `validator_rejects_empty_stable_name`

Set a later entity row `stableName` to empty after successful parsing or build
the seed manually. Assert `MissingStableName`, diagnostic
`scenario.missing_stable_name`, diagnostic location identifying that row, and no
session creation from this invalid package.

### `validator_rejects_missing_player_binding`

Remove `[[players]]` or actor binding. Assert `MissingPlayerBinding`.

### `validator_rejects_non_finite_transform`

Set player position to NaN/inf token accepted by parser test fixture or build
seed manually. Assert `NonFiniteTransform`.

### `validator_rejects_invalid_bounds`

Set bounds min greater than max. Assert `InvalidBounds`.

### `validator_rejects_missing_gold_key_interaction`

After successful loading, mutate the gold key interaction to remove canonical
metadata such as `interaction.kind=Pickup`, `itemId=gold_key`,
`itemCount=1`, `objectiveId=collect_gold_key`, or
`deactivateTargetOnSuccess=true`. Assert `MissingGoldKeyInteraction` and
diagnostic `scenario.missing_gold_key_interaction`.

### `validator_rejects_invalid_objective_condition`

Change objective item id or player slot. Assert `InvalidObjectiveCondition`.

### `loader_rejects_old_iggy_absolute_scenario_path`

Set package `scenario = "/Users/kogaryu/iggy/foo.toml"`. Assert load status
`InvalidPath` and diagnostic `package.invalid_path`.

### `validator_rejects_old_iggy_asset_dependency`

Set a relative asset path or metadata string to contain `old_iggy_dependency`.
Assert validator status `OldIggyDependency` and diagnostic
`package.old_iggy_dependency`.

### `loader_rejects_unsupported_key`

Add an unknown key to `[package]` or `[[entities]]`. Assert load status
`UnsupportedKey` and diagnostic `package.unsupported_key` for package keys or
`scenario.unsupported_key` for scenario keys.

### `loader_rejects_invalid_scenario_path`

Set scenario to `../scenario.iggy3d.toml` or absolute path. Assert
`InvalidPath` and diagnostic `package.invalid_path`.

### `loader_rejects_missing_required_scenario_key`

Remove `bounds_min` from `gold_key`. Assert load status `MissingRequiredKey`
and diagnostic `scenario.missing_required_key`.

### `loader_rejects_missing_pickup_objective_ref`

Remove `objective_ref` from the `gold_key` pickup row. Assert load status
`MissingRequiredKey`, diagnostic `scenario.missing_required_key`, and a
diagnostic location on the later `gold_key` entity row/key context. Do not call
`PackageValidator` for this non-Ok result.

### `loader_rejects_inspect_only_token`

Set one entity `interaction = "InspectOnly"` in scenario text. Assert load
status `PackageLoadStatus::InvalidEnum`, diagnostic `scenario.invalid_enum`, and no
`PackageValidator` or session creation for this non-Ok parse result.

### `loader_rejects_unsupported_initial_clock_with_location`

Use a scenario text with deterministic lines:

```toml
[scenario]
id = "first_room.runtime_loop"

[defaults]
fixed_tick_rate_hz = 20
interaction_range_meters = 1.500
movement_distance_meters = 3.000
slow_time_scale = 0.250
initial_clock = "Paused"
default_realtime_camera = "ThirdPerson"
default_tactical_camera = "TacticalOverhead"
```

Append the remaining valid `[[players]]`, `[[entities]]`, and `[[objectives]]`
rows from the shared valid scenario text so no other validation failure is
introduced. Pass scenario input name
`inline/scenario_invalid_initial_clock.iggy3d.toml`.

Assert:

- load status `PackageLoadStatus::InvalidEnum`;
- diagnostic domain `Content`;
- diagnostic code `scenario.invalid_enum`;
- diagnostic location file
  `inline/scenario_invalid_initial_clock.iggy3d.toml`;
- diagnostic line `9`, the line containing `initial_clock = "Paused"`;
- diagnostic column `1`, the first character of `initial_clock` using the
  1-based convention;
- diagnostic message contains both `Paused` and `Normal`;
- no accepted `FixtureScenarioSeed`, no package validation, and no session
  creation from this non-Ok parse result.

### `loader_maps_delegated_invalid_number`

Set `movement_distance_meters = "far"` or a non-finite numeric token in scenario
text. Assert load status `InvalidNumber` and diagnostic
`scenario.invalid_number`.

### `loader_maps_delegated_missing_scenario_id`

Remove `[scenario].id`. Assert load status `MissingScenarioId` and diagnostic
`scenario.missing_id`.

### `non_ok_load_payload_is_ignored`

Create a non-Ok package load result by removing a required scenario key. Assert
test code does not pass `manifest` or `scenario` payload fields to
`PackageValidator` or session creation unless `status == Ok`; only status and
diagnostics are authoritative after failure.

## Completion Criteria

Tests lock actual TOML schema, parser statuses, validator statuses, and
first-room acceptance facts before runtime implementation begins.
