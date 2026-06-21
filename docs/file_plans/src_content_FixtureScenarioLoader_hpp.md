# `src/content/FixtureScenarioLoader.hpp`

Updated: 2026-06-20

Exact purpose: declare scenario seed structs and parse APIs for
`scenario.iggy3d.toml`.

## Build Position

- priority rank: 39
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/content/FixtureScenarioLoader.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/RuntimeConfig.hpp"
#include "core/diagnostics/Diagnostic.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/player/PlayerSlot.hpp"
#include "runtime/world/EntityState.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Status Enum

Declare:

```cpp
enum class ScenarioLoadStatus : std::uint8_t {
  Ok,
  ParseError,
  UnsupportedKey,
  MissingRequiredKey,
  MissingScenarioId,
  InvalidNumber,
  InvalidEnum,
  InvalidPath,
};
```

## Required Seed Types

Declare:

```cpp
struct ScenarioPlayerSeed {
  PlayerSlotId slot = kInvalidPlayerSlotId;
  PlayerSlotKind kind = PlayerSlotKind::Unknown;
  std::string actorStableName;
};

struct ScenarioEntitySeed {
  std::string stableName;
  EntityKind kind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 localBounds;
  bool active = true;
  bool persistent = true;
  EntityTargeting targeting;
  InteractionDefinition interaction;
};

enum class ObjectiveStatusSeed : std::uint8_t {
  Active,
  Complete,
  Failed,
};

struct ScenarioObjectiveSeed {
  std::string id;
  ObjectiveStatusSeed initialStatus = ObjectiveStatusSeed::Active;
  std::string condition = "InventoryContains";
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::uint32_t itemCount = 0;
  ObjectiveStatusSeed completeStatus = ObjectiveStatusSeed::Complete;
};

struct FixtureScenarioSeed {
  std::string scenarioId;
  RuntimeConfig config;
  ClockMode initialClockMode = ClockMode::Normal;
  CameraMode defaultRealtimeCamera = CameraMode::ThirdPerson;
  CameraMode defaultTacticalCamera = CameraMode::TacticalOverhead;
  std::vector<ScenarioPlayerSeed> players;
  std::vector<ScenarioEntitySeed> entities;
  std::vector<ScenarioObjectiveSeed> objectives;
};

struct ScenarioLoadResult {
  ScenarioLoadStatus status = ScenarioLoadStatus::Ok;
  FixtureScenarioSeed seed;
  std::vector<Diagnostic> diagnostics;
};
```

## Required API

Declare:

```cpp
ScenarioLoadResult parseScenarioText(const std::string& scenarioText);
```

## First-Room Seed Requirements

Parsing the required fixture produces:

- scenario id `first_room.runtime_loop`;
- config: 20Hz, range `1.500`, move `3.000`, slow scale `0.250`;
- initial runtime modes: clock `Normal`, realtime camera `ThirdPerson`,
  tactical camera `TacticalOverhead`;
- one player seed slot 0 Local actor `player`;
- three entity seeds in order: `player`, `gold_key`, `tactical_marker_alpha`;
- one objective seed `collect_gold_key`.
- `gold_key` maps TOML pickup fields into canonical
  `InteractionDefinition`: `kind=Pickup`,
  `primaryEffect=AddItemToInventory`, `itemId=gold_key`, `itemCount=1`,
  `objectiveId=collect_gold_key`, `repeatable=false`,
  `deactivateTargetOnSuccess=true`.
- The loader owns parse-level requiredness for pickup `objective_ref` in the first
  complete demo schema. Missing `objective_ref` on `gold_key` returns
  `MissingRequiredKey` with diagnostic `scenario.missing_required_key`.
- `PackageValidator` still owns validating that the parsed `objectiveId` refers to
  an existing objective with the expected condition.

## Ownership

These are content seed values only. Runtime copies them into `SessionState`
during session creation. `initialClockMode` initializes `ClockState::mode` and
`previousUnpausedMode`; camera fields initialize `CameraState` and
`CameraModePolicy` defaults. `RuntimeConfig` remains independent from
`runtime/*` headers and stores only numeric defaults. The loader does not
allocate runtime entity ids.

## Completion Criteria

The header defines exact seed structs and parse API for package loading and
session creation.
