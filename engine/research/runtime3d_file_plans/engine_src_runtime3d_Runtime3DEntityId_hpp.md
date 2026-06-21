# `engine/src/runtime3d/Runtime3DEntityId.hpp`

Purpose: declare the stable runtime3d entity id value used by world state,
commands, targets, saves, and scene projection.

Must contain:

- `#pragma once`.
- Include `<cstdint>`.
- Namespace `iggy::runtime3d`.
- `struct Runtime3DEntityId` with `std::uint32_t value = 0`.
- Invalid/default id value `0`.
- Equality and inequality operators.
- `valid()` method or `IsValid(Runtime3DEntityId)` helper.

Construction rules:

- Keep id generation out of this file.
- Do not use strings or UUIDs in the skeleton.
- Do not include world/session headers.

Completion:

- Any file can compare ids without creating dependency cycles.

