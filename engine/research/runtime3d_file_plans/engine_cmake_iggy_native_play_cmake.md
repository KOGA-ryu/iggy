# `engine/cmake/iggy_native_play.cmake`

Purpose: register native runtime3d integration sources when they are added.

Must contain changes when native integration begins:

- Add `apps/native_play/Native3DProductSession.cpp` to native play target.
- Keep existing Vulkan, SDL2, and `glslc` gating unchanged.

Construction rules:

- Do not make runtime3d build depend on native play.
- Native play may depend on runtime3d through `iggy_engine`.

Completion:

- `iggy_native_play` builds with runtime3d native wrapper when dependencies are
  available.

