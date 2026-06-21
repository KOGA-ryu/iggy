# `engine/src/runtime3d/Runtime3DCollisionVolume.hpp`

Purpose: declare the minimal collision volume shape owned by runtime3d.

Must contain:

- `#pragma once`.
- Include `core/math/Aabb3.hpp` and `core/math/Vec3.hpp`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DCollisionVolumeKind` with `None`, `Aabb`, `Capsule`.
- `struct Runtime3DCollisionVolume` with:
  - kind default `None`;
  - `Aabb3 bounds`;
  - optional capsule fields `Vec3 base`, `Vec3 tip`, `float radius`.

Construction rules:

- This is broad-phase data only in the skeleton.
- No physics response, no server physics dependency, no renderer coupling.

Completion:

- Entity state can carry collision data without pulling in gameplay systems.

