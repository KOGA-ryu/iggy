# `engine/src/runtime3d/Runtime3DCameraState.hpp`

Purpose: declare runtime-owned camera mode and semantic camera state.

Must contain:

- `#pragma once`.
- Include `core/math/Vec3.hpp` and `runtime3d/Runtime3DEntityId.hpp`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DCameraMode` with `RealtimeFirstPerson`,
  `RealtimeThirdPersonClose`, `TacticalOrbit`, `TacticalOverhead`.
- `struct Runtime3DCameraModeState` with active mode, previous real-time mode,
  yaw, pitch, orbit distance, target entity id, and optional target point.

Construction rules:

- Defaults: active and previous real-time mode are close third-person unless
  product config later chooses FPS.
- Store semantic camera state only. No raw mouse deltas or renderer resources.

Completion:

- Camera policy can transition modes without native app dependencies.

