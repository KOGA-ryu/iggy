# `engine/src/runtime3d/Runtime3DInteractionVolume.hpp`

Purpose: declare the volume used by runtime3d target discovery and interaction
reach validation.

Must contain:

- `#pragma once`.
- Include `core/math/Aabb3.hpp`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DInteractionVolumeKind` with `None`, `Aabb`.
- `struct Runtime3DInteractionVolume` with kind, `Aabb3 bounds`, and optional
  reach/range scalar if admission needs it.

Construction rules:

- Keep interaction separate from collision.
- Do not apply inventory, effects, or commands here.

Completion:

- `Runtime3DTargetQuery` and command admission can read interaction shape.

