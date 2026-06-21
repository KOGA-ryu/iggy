# `engine/src/runtime3d/Runtime3DEntityState.hpp`

Purpose: declare authoritative per-entity runtime3d state.

Must contain:

- `#pragma once`.
- Include `<string>`.
- Include `Runtime3DEntityId.hpp`, `Runtime3DTransform.hpp`,
  `Runtime3DCollisionVolume.hpp`, and `Runtime3DInteractionVolume.hpp`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DEntityKind` with `Player`, `Ally`, `Enemy`, `Pickup`,
  `Door`, `Wall`, `Floor`, `Prop`, `Objective`, `TacticalMarker`,
  `CameraAnchor`.
- `struct Runtime3DEntityState` with id, kind, transform, collision volume,
  interaction volume, `std::string assetRef`, and `bool persistent`.

Construction rules:

- Keep state serializable by shape: no pointers, references, or callbacks.
- Do not put renderer draw items here.

Completion:

- World state can store entities by value.

