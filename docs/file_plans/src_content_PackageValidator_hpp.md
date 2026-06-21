# `src/content/PackageValidator.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic validation for loaded package manifest and
scenario seed data before session creation.

## Build Position

- priority rank: 35
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/content/PackageValidator.hpp
```

Required includes:

```cpp
#pragma once

#include <vector>

#include "core/diagnostics/Diagnostic.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "content/PackageManifest.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Status Enum

Declare:

```cpp
enum class PackageValidationStatus : std::uint8_t {
  Ok,
  MissingPackageId,
  WrongPackageId,
  UnsupportedSchemaVersion,
  InvalidScenarioPath,
  MissingScenarioId,
  WrongScenarioId,
  MissingStableName,
  DuplicateStableName,
  MissingPlayerBinding,
  MissingPlayerEntity,
  NonFiniteTransform,
  InvalidBounds,
  MissingGoldKeyInteraction,
  InvalidObjectiveCondition,
  OldIggyDependency,
};
```

## Required Types And API

Declare:

```cpp
struct PackageValidationRequest {
  PackageManifest manifest;
  FixtureScenarioSeed scenario;
};

struct PackageValidationResult {
  PackageValidationStatus status = PackageValidationStatus::Ok;
  std::vector<Diagnostic> diagnostics;
};

PackageValidationResult validatePackage(
    const PackageValidationRequest& request);
```

## Validation Order

First failing status wins:

1. package id present;
2. package id equals `iggy3d.first_room` for first fixture tests;
3. schema versions supported;
4. scenario path valid and relative;
5. no old `/Users/kogaryu/iggy` path/dependency appears in manifest or seed;
6. scenario id present;
7. scenario id equals `first_room.runtime_loop`;
8. each entity has a non-empty stable name;
9. duplicate stable names are detected by deterministic scenario-order vector
   scan;
10. player slot 0 exists and binds to stable name `player`;
11. `player` entity exists;
12. all transforms finite;
13. all bounds valid;
14. `gold_key` has canonical pickup interaction:
    `kind=Pickup`, `primaryEffect=AddItemToInventory`, `itemId=gold_key`,
    `itemCount=1`, `objectiveId=collect_gold_key`, `repeatable=false`,
    `deactivateTargetOnSuccess=true`, and targetable interact/inspect;
15. objective `collect_gold_key` condition is player slot 0 has `gold_key:1`.

## Completion Criteria

The header defines every validation status, request/result shape, and first
failure order required by loaders, tools, and tests.
