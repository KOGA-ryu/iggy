# `tests/unit/camera_mode_policy_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove semantic camera mode defaults, tactical transitions,
previous realtime restoration, clock-to-camera mapping, and input-clear request
behavior.

## Build Position

- priority rank: 56
- tier: Tier 4: Time Camera Command Session Base
- module: `tests/unit`
- file kind: `test`

## Required Includes

```cpp
#include "runtime/camera/CameraModePolicy.hpp"
```

No renderer, GPU matrices, raw input devices, app windows, filesystem,
wall-clock timing, network, or old iggy.

## Required Test Cases

### `default_camera_is_third_person`

Assert default active mode ThirdPerson, previous realtime ThirdPerson, orbit
distance 8, input clear false.

### `enter_tactical_stores_previous_realtime_mode`

For active ThirdPerson:

- result Ok;
- active TacticalOverhead;
- previous realtime ThirdPerson;
- modeChanged true;
- inputClearRequested true.

Repeat with active FirstPerson and assert previous realtime FirstPerson.

### `enter_tactical_is_idempotent`

From TacticalOverhead with previous ThirdPerson, call enter tactical.

Assert mode unchanged, previous unchanged, modeChanged false.

### `exit_tactical_restores_previous_realtime`

From TacticalOverhead previous ThirdPerson:

- active returns ThirdPerson;
- modeChanged true;
- inputClearRequested true.

Repeat for previous FirstPerson.

### `normal_clock_restores_realtime_camera`

Request active TacticalOverhead, previous ThirdPerson, clock Normal. Assert
ThirdPerson restored.

### `slow_clock_enters_tactical_camera`

Request active ThirdPerson, clock Slow. Assert TacticalOverhead and previous
ThirdPerson.

### `paused_clock_uses_tactical_camera`

Request active ThirdPerson, clock Paused. Assert TacticalOverhead. Request
active TacticalOverhead, clock Paused. Assert unchanged.

### `clear_camera_input_request_only_clears_flag`

Set non-default yaw/pitch/orbit/target and inputClearRequested true. Call clear.

Assert only flag changes.

### `acceptance_camera_flow_matches_demo`

Execute:

1. default ThirdPerson;
2. apply Slow -> TacticalOverhead;
3. apply Paused -> TacticalOverhead;
4. apply Slow -> TacticalOverhead;
5. apply Normal -> ThirdPerson.

Assert previous realtime remains ThirdPerson throughout.

## Completion Criteria

Tests lock camera behavior for slow-time tactical mode before renderer code
exists.
