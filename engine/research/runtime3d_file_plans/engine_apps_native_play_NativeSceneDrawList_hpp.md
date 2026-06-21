# `engine/apps/native_play/NativeSceneDrawList.hpp`

Purpose: bridge runtime3d scene projection into existing native draw item shape.

Must contain changes when integration begins:

- Adapter from `Runtime3DSceneProjectionResult` to native draw items.
- Mapping from runtime3d scene item kind to native model slot/draw item.
- Stable ordering matching runtime3d projection order.
- Selected/target marker handling when renderer slots exist.

Construction rules:

- Keep gameplay semantics in runtime3d.
- Native draw list is renderer input, not save truth.
- Preserve old path while runtime3d is behind a switch.

Completion:

- Renderer can draw runtime3d-projected items without inspecting gameplay state.

