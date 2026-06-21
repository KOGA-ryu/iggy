# `tests/unit/clock_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove deterministic clock defaults, mode transitions, automatic
tick decisions, pause, single-step, resume, and no-mutation-on-failure.

## Build Position

- priority rank: 52
- tier: Tier 4: Time Camera Command Session Base
- module: `tests/unit`
- file kind: `test`

## Required Includes

```cpp
#include "runtime/clock/Clock.hpp"
```

No app, filesystem, renderer, wall-clock timing, sleeps, network, or old iggy.

## Required Test Cases

### `default_clock_is_normal_twenty_hertz`

Assert:

- mode Normal;
- previous unpaused Normal;
- previous unpaused time scale 1.0;
- tick index 0;
- fixed tick rate 20;
- time scale 1.0;
- step not requested;
- validation Ok.

### `normal_and_slow_run_automatic_ticks`

Assert:

- `shouldRunAutomaticTick(default).shouldRunTick == true`;
- after `enterSlow(default, 0.250F)`, mode Slow, time scale 0.250,
  previous unpaused Slow, previous unpaused time scale 0.250, auto tick true.

### `paused_does_not_run_automatic_tick`

Pause a Slow(0.250) state. Assert mode Paused, previous Slow, previous
unpaused time scale 0.250, current time scale 0.0, auto tick false. Pause the
paused state again and assert previous unpaused mode and scale remain unchanged.

### `request_step_requires_paused`

Assert request step from Normal returns `StepRequiresPaused` and unchanged
state. Assert request step from Paused sets `stepRequested=true`.

### `consume_step_runs_exactly_one_tick_decision`

From paused step-requested state:

- `consumeStep` returns `shouldRunTick=true`;
- `consumedStep=true`;
- mode remains Paused;
- step flag clears;
- second `consumeStep` returns no tick.

### `advance_tick_increments_only_tick_index`

Assert `advanceTick` increments `tickIndex` by exactly 1 and preserves mode.

### `acceptance_clock_flow_matches_demo`

Execute:

1. default Normal;
2. `enterSlow(..., 0.250F)`;
3. `pause`;
4. `requestStep`;
5. `consumeStep`;
6. `advanceTick`;
7. `resume`;
8. `exitSlow`.

Assert modes: Normal -> Slow -> Paused -> Paused -> Slow -> Normal and final
tick index 1. Assert `resume` restores `timeScale=0.250F` and `exitSlow`
restores current and previous unpaused time scale to `1.0F`.

### `invalid_tick_rate_and_scale_reject`

Assert zero tick rate, NaN/negative current time scale, NaN/zero/negative
previous unpaused time scale, and `previousUnpausedMode=Paused` return
validation failures.

## Completion Criteria

Tests lock clock behavior for command admission, session tick, save/load,
replay, and camera mode policy.
