# `engine/src/runtime3d/Runtime3DSceneProjection.cpp`

Purpose: implement deterministic scene projection from world entities.

Must contain:

- Include `runtime3d/Runtime3DSceneProjection.hpp`.
- Map entity kinds to scene item kinds.
- Copy transform and assetRef.
- Preserve entity vector order.
- Skip only entities explicitly marked non-renderable when such a flag exists.

Construction rules:

- Do not inspect gameplay state outside runtime3d world state.
- Do not allocate GPU resources.

Compute cost:

- `O(renderable entity count)`.

Completion:

- `runtime3d_scene_projection_tests.cpp` can prove projection behavior.

