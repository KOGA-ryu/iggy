# `engine/src/runtime3d/Runtime3DRayProjection.hpp`

Purpose: declare ray-to-world projection for tactical cursor and camera picking.

Must contain:

- `#pragma once`.
- Include `core/math/Ray3.hpp`, `core/math/Vec3.hpp`, and
  `runtime3d/Runtime3DEntityId.hpp`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DRayProjectionStatus`.
- `struct Runtime3DRayProjectionInput`.
- `struct Runtime3DRayProjectionResult`.
- `class Runtime3DRayProjection`.

Input fields:

- ray;
- ground plane height or target plane;
- optional world pointer/input when entity picking arrives.

Result fields:

- status;
- hit point;
- hit entity id plus flag when entity picking arrives.

Construction rules:

- Skeleton projects to ground plane `y = 0`.
- Renderer matrices are not owned here; caller provides the ray.

Completion:

- Ray projection tests can prove hit/no-hit outcomes.

