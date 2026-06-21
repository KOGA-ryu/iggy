# `engine/apps/native_play/Native3DProductSession.hpp`

Purpose: declare the native shell wrapper around `Runtime3DSessionState`.

Must contain:

- Native app namespace matching `native_play` style.
- Config struct for runtime3d session launch.
- Class holding runtime3d session state, transient normalized input, and timing
  bridge data.
- Methods for loaded state, stepping, command proposal, camera mode state, and
  scene projection access.

Construction rules:

- May include runtime3d headers.
- Must not expose SDL events in runtime3d types.
- Raw mouse/keyboard/controller state stays transient in native shell.

Completion:

- Native app can own a runtime3d session without old 2D session ownership.

