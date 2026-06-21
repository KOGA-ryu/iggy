# `engine/tests/runtime3d_ray_projection_tests.cpp`

Purpose: prove ray-to-world projection behavior.

Must test:

- Ray from above toward ground hits expected point.
- Ray parallel to ground returns no hit.
- Ray pointing away from ground returns no hit.
- Returned hit point uses `Ray3::pointAtDistance`.

Construction rules:

- No renderer matrices.
- Caller constructs ray directly.

Completion:

- Test executable builds and passes when ray projection is implemented.

