# `engine/tests/runtime3d_scene_projection_tests.cpp`

Purpose: prove renderer-facing scene facts derive from runtime3d world state.

Must test:

- Player, floor, wall, and pickup produce scene items.
- Entity id, kind, transform, and assetRef copy through.
- Selected/target flags copy through when those fields exist.
- Item order is stable.

Construction rules:

- No Vulkan or native renderer types.
- Scene items are render input, not save truth.

Completion:

- Test executable builds and passes when scene projection is implemented.

