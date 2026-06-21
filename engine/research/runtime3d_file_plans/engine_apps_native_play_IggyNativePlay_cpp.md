# `engine/apps/native_play/IggyNativePlay.cpp`

Purpose: wire runtime3d session mode into the native application loop.

Must contain changes when runtime3d integration begins:

- Option or mode selection for runtime3d path.
- Creation of `Native3DProductSession`.
- Keyboard/controller movement mapping.
- Mouse motion and relative mouse capture for FPS/close third-person.
- Tactical toggle, step tick, save/load, retry/reset routing.
- Draw path that consumes runtime3d scene projection.

Construction rules:

- Native shell collects raw input only.
- Do not put command admission, reach checks, target rules, or save truth here.
- Renderer still receives view/projection and draw items.

Completion:

- Native app can run runtime3d session without changing old 2D behavior by
  default.

