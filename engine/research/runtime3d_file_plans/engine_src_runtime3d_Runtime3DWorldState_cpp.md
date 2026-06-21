# `engine/src/runtime3d/Runtime3DWorldState.cpp`

Purpose: implement runtime3d world collection helpers.

Must contain:

- Include `runtime3d/Runtime3DWorldState.hpp`.
- Linear-scan const and mutable find helpers.
- `add` and/or `upsert` implementation.

Construction rules:

- `upsert` replaces same-id entity, otherwise appends.
- Invalid ids must behave deterministically.
- Do not add a spatial index yet.

Compute cost:

- Find/upsert are `O(entity count)`.

Completion:

- `runtime3d_world_state_tests.cpp` proves add/find/upsert.

