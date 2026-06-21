# `engine/src/core/math/Transform3.hpp`

Purpose: declare the shared 3D transform value type used by runtime3d entities
and scene projection.

Must contain:

- `#pragma once`.
- Include `core/math/Vec3.hpp`.
- Namespace `iggy`.
- `struct Transform3` with:
  - `Vec3 position`;
  - `Vec3 rotation`;
  - `Vec3 scale` defaulting to `{ 1.0F, 1.0F, 1.0F }`.

Construction rules:

- Rotation is Euler in the skeleton.
- Keep this as data only unless a later projection helper is required.
- No renderer model matrix ownership here.

Completion:

- Usable by runtime3d headers without native app dependencies.

