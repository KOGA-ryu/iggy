# `src/runtime/clock/Clock.hpp`

Updated: 2026-06-20

Exact purpose: declare pure deterministic helpers for changing and querying
`ClockState`.

## Build Position

- priority rank: 50
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/clock`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/runtime/clock/Clock.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>

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
enum class ClockStatus : std::uint8_t {
  Ok,
  InvalidTickRate,
  InvalidTimeScale,
  StepRequiresPaused,
};

struct ClockDecision {
  ClockStatus status = ClockStatus::Ok;
  bool shouldRunTick = false;
  bool consumedStep = false;
  ClockState state;
};
```

## Required API

Declare pure functions:

```cpp
ClockState makeDefaultClockState();
ClockStatus validateClockState(const ClockState& state);

ClockDecision shouldRunAutomaticTick(const ClockState& state);
ClockDecision enterSlow(ClockState state, float slowScale);
ClockDecision exitSlow(ClockState state);
ClockDecision pause(ClockState state);
ClockDecision resume(ClockState state);
ClockDecision requestStep(ClockState state);
ClockDecision consumeStep(ClockState state);
ClockState advanceTick(ClockState state);
```

All functions take state by value and return a new state in the result. They do
not read wall-clock time, mutate globals, or depend on app timers.

## Acceptance Semantics

- `makeDefaultClockState()` returns Normal, previous Normal, tick 0, 20Hz,
  previous unpaused time scale 1.0, current time scale 1.0, no step request.
- `enterSlow(state, slowScale)` validates `slowScale > 0.0F` and `< 1.0F`,
  sets mode Slow, time scale to slowScale, previous unpaused Slow, and
  previous unpaused time scale to slowScale.
- `exitSlow` sets mode Normal, time scale 1.0, previous unpaused Normal, and
  previous unpaused time scale 1.0, and clears step request.
- `pause` stores current Normal/Slow mode in `previousUnpausedMode`, sets mode
  Paused, stores the current positive time scale in
  `previousUnpausedTimeScale`, sets current time scale 0.0, and clears step
  request. Calling `pause` while already Paused leaves previous unpaused mode
  and previous unpaused time scale unchanged.
- `resume` sets mode to `previousUnpausedMode` and restores
  `timeScale=previousUnpausedTimeScale`.
- `requestStep` succeeds only while Paused; otherwise returns
  `StepRequiresPaused`.
- `consumeStep` while Paused and requested returns `shouldRunTick=true`,
  `consumedStep=true`, clears request, keeps mode Paused, and advances no tick
  by itself. Session tick calls `advanceTick`.
- `shouldRunAutomaticTick` returns true in Normal or Slow and false in Paused.

## Save Replay Multiplayer

Clock helpers are deterministic state transitions. Replay must call the same
helpers for pause/step/resume commands. Multiplayer authority treats clock mode
as host/session state, not local UI state.

## Tests

`tests/unit/clock_tests.cpp` must assert every API above, including acceptance
flow Normal -> Slow(0.250) -> Paused -> step -> Slow(0.250) -> Normal.

## Completion Criteria

The header gives exact function signatures and status values for clock logic.
