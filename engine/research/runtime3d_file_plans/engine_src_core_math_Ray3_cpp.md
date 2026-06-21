# `engine/src/core/math/Ray3.cpp`

Purpose: implement the pure 3D ray helper.

Must contain:

- Include `core/math/Ray3.hpp`.
- Implement `Ray3::pointAtDistance(float distance)` as
  `origin + direction * distance`.

Construction rules:

- Keep this file pure math.
- No hit testing, target selection, or renderer projection here.

Compute cost:

- `O(1)`.

Completion:

- Builds through `iggy_core_sources.cmake`.

