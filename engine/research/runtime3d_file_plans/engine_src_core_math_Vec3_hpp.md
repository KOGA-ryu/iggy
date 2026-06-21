# `engine/src/core/math/Vec3.hpp`

Purpose: declare the shared 3D vector value type used by runtime3d, camera
projection, collision volumes, transforms, and native rendering bridges.

Must contain:

- `#pragma once`.
- Namespace `iggy`.
- `struct Vec3` with `float x`, `float y`, `float z`, each defaulting to `0.0F`.
- Method declarations: `lengthSquared()`, `length()`, `normalized()`.
- Operator declarations: `operator+`, `operator-`, `operator*`, `operator/`,
  `operator==`.
- Function declarations: `Dot(Vec3, Vec3)` and `Cross(Vec3, Vec3)`.

Construction rules:

- Follow `Vec2.hpp` style.
- Keep this as pure math with no runtime3d namespace and no gameplay units.
- Do not include renderer, native app, SDL, Vulkan, or runtime headers.

Completion:

- Compiles as a public engine include.
- `Vec3.cpp` implements every declaration.

