# `engine/tests/runtime3d_clock_tests.cpp`

Purpose: prove runtime3d clock decision semantics.

Must test:

- Normal mode runs an automatic tick.
- Slow mode runs an automatic tick and preserves slow semantics/time scale.
- Paused mode does not run an automatic tick.
- StepRequested runs exactly one tick, marks step consumed, and returns Paused.

Construction rules:

- No sleeps, wall-clock calls, render-frame assumptions, SDL, or native app.
- Assert returned state as well as decision flags.

Completion:

- Test executable builds and passes.

