# `engine/apps/native_play/NativeVulkanRenderer.cpp`

Purpose: draw runtime3d-projected native scene items.

Must contain changes when integration begins:

- Model-slot handling for runtime3d item kinds.
- Fallback mesh behavior for missing assets.
- Draw item iteration that remains renderer-owned.
- Diagnostics for loaded/fallback/missing render assets.

Construction rules:

- Do not inspect runtime3d command state or save data.
- Do not perform target discovery or reach checks.
- GPU upload and pipeline state remain renderer-owned.

Completion:

- Runtime3D scene items render through native renderer.

