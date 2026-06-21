# `src/runtime/camera/CameraModePolicy.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic semantic camera mode transitions for
normal realtime play, slow tactical planning, pause, resume, and input-clear
requests.

## Build Position

- priority rank: 55
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/camera`
- file kind: `source`

## Required Include Order

```cpp
#include "runtime/camera/CameraModePolicy.hpp"
```

No renderer, matrix projection, raw input, app, network, tests, or old iggy
headers.

## Algorithms

### `isRealtimeCameraMode`

Returns true for `FirstPerson` and `ThirdPerson`.

### `isTacticalCameraMode`

Returns true for `TacticalOverhead`.

### `makeDefaultCameraState`

Returns exact defaults from `CameraState.hpp`.

### `enterTacticalCamera`

Algorithm:

1. validate active mode.
2. if active mode is realtime, store it in `previousRealtimeMode`.
3. if active mode is already `TacticalOverhead`, return unchanged state with
   `modeChanged=false`.
4. set active mode `TacticalOverhead`.
5. set `inputClearRequested=true`.
6. return `Ok`.

### `exitTacticalCamera`

Algorithm:

1. validate `previousRealtimeMode` is realtime.
2. if active mode already equals previous realtime, return unchanged with
   `modeChanged=false`.
3. set active mode to previous realtime.
4. set `inputClearRequested=true`.
5. return `Ok`.

### `applyClockModeToCamera`

Switch by clock mode:

- `Normal`: call `exitTacticalCamera`.
- `Slow`: call `enterTacticalCamera`.
- `Paused`: if active tactical, return unchanged; otherwise call
  `enterTacticalCamera`.

Unknown clock values return `InvalidClockMode` and leave camera unchanged.

### `clearCameraInputRequest`

Set `inputClearRequested=false`, leave all other fields unchanged, return `Ok`.

## No-Mutation-On-Failure

All APIs take state by value. On invalid mode/status, return the original state
unchanged.

## Compute Cost

All operations are O(1).

## Completion Criteria

Implementation passes policy tests and never owns renderer matrices or raw
input.
