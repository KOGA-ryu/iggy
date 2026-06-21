# `engine/src/runtime3d/Runtime3DTargetQuery.cpp`

Purpose: implement skeletal runtime3d target discovery.

Must contain:

- Include `runtime3d/Runtime3DTargetQuery.hpp`.
- Linear scan over world entities.
- First-match or nearest-match behavior, with deterministic tie behavior.
- Ignore non-targetable kinds unless action explicitly targets them.
- Return explicit no-target status.

Construction rules:

- Use interaction volume when present; otherwise use transform position.
- Do not build a spatial index yet.

Compute cost:

- `O(entity count)`.

Completion:

- `runtime3d_target_query_tests.cpp` can prove behavior.

