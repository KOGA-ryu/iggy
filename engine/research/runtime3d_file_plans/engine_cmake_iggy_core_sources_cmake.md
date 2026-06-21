# `engine/cmake/iggy_core_sources.cmake`

Purpose: register promoted shared 3D math `.cpp` files in the engine library.

Must contain changes when math files exist:

- `src/core/math/Vec3.cpp`
- `src/core/math/Ray3.cpp`
- `src/core/math/Aabb3.cpp`
- `src/core/math/Mat4.cpp`
- `src/core/math/Transform3.cpp`

Construction rules:

- Keep existing 2D math entries unchanged.
- Add only `.cpp` files, not headers.
- Preserve current `list(APPEND IGGY_ENGINE_SOURCES ...)` style.

Completion:

- Core math files build into `iggy_engine`.

