# `engine/apps/native_play/NativeVulkanRenderer.hpp`

Purpose: declare renderer interface support for runtime3d-derived scene items
and camera matrices.

Must contain changes when integration begins:

- Frame input remains renderer-facing: view/projection plus draw items.
- Any additional runtime3d item kinds appear only as renderer-neutral draw item
  data or model slots.

Construction rules:

- Do not include active runtime3d session state if draw items already contain
  needed render facts.
- Renderer owns GPU resources, not gameplay truth.

Completion:

- Runtime3D can feed renderer without coupling renderer to command/world
  mutation.

