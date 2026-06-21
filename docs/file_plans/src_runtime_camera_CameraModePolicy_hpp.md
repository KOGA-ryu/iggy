# `src/runtime/camera/CameraModePolicy.hpp`

Updated: 2026-06-20

Exact purpose: declare pure policy functions that update semantic `CameraState`
from clock/session mode changes without owning renderer matrices or raw input.

## Build Position

- priority rank: 54
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/camera`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/runtime/camera/CameraModePolicy.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>

#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Status And Result Types

Declare:

```cpp
enum class CameraPolicyStatus : std::uint8_t {
  Ok,
  InvalidCameraMode,
  InvalidClockMode,
};

struct CameraModePolicyRequest {
  CameraState camera;
  ClockMode clockMode = ClockMode::Normal;
};

struct CameraModePolicyResult {
  CameraPolicyStatus status = CameraPolicyStatus::Ok;
  CameraState camera;
  bool modeChanged = false;
};
```

## Required API

Declare:

```cpp
bool isRealtimeCameraMode(CameraMode mode);
bool isTacticalCameraMode(CameraMode mode);
CameraState makeDefaultCameraState();

CameraModePolicyResult enterTacticalCamera(CameraState camera);
CameraModePolicyResult exitTacticalCamera(CameraState camera);
CameraModePolicyResult applyClockModeToCamera(
    const CameraModePolicyRequest& request);
CameraModePolicyResult clearCameraInputRequest(CameraState camera);
```

## Policy Semantics

- `makeDefaultCameraState()` returns active `ThirdPerson`, previous realtime
  `ThirdPerson`, no target, yaw 0, pitch 0, orbit distance 8, no input clear.
- `enterTacticalCamera`:
  - if active mode is realtime, stores it as previous realtime;
  - sets active mode to `TacticalOverhead`;
  - sets `inputClearRequested=true`;
  - returns `modeChanged=true` only when active mode changed.
- `exitTacticalCamera`:
  - restores `previousRealtimeMode`;
  - if previous mode is invalid/tactical, returns `InvalidCameraMode`;
  - sets `inputClearRequested=true` when mode changed.
- `applyClockModeToCamera`:
  - `ClockMode::Normal` exits tactical and restores previous realtime mode;
  - `ClockMode::Slow` enters tactical;
  - `ClockMode::Paused` keeps tactical if already tactical; if currently
    realtime, enters tactical for planning/paused command mode.
- `clearCameraInputRequest` clears only the flag and preserves all other
  fields.

## Acceptance Behavior

The acceptance flow must produce:

```text
initial camera = ThirdPerson
ToggleTacticalMode / Slow -> TacticalOverhead, previousRealtimeMode=ThirdPerson
Pause -> TacticalOverhead
Resume -> TacticalOverhead
exit tactical / Normal -> ThirdPerson
```

## Save Replay Multiplayer

Policy transitions are deterministic and replayable. Camera mode is session
state, not local UI-only state. Raw look input is not saved or replayed.

## Tests

`tests/unit/camera_mode_policy_tests.cpp` must cover every API above and the
acceptance behavior.

## Completion Criteria

The header gives exact policy APIs, status values, and mode-transition rules.
