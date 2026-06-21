# `engine/src/runtime3d/Runtime3DWorldState.hpp`

Purpose: declare the runtime3d world entity collection.

Must contain:

- `#pragma once`.
- Include `<vector>`.
- Include `runtime3d/Runtime3DEntityState.hpp`.
- Namespace `iggy::runtime3d`.
- `struct Runtime3DWorldState` with:
  - `std::vector<Runtime3DEntityState> entities`;
  - optional `std::uint32_t nextEntityId = 1`;
  - const and mutable `find(Runtime3DEntityId)` declarations;
  - `add` or `upsert` declarations.

Construction rules:

- Use value storage.
- Use id `0` as invalid.
- Keep lookup linear in the skeleton.

Completion:

- World tests can add, find, and upsert entities.

