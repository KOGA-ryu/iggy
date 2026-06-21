# `engine/src/core/math/Aabb3.hpp`

Purpose: declare the shared 3D axis-aligned bounding box type for runtime3d
collision and interaction volumes.

Must contain:

- `#pragma once`.
- Include `core/math/Vec3.hpp`.
- Namespace `iggy`.
- `struct Aabb3` with `Vec3 min` and `Vec3 max`.
- Method declarations: `contains(Vec3 point)` and `overlaps(Aabb3 other)`.

Construction rules:

- Match `Aabb2.hpp` style.
- Use inclusive min/max semantics unless tests later change the rule.
- Do not normalize malformed boxes silently.

Completion:

- `Aabb3.cpp` implements both methods.

