# `engine/src/core/math/Mat4.hpp`

Purpose: declare backend-free 4x4 matrix math for runtime3d camera output and
renderer handoff.

Must contain:

- `#pragma once`.
- Include `<array>` and `core/math/Vec3.hpp`.
- Namespace `iggy`.
- `struct Mat4` with `std::array<float, 16> values`.
- Declarations for `Identity`, `Multiply`, `Translation`, `Scale`,
  `RotationX`, `RotationY`, `Perspective`, and `LookAt` if promoted from native
  play.

Construction rules:

- Preserve current native play column-major convention.
- Keep the type backend-free.
- Do not include Vulkan, native play, shader, or renderer headers.

Completion:

- `Mat4.cpp` implements every declaration that this header exposes.

