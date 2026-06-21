# `src/runtime/clock/Clock.cpp`

Updated: 2026-06-20

Exact purpose: implement pure deterministic `ClockState` validation, mode
transitions, step consumption, and tick advancement.

## Build Position

- priority rank: 51
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/clock`
- file kind: `source`

## Required Include Order

```cpp
#include "runtime/clock/Clock.hpp"

#include <cmath>
```

No app, renderer, projection, threading, sleep, timer, network, tests, or old
iggy headers.

## Validation Algorithm

`validateClockState` returns:

1. `InvalidTickRate` if `fixedTickRateHz == 0`;
2. `InvalidTimeScale` if `timeScale` is not finite or is negative;
3. `InvalidTimeScale` if `previousUnpausedTimeScale` is not finite or is not
   positive;
4. `InvalidTimeScale` if `previousUnpausedMode` is `Paused`;
5. `InvalidTimeScale` if mode Normal has time scale not equal to `1.0F`;
6. `InvalidTimeScale` if mode Slow has time scale not finite, not positive, or
   greater than or equal to `1.0F`;
7. `InvalidTimeScale` if mode Paused has time scale not equal to `0.0F`;
8. `InvalidTimeScale` if `previousUnpausedMode=Normal` and
   `previousUnpausedTimeScale != 1.0F`;
9. `InvalidTimeScale` if `previousUnpausedMode=Slow` and
   `previousUnpausedTimeScale <= 0.0F` or `>= 1.0F`;
10. `Ok` otherwise.

Transition helpers must always leave `previousUnpausedMode` as Normal or Slow.

## Function Algorithms

- `makeDefaultClockState`: return default fields exactly as documented in
  `ClockState.hpp`.
- `shouldRunAutomaticTick`: validate state; return true only for Normal/Slow.
- `enterSlow`: validate positive finite scale less than 1, set Slow, previous
  Slow, current scale, previous unpaused time scale, clear step.
- `exitSlow`: set Normal, previous Normal, current scale 1.0, previous
  unpaused time scale 1.0, clear step.
- `pause`: if mode is Normal or Slow, store it as previous unpaused and store
  the current positive time scale as `previousUnpausedTimeScale`; if already
  Paused, keep existing previous unpaused mode and scale; set Paused, current
  scale 0, clear step.
- `resume`: validate state, set mode to `previousUnpausedMode`, set
  `timeScale=previousUnpausedTimeScale`, and clear step. Resume from a state
  with invalid previous unpaused mode or scale returns `InvalidTimeScale` and
  leaves state unchanged.
- `requestStep`: if not Paused, return status `StepRequiresPaused` and leave
  state unchanged; otherwise set `stepRequested=true`.
- `consumeStep`: if Paused and `stepRequested`, return `shouldRunTick=true`,
  `consumedStep=true`, clear step, keep Paused. If no step requested, return no
  tick and unchanged state.
- `advanceTick`: increment `tickIndex` by one, no mode change.

## No-Mutation-On-Failure

All functions operate on copies. On failure they return the original state in
`ClockDecision::state`.

## Compute Cost

All operations are O(1).

## Tests

`tests/unit/clock_tests.cpp` must verify no function reads wall-clock time by
being fully deterministic from input state.

## Completion Criteria

Clock logic is pure, deterministic, and acceptance-flow exact.
