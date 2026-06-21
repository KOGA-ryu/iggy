# `engine/src/runtime3d/Runtime3DClock.cpp`

Purpose: implement deterministic clock decisions.

Must contain:

- Include `runtime3d/Runtime3DClock.hpp`.
- Switch over `Runtime3DClockMode`.
- Normal: automatic tick runs.
- Slow: automatic tick runs and preserves slow mode/time scale.
- Paused: no automatic tick.
- StepRequested: one tick runs, step request is consumed, returned mode becomes
  Paused.

Construction rules:

- Do not mutate external state.
- Do not use render-frame timing directly.

Compute cost:

- `O(1)`.

Completion:

- `runtime3d_clock_tests.cpp` passes.

