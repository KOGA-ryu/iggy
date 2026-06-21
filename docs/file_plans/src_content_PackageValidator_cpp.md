# `src/content/PackageValidator.cpp`

Updated: 2026-06-20

Exact purpose: implement package and scenario seed validation before runtime
session creation.

## Build Position

- priority rank: 36
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `source`

## Required Include Order

```cpp
#include "content/PackageValidator.hpp"

#include <algorithm>
```

Do not include runtime/session, renderer, app, tests, or old iggy headers.

## Validation Algorithm

Apply the order from `PackageValidator.hpp` exactly. For each failure:

1. append one `Diagnostic` with stable code;
2. return immediately with matching `PackageValidationStatus`;
3. do not mutate manifest or seed values.

Diagnostic codes:

- `package.missing_id`
- `package.wrong_id`
- `package.unsupported_schema`
- `package.invalid_scenario_path`
- `package.old_iggy_dependency`
- `scenario.missing_id`
- `scenario.wrong_id`
- `scenario.missing_stable_name`
- `scenario.duplicate_stable_name`
- `scenario.missing_player_binding`
- `scenario.missing_player_entity`
- `scenario.non_finite_transform`
- `scenario.invalid_bounds`
- `scenario.missing_gold_key_interaction`
- `scenario.invalid_objective_condition`

## Exact First-Room Checks

- package id equals `iggy3d.first_room`;
- scenario id equals `first_room.runtime_loop`;
- exactly one local player slot id `0` binds to stable name `player`;
- entities include `player`, `gold_key`, `tactical_marker_alpha`;
- `gold_key` kind `Pickup`, position `(3.000,0.000,0.000)`, target actions
  Interact/Inspect, canonical interaction `kind=Pickup`,
  `primaryEffect=AddItemToInventory`, `itemId=gold_key`, `itemCount=1`,
  `objectiveId=collect_gold_key`, `repeatable=false`,
  `deactivateTargetOnSuccess=true`;
- objective id `collect_gold_key`, initial Active, condition slot 0 inventory
  contains `gold_key:1`.

## Compute Cost

Validation is O(entity count + objective count + asset count), except duplicate
stable-name detection is O(entity count squared) in the first complete build.
Duplicate detection uses a deterministic vector scan in scenario entity order:

1. for each entity index `i`, require non-empty `stableName`; empty names return
   `MissingStableName` with diagnostic code `scenario.missing_stable_name`, a
   location for row/index `i`, and no duplicate comparisons for that row;
2. compare against every prior entity index `j < i`;
3. on the first exact duplicate stable name, return `DuplicateStableName` with
   diagnostic code `scenario.duplicate_stable_name`;
4. diagnostic location identifies the later duplicate row/index and the message
   includes the duplicate stable name;
5. no unordered container iteration may decide which duplicate is reported.

## Tests

Package loader tests must assert each validation status by modifying a valid
fixture seed one failure at a time.

## Completion Criteria

Validator rejects every audited failure mode with stable status/diagnostic code
and accepts the first-room fixture.
