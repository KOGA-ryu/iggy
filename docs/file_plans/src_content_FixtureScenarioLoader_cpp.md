# `src/content/FixtureScenarioLoader.cpp`

Updated: 2026-06-20

Exact purpose: implement parsing of the first-build scenario TOML subset into
`FixtureScenarioSeed`.

## Build Position

- priority rank: 40
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `source`

## Required Include Order

```cpp
#include "content/FixtureScenarioLoader.hpp"

#include <sstream>
```

Do not include runtime/session, app, renderer, tests, or old iggy code.

## Parsing Algorithm

1. Tokenize line-by-line enough to support the documented fixture TOML.
2. Recognize `[scenario]`, `[defaults]`, `[[players]]`, `[[entities]]`, and
   `[[objectives]]`.
3. Reject unknown table names as `UnsupportedKey`.
4. Reject unknown keys in recognized tables as `UnsupportedKey`.
5. Reject missing required keys as `MissingRequiredKey`.
6. Convert enums exactly:
   - clock: `Normal` only in the first complete build;
   - camera: `FirstPerson`, `ThirdPerson`, `TacticalOverhead`;
   - player kind: `Local`, `Remote`, `Ai`, `Observer`;
   - entity kind: `Player`, `Pickup`, `Door`, `Marker`, `Npc`;
   - target actions: `Interact`, `Inspect`, `Move`;
   - interaction: `Pickup`, `OpenDoor`, `Inspect`, `Activate`;
   - objective status: `Active`, `Complete`, `Failed`.
7. Convert vectors `[x, y, z]` into `Vec3`.
8. Build `Transform3` from `position` with identity rotation/scale.
9. Build `Aabb3` from `bounds_min` and `bounds_max`.
10. Build canonical `InteractionDefinition`:
   - absent `interaction` means `kind=None`, `primaryEffect=None`, empty ids,
     `itemCount=0`, `repeatable=false`,
     `deactivateTargetOnSuccess=false`;
   - `interaction="Pickup"` means `kind=Pickup`,
     `primaryEffect=AddItemToInventory`, `itemId` from TOML `item_id`,
     `itemCount` from TOML `item_count`, `objectiveId` from TOML
     `objective_ref`, `repeatable=false`, and
     `deactivateTargetOnSuccess` from TOML `deactivate_on_success`.
11. Preserve entity order exactly.
12. Store `[defaults].initial_clock` as `FixtureScenarioSeed::initialClockMode`;
    accepted value is exactly `Normal`. Store `default_realtime_camera` and
    `default_tactical_camera` in the scenario seed camera fields. These values
    are not copied into `RuntimeConfig`.

## Exact Inline Schema

Accepted tables and keys are:

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

Nested entity/objective subtables are not accepted in the first complete build.
`position`, `bounds_min`, `bounds_max`, and `target_actions` are inline arrays
on the containing row.

Required keys:

- `[scenario]`: `id`.
- `[defaults]`: every accepted defaults key.
- `[[players]]`: `slot`, `kind`, `actor`.
- `[[entities]]`: `stable_name`, `kind`, `active`, `persistent`, `position`,
  `bounds_min`, `bounds_max`, `targetable`, `target_actions`.
- Pickup rows with `interaction="Pickup"`: `item_id`, `item_count`,
  `objective_ref`, `deactivate_on_success`.
- `[[objectives]]`: every accepted objective key.

Missing keys return `MissingRequiredKey` with diagnostic
`scenario.missing_required_key`.

## Failure Behavior

Return `ParseError` for malformed syntax, `InvalidNumber` for bad numeric
values, `InvalidEnum` for unrecognized enum strings, `MissingScenarioId` when
the scenario id is absent, `MissingRequiredKey` for absent required table keys,
and `UnsupportedKey` for extra tables/keys.

`InspectOnly` is not an accepted interaction or target-action token. A fixture
using `InspectOnly` returns `InvalidEnum` with diagnostic
`scenario.invalid_enum`.

Unsupported `[defaults].initial_clock` values, including `Paused` and `Slow`,
return scenario parse status `InvalidEnum`; the public package-load boundary
maps that failure to `PackageLoadStatus::InvalidEnum` with diagnostic
`scenario.invalid_enum`. The diagnostic domain is `DiagnosticDomain::Content`.
The diagnostic location is the exact physical source location represented as
`DiagnosticLocation { file, line, column }`: `file` is the scenario input
path/name supplied to the loader, `line` is the 1-based source line containing
the invalid `initial_clock` key, and `column` is the 1-based column of the first
character of the `initial_clock` key. The location must not point at the
`[defaults]` table header, the invalid enum value, or a later runtime command
field. The diagnostic message names the bad value and the expected first-build
value `Normal`.
Tactical and paused states remain reachable only by runtime commands after
initialization, not by the first-room seed.

Diagnostics use codes:

- `scenario.parse_error`
- `scenario.unsupported_key`
- `scenario.invalid_number`
- `scenario.invalid_enum`
- `scenario.missing_id`
- `scenario.missing_required_key`

## First-Room Output

The parser must output the exact first-room seed values documented in
`fixtures_demos_first_room_scenario_iggy3d_toml.md`, including `gold_key`
canonical pickup interaction metadata and `tactical_marker_alpha` move/inspect
targetability. The output seed has `initialClockMode=Normal`,
`defaultRealtimeCamera=ThirdPerson`, and
`defaultTacticalCamera=TacticalOverhead`.

## Compute Cost

Parsing is O(text bytes + rows). It does not mutate runtime state.

## Completion Criteria

The source parses the first-room fixture and fails deterministically on invalid
tables, keys, numbers, and enum values.
