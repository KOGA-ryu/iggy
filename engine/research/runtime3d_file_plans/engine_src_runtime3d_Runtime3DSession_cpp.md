# `engine/src/runtime3d/Runtime3DSession.cpp`

Purpose: implement runtime3d session operations.

Must contain:

- Include `runtime3d/Runtime3DSession.hpp`.
- Empty/playing session creation.
- Lifecycle transition helper.
- Later: command admission/application and reset/retry/load helpers.

Construction rules:

- Keep the skeleton small but real.
- Do not load packages or touch filesystem in this file.
- Do not mutate renderer or native shell state.

Completion:

- `runtime3d_session_state_tests.cpp` passes.

