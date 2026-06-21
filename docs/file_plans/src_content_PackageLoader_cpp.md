# `src/content/PackageLoader.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic loading/parsing of
`package.iggy3d.toml` and its scenario file into `PackageManifest` and
`FixtureScenarioSeed`.

## Build Position

- priority rank: 34
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `source`

## Required Include Order

```cpp
#include "content/PackageLoader.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
```

Use a deterministic first-build TOML subset parser implemented in this source
or a repo-local content helper. Do not pull in old iggy parser code.

## Parsing Algorithm

1. Read package text.
2. Parse `[package]` scalar keys exactly:
   - `id`
   - `schema_version`
   - `required_runtime_schema`
   - `scenario`
3. Parse zero or more `[[assets]]` tables with keys `id` and `path`.
4. Reject unknown package tables or keys as `UnsupportedKey`.
5. Validate scenario path is relative, has filename `scenario.iggy3d.toml`,
   does not contain `..`, and does not contain `/Users/kogaryu/iggy`.
6. Read scenario text from package directory plus scenario path.
7. Delegate scenario text parsing to `FixtureScenarioLoader`.
8. Map delegated scenario parse failures exactly:
   - `ScenarioLoadStatus::ParseError` -> `PackageLoadStatus::ParseError`;
   - `UnsupportedKey` -> `UnsupportedKey`;
   - `MissingRequiredKey` -> `MissingRequiredKey`;
   - `MissingScenarioId` -> `MissingScenarioId`;
   - `InvalidNumber` -> `InvalidNumber`;
   - `InvalidEnum` -> `InvalidEnum`;
   - `InvalidPath` -> `InvalidPath`.
9. Return manifest plus scenario seed only when status is `Ok`.

## Scenario TOML Tables To Recognize

The loader accepts only the inline scenario schema documented in
`fixtures_demos_first_room_scenario_iggy3d_toml.md`:

- `[scenario]`
- `[defaults]`
- `[[players]]`
- `[[entities]]`
- `[[objectives]]`

Accepted keys:

- `[scenario]`: `id`.
- `[defaults]`: `fixed_tick_rate_hz`, `interaction_range_meters`,
  `movement_distance_meters`, `slow_time_scale`, `initial_clock`,
  `default_realtime_camera`, `default_tactical_camera`.
- `[[players]]`: `slot`, `kind`, `actor`.
- `[[entities]]`: `stable_name`, `kind`, `active`, `persistent`, `position`,
  `bounds_min`, `bounds_max`, `targetable`, `target_actions`, `item_id`,
  `item_count`, `interaction`, `deactivate_on_success`, `objective_ref`.
- `[[objectives]]`: `id`, `initial_status`, `condition`, `player_slot`,
  `item_id`, `item_count`, `complete_status`.

Unsupported nested tables such as `[entities.bounds]`,
`[entities.position]`, `[entities.targeting]`, `[entities.interaction]`,
`[entities.item]`, or `[objectives.condition]` return `UnsupportedKey`.

Required-key behavior:

- Missing `[scenario].id` returns `MissingScenarioId`.
- Missing any `[defaults]` key returns `MissingRequiredKey`.
- Missing `slot`, `kind`, or `actor` in a player row returns
  `MissingRequiredKey`.
- Missing `stable_name`, `kind`, `position`, `bounds_min`, `bounds_max`,
  `active`, `persistent`, `targetable`, or `target_actions` in an entity row
  returns `MissingRequiredKey`.
- For `kind="Pickup"` with `interaction="Pickup"`, missing `item_id`,
  `item_count`, `objective_ref`, or `deactivate_on_success` returns
  `MissingRequiredKey`.
- Missing objective `id`, `initial_status`, `condition`, `player_slot`,
  `item_id`, `item_count`, or `complete_status` returns `MissingRequiredKey`.

Type conversion:

- strings require TOML double-quoted strings;
- booleans accept only lowercase `true` or `false`;
- integer fields must fit their target unsigned type;
- decimal fields parse to `float` and must be finite;
- unrecognized enum strings return `InvalidEnum` with diagnostic
  `scenario.invalid_enum`;
- non-finite or malformed numeric values return `InvalidNumber` with diagnostic
  `scenario.invalid_number`;
- `position`, `bounds_min`, and `bounds_max` are exactly three-number inline
  arrays;
- `target_actions` is an inline string array preserving document order.

## Diagnostics

Use stable diagnostic codes:

- `package.missing_file`
- `package.read_failed`
- `package.unsupported_key`
- `package.invalid_path`
- `package.parse_error`
- `package.missing_required_key`
- `scenario.read_failed`
- `scenario.parse_error`
- `scenario.unsupported_key`
- `scenario.missing_required_key`
- `scenario.missing_id`
- `scenario.invalid_number`
- `scenario.invalid_enum`

Diagnostics include file path and line when known.

## Compute Cost

Parsing is O(file bytes + table count). No runtime mutation occurs.

## Tests

Package loader tests must cover valid first-room package, unsupported key,
invalid scenario path, and scenario parse failure.

## Completion Criteria

The source implements the documented TOML subset, returns stable statuses, and
never creates runtime state directly.
