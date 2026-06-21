# `engine/src/core/math/Aabb3.cpp`

Purpose: implement pure 3D AABB containment and overlap logic.

Must contain:

- Include `core/math/Aabb3.hpp`.
- Implement inclusive `contains`.
- Implement axis-wise `overlaps`.

Construction rules:

- Keep behavior deterministic for malformed min/max boxes.
- Do not add physics response, sweep tests, or spatial indexing.

Compute cost:

- `O(1)`.

Completion:

- Builds through `iggy_core_sources.cmake`.

