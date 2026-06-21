# `engine/src/runtime3d/Runtime3DCameraState.cpp`

Purpose: implement pure camera-state helpers.

Must contain:

- Include `runtime3d/Runtime3DCameraState.hpp`.
- Optional helpers `IsRealtimeCameraMode` and `IsTacticalCameraMode`.
- Optional helper to normalize/restore previous real-time mode.

Construction rules:

- Keep helpers pure and testable.
- Do not compute renderer matrices here unless a later camera-matrix contract is
  explicitly added.

Completion:

- Builds through `iggy_runtime3d_sources.cmake`.

