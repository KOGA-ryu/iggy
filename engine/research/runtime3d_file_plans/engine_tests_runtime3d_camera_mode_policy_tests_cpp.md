# `engine/tests/runtime3d_camera_mode_policy_tests.cpp`

Purpose: prove real-time and tactical camera mode transitions.

Must test:

- Normal mode keeps close third-person.
- Normal mode keeps/restores first-person when it was previous realtime mode.
- Slow mode switches to tactical camera.
- Tactical planning switches to tactical camera.
- Returning to Normal restores previous real-time camera.
- Changed-mode flag is set when camera mode changes.

Construction rules:

- No renderer matrices.
- No native input events.
- Test semantic camera state only.

Completion:

- Test executable builds and passes.

