# `src/config/RuntimeConfig.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic numeric runtime defaults consumed during
session creation, command admission, clock behavior, movement, and interaction
reach.

## Build Position

- priority rank: 27
- tier: Tier 2: Content And Configuration
- module: `config`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/config/RuntimeConfig.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
```

This header must not include any `runtime/*` header. Camera mode enums are owned
by the runtime camera state header; clock modes are owned by the runtime clock
state header. Scenario loading stores those runtime initial state choices in
`FixtureScenarioSeed`, not in `RuntimeConfig`. This header must not depend on
app, content loader implementation, renderer, tests, or old iggy code.

## Required Status And Type

Declare:

```cpp
enum class RuntimeConfigStatus : std::uint8_t {
  Ok,
  InvalidTickRate,
  InvalidInteractionRange,
  InvalidMovementDistance,
  InvalidSlowTimeScale,
};

struct RuntimeConfig {
  std::uint32_t fixedTickRateHz = 20;
  float interactionRangeMeters = 1.500F;
  float movementDistanceMeters = 3.000F;
  float slowTimeScale = 0.250F;
};
```

Declare:

```cpp
RuntimeConfig makeDefaultRuntimeConfig();
RuntimeConfigStatus validateRuntimeConfig(const RuntimeConfig& config);
```

## Semantics

- Numeric defaults must match `docs/acceptance_demo.md`: 20Hz, 1.500m reach,
  3.000m move, and slow-time scale `0.250`.
- Initial clock and camera modes are scenario initial runtime state, owned by
  `FixtureScenarioSeed` and applied by session construction through
  `ClockState`, `CameraState`, and `CameraModePolicy`.
- Config values are copied into `Session` or consumed at session creation.
- Config is not save payload by itself. Runtime state/save envelopes store the
  resulting authoritative state and schema/version metadata.
- App CLI options select fixture paths but do not mutate this config after
  session creation.

## Validation Order

`validateRuntimeConfig` returns the first failure in this order:

1. tick rate must be greater than zero;
2. interaction range must be finite and greater than zero;
3. movement distance must be finite and greater than zero;
4. slow time scale must be finite, greater than zero, and less than `1.0F`.

## Save Replay Multiplayer

Config is seed/default input. Runtime save truth is the authoritative state
created from this config, plus schema/runtime compatibility metadata. Replay and
multiplayer authority must use the same config-derived session state for
session creation.

## Tests

Package loader tests assert first-room numeric config values. Clock and camera
tests assert initial modes through `FixtureScenarioSeed`, `ClockState`, and
`CameraState`, not through `RuntimeConfig`.

## Completion Criteria

The header gives exact fields, defaults, validation status, and validation
order.
