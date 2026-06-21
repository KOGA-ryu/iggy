# `engine/tests/runtime3d_session_state_tests.cpp`

Purpose: prove default and minimal playable runtime3d session state.

Must test:

- Default `Runtime3DSessionState.lifecycle` is `NotLoaded`.
- A session can be created or set to `PlayingRealtime`.
- A player entity can be added to session world.
- Added player entity can be found by stable id.
- Session state remains backend-free.

Construction rules:

- Use existing simple test style and local `Expect` helper.
- Include runtime3d headers only.
- No package loading, SDL, Vulkan, or native play.

Completion:

- Test executable builds and passes.

