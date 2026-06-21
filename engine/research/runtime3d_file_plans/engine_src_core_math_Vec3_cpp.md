# `engine/src/core/math/Vec3.cpp`

Purpose: implement pure `Vec3` operations.

Must contain:

- Include `core/math/Vec3.hpp`.
- Implement `lengthSquared()` using `Dot`.
- Implement `length()` using `std::sqrt`.
- Implement zero-safe `normalized()`.
- Implement arithmetic operators.
- Implement exact `operator==`, matching current `Vec2` convention.
- Implement `Dot` and `Cross`.

Construction rules:

- Return `{}` from `normalized()` when magnitude is zero.
- Keep all operations deterministic and allocation-free.
- Do not add epsilon comparison yet.

Compute cost:

- Every operation is `O(1)`.

Completion:

- Builds when registered in `iggy_core_sources.cmake`.

