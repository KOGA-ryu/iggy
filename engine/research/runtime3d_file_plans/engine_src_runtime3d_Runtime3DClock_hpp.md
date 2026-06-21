# `engine/src/runtime3d/Runtime3DClock.hpp`

Purpose: declare runtime3d simulation clock state and tick-decision policy.

Must contain:

- `#pragma once`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DClockMode` with `Normal`, `Slow`, `Paused`,
  `StepRequested`.
- `struct Runtime3DClockState` with mode, `float timeScale = 1.0F`, and
  `float accumulatedSeconds = 0.0F`.
- `struct Runtime3DClockTickDecision` with `bool runAutomaticTick`,
  `bool consumedStepRequest`, and returned state.
- `class Runtime3DClock` or pure function declarations for deciding ticks.

Construction rules:

- Clock decisions are pure and deterministic.
- No native wall-clock, SDL, or sleep calls.

Completion:

- Clock tests prove normal, paused, slow, and step semantics.

