# `engine/src/runtime3d/Runtime3DEntityState.cpp`

Purpose: implement entity-state helper functions when needed.

Must contain:

- Include `runtime3d/Runtime3DEntityState.hpp`.
- Optional helpers such as `IsRuntime3DTargetable(Runtime3DEntityKind)`.

Construction rules:

- Start minimal.
- Add helpers only when multiple files need the same entity-kind rule.
- Do not include renderer, native play, or save encoding.

Completion:

- Builds through `iggy_runtime3d_sources.cmake`.

