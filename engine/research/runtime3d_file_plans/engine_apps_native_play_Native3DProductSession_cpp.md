# `engine/apps/native_play/Native3DProductSession.cpp`

Purpose: implement native-to-runtime3d session bridge.

Must contain:

- Session creation from runtime3d state or legacy adapter.
- Native timing bridge into `Runtime3DClock`.
- Normalized input to runtime3d command proposals.
- Camera mode policy application.
- Transient input clear on camera mode changes.
- Scene projection retrieval for renderer-facing bridge.

Construction rules:

- Keep command admission in runtime3d, not native shell.
- Keep renderer resource ownership out.
- Do not persist raw input.

Completion:

- Native play can step runtime3d session behind a switch or mode.

