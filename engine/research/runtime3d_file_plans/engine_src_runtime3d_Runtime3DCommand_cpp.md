# `engine/src/runtime3d/Runtime3DCommand.cpp`

Purpose: implement command helper functions.

Must contain:

- Include `runtime3d/Runtime3DCommand.hpp`.
- Optional helpers:
  - `RequiresActor(Runtime3DCommandKind)`;
  - `IsSessionCommand(Runtime3DCommandKind)`;
  - simple status/reason helpers if tests need them.

Construction rules:

- Keep helpers pure.
- Do not mutate session or world state.
- Do not validate reach here; command admission owns validation.

Completion:

- Builds through `iggy_runtime3d_sources.cmake`.

