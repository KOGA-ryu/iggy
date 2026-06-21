# `engine/src/runtime3d/Runtime3DSessionState.cpp`

Purpose: implement session-state helpers.

Must contain:

- Include `runtime3d/Runtime3DSessionState.hpp`.
- Optional helper `IsPlayableLifecycle`.
- Optional helper to create an empty loaded/playing session.

Construction rules:

- Keep helpers pure.
- No package load, file IO, native app calls, or renderer integration.

Completion:

- Builds through `iggy_runtime3d_sources.cmake`.

