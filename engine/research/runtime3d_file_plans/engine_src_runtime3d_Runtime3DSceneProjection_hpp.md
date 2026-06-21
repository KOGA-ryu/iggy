# `engine/src/runtime3d/Runtime3DSceneProjection.hpp`

Purpose: declare projection from authoritative runtime3d world state to
renderer-facing scene facts.

Must contain:

- `#pragma once`.
- Include `runtime3d/Runtime3DWorldState.hpp`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DSceneItemKind`.
- `struct Runtime3DSceneItem`.
- `struct Runtime3DSceneProjectionInput`.
- `struct Runtime3DSceneProjectionResult`.
- `class Runtime3DSceneProjection`.

Scene item fields:

- source entity id;
- item kind;
- transform;
- asset ref;
- selected flag;
- target flag.

Construction rules:

- No Vulkan or native renderer types.
- Output is render input, not save truth.
- Preserve stable entity order.

Completion:

- Scene projection tests can prove item count, mapping, asset refs, and order.

