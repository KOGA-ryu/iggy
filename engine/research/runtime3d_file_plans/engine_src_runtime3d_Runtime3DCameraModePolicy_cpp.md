# `engine/src/runtime3d/Runtime3DCameraModePolicy.cpp`

Purpose: implement runtime3d camera mode transition rules.

Must contain:

- Include `runtime3d/Runtime3DCameraModePolicy.hpp`.
- Normal clock restores previous real-time camera.
- Slow clock switches to tactical camera.
- Tactical planning flag switches to tactical camera.
- Paused preserves tactical camera if already tactical; otherwise uses tactical
  default.
- Realtime mode changes update previous real-time mode.

Construction rules:

- Do not inspect renderer state.
- Do not mutate world state.
- Return `changedMode` so native shell can clear transient input.

Completion:

- `runtime3d_camera_mode_policy_tests.cpp` passes.

