# `src/runtime/camera/CameraState.hpp`

Updated: 2026-06-20

Exact purpose: declare semantic runtime camera state: realtime camera mode,
tactical camera mode, previous realtime mode memory, target references, and the
transient input-clear request flag.

## Build Position

- priority rank: 53
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/camera`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/runtime/camera/CameraState.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Types

Declare:

```cpp
enum class CameraMode : std::uint8_t {
  FirstPerson,
  ThirdPerson,
  TacticalOverhead,
};

struct CameraTarget {
  EntityId entity;
  Vec3 point;
  bool hasPoint = false;
};

struct CameraState {
  CameraMode activeMode = CameraMode::ThirdPerson;
  CameraMode previousRealtimeMode = CameraMode::ThirdPerson;
  CameraTarget target;
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
  float orbitDistance = 8.0F;
  bool inputClearRequested = false;
};
```

## Semantics

- `FirstPerson` and `ThirdPerson` are realtime modes.
- `TacticalOverhead` is the first tactical mode.
- Initial acceptance camera is `ThirdPerson`.
- Scenario `default_realtime_camera` and `default_tactical_camera` are parsed
  into `FixtureScenarioSeed` and applied during session construction; they are
  not `RuntimeConfig` fields.
- Entering tactical stores the active realtime mode in `previousRealtimeMode`
  before changing `activeMode` to `TacticalOverhead`.
- Leaving tactical restores `previousRealtimeMode`.
- `inputClearRequested` means app/input code should clear transient look/camera
  input after a mode switch. Runtime policy sets it; app code clears raw input
  externally, then calls the policy/API that clears this flag.
- Camera state is semantic only. It does not own view/projection matrices, GPU
  resources, raw mouse deltas, sensitivity settings, or renderer handles.

## Save Replay Multiplayer

`activeMode`, `previousRealtimeMode`, target entity/point, yaw, pitch, and
orbit distance are save truth and replay hash input. `inputClearRequested` is a
transient runtime flag and is excluded from durable save unless a debug-save
section explicitly records it.

## Completion Criteria

The header locks camera modes, fields, defaults, previous realtime behavior,
and transient input-clear semantics without renderer ownership.
