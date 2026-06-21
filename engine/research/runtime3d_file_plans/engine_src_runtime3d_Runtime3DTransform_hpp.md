# `engine/src/runtime3d/Runtime3DTransform.hpp`

Purpose: define the runtime-facing transform type for authoritative 3D entity
placement.

Must contain:

- `#pragma once`.
- Include `core/math/Transform3.hpp`.
- Namespace `iggy::runtime3d`.
- Either `using Runtime3DTransform = Transform3` or a thin struct wrapping
  `Transform3`.

Construction rules:

- Runtime3D owns this transform as gameplay truth.
- Renderer receives copied transforms, not ownership.
- Do not include native play or renderer math.

Completion:

- `Runtime3DEntityState.hpp` can include this file and remain backend-free.

