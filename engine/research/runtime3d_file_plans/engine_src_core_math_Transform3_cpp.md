# `engine/src/core/math/Transform3.cpp`

Purpose: provide a translation unit for `Transform3`.

Must contain:

- Include `core/math/Transform3.hpp`.
- No implementation is required until transform helpers are introduced.

Construction rules:

- Keep the file even if empty except for the include, so future transform logic
  has a stable source path.
- Do not add matrix generation here unless scene projection requires it.

Completion:

- Builds through `iggy_core_sources.cmake`.

