# `engine/src/runtime3d/Runtime3DRayProjection.cpp`

Purpose: implement ray-to-ground projection.

Must contain:

- Include `runtime3d/Runtime3DRayProjection.hpp`.
- Reject parallel rays.
- Solve distance to configured plane height.
- Reject negative distance unless backward hits are explicitly allowed.
- Return hit point from `Ray3::pointAtDistance`.

Construction rules:

- Keep projection deterministic and pure.
- Do not inspect renderer state or SDL input.

Compute cost:

- Ground plane projection is `O(1)`.

Completion:

- `runtime3d_ray_projection_tests.cpp` can prove behavior.

