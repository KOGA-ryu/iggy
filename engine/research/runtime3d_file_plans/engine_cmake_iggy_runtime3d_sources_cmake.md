# `engine/cmake/iggy_runtime3d_sources.cmake`

Purpose: register runtime3d `.cpp` files in `IGGY_ENGINE_SOURCES`.

Must contain:

- `list(APPEND IGGY_ENGINE_SOURCES ...)`.
- One entry for every runtime3d implementation file:
  - `src/runtime3d/Runtime3DSessionState.cpp`
  - `src/runtime3d/Runtime3DSession.cpp`
  - `src/runtime3d/Runtime3DClock.cpp`
  - `src/runtime3d/Runtime3DCommand.cpp`
  - `src/runtime3d/Runtime3DCommandAdmission.cpp`
  - `src/runtime3d/Runtime3DWorldState.cpp`
  - `src/runtime3d/Runtime3DEntityState.cpp`
  - `src/runtime3d/Runtime3DCameraState.cpp`
  - `src/runtime3d/Runtime3DCameraModePolicy.cpp`
  - `src/runtime3d/Runtime3DTargetQuery.cpp`
  - `src/runtime3d/Runtime3DRayProjection.cpp`
  - `src/runtime3d/Runtime3DSceneProjection.cpp`
  - `src/runtime3d/Runtime3DSaveEnvelope.cpp`
  - `src/runtime3d/Runtime3DSaveLoad.cpp`
  - `src/runtime3d/Runtime3DLegacy2DAdapter.cpp`

Construction rules:

- Do not register tests here.
- Do not register old 2D runtime files here.
- Keep ordering primitive-to-session where practical.

Completion:

- `engine/CMakeLists.txt` includes this file.

