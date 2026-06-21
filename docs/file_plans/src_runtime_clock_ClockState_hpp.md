# `src/runtime/clock/ClockState.hpp`

Updated: 2026-06-20

Exact purpose: declare authoritative runtime clock state for normal realtime,
slow tactical time, paused mode, and single-step requests.

## Build Position

- priority rank: 49
- tier: Tier 4: Time Camera Command Session Base
- module: `src/runtime/clock`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/runtime/clock/ClockState.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Types

Declare:

```cpp
enum class ClockMode : std::uint8_t {
  Normal,
  Slow,
  Paused,
};

struct ClockState {
  ClockMode mode = ClockMode::Normal;
  ClockMode previousUnpausedMode = ClockMode::Normal;
  float previousUnpausedTimeScale = 1.0F;
  std::uint64_t tickIndex = 0;
  std::uint32_t fixedTickRateHz = 20;
  float timeScale = 1.0F;
  bool stepRequested = false;
};
```

## Field Semantics

- `mode`: current simulation mode.
- `previousUnpausedMode`: mode restored by `resume`; must be `Normal` or
  `Slow`, never `Paused`.
- `previousUnpausedTimeScale`: positive finite time scale restored by
  `resume`; `1.0F` for Normal and the active slow scale for Slow.
- `tickIndex`: deterministic runtime tick counter.
- `fixedTickRateHz`: first build default `20`.
- `timeScale`: `1.0F` for normal, config-defined slow scale for slow, `0.0F`
  for paused if stored.
- `stepRequested`: transient command-consumption flag consumed by
  `consumeStep`; only meaningful while paused; excluded from save and state
  hash.

## Acceptance Clock Contract

- Initial state: `mode=Normal`, `previousUnpausedMode=Normal`,
  `previousUnpausedTimeScale=1.0F`, `tickIndex=0`, `fixedTickRateHz=20`,
  `timeScale=1.0F`, `stepRequested=false`.
- First-room `initial_clock = "Normal"` is parsed into
  `FixtureScenarioSeed::initialClockMode` and then applied during session
  construction to initialize `ClockState::mode`; it is not a `RuntimeConfig`
  field.
- `ToggleTacticalMode` entering tactical changes mode to `Slow`.
- `Pause` changes mode to `Paused` and stores previous unpaused mode `Slow` and
  previous unpaused time scale `0.250F` during the acceptance flow.
- `StepTacticalTick` while paused advances exactly one tick decision and keeps
  mode `Paused`.
- `Resume` returns to `Slow` with `timeScale=0.250F`.
- Exiting tactical returns to `Normal`.

## Ownership

Clock state owns simulation mode and deterministic tick counters only. It must
not own wall-clock timestamps, render frame pacing, app timers, raw input, or
thread sleeps.

## Save Replay Multiplayer

Durable `ClockState` fields are save truth, replay input, and multiplayer
authority state except `stepRequested`. State hash includes mode, previous
unpaused mode, tick index, tick rate, previous unpaused time scale, and current
time scale quantized through `StableHash`. State hash excludes `stepRequested`.
Save/load and replay boundary reconstruction set `stepRequested=false`.

## Completion Criteria

The header locks clock modes, fields, defaults, and acceptance semantics without
requiring app or wall-clock data.
