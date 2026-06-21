# `engine/src/core/math/Ray3.hpp`

Purpose: declare the shared 3D ray value type for camera picking and runtime3d
target projection.

Must contain:

- `#pragma once`.
- Include `core/math/Vec3.hpp`.
- Namespace `iggy`.
- `struct Ray3` with `Vec3 origin` and `Vec3 direction`.
- Method declaration `pointAtDistance(float distance)`.

Construction rules:

- Mirror `Ray2.hpp`.
- Do not normalize direction in the type. Callers decide when normalization is
  required.
- Do not include collision, renderer, or runtime headers.

Completion:

- `Ray3.cpp` implements `pointAtDistance`.

