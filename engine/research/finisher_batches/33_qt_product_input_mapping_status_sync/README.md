# 33 Qt Product Input Mapping Status Sync

Status: complete.

Goal: update planning and API docs after Product Loop/UI 4-G-B Qt raw-device
input mapping integration.

Integrated behavior recorded:
- Qt shell maps supported keyboard press/release events for ready, focused
  `--play` sessions into the app-shell-owned transient
  `RuntimeGameplayProductInputFrame2D productInputFrame_`.
- Only product input events are stored; raw `QKeyEvent` objects or pointers are
  not persisted.
- Mapping is gated by product play mode presence, ready play-mode build status,
  and enabled product input focus.
- Disabling focus clears the transient input frame; failed/not-ready play state
  and disabled focus record no events.
- The frame is bounded to the latest event by clearing before appending one
  mapped event.
- Auto-repeat and unsupported keys are ignored.
- Arrow/WASD map to cardinal movement, `E`/Return/Enter to interact, `I` to
  inspect, Space to wait, and Escape to cancel.
- Binding context remains default; no current-player tile, selected target,
  hovered target, scene/UI model exposure, or settings exposure was added.

Boundaries preserved:
- No `RuntimeGameplayProductPlayMode::frame(...)` call.
- No `RuntimeGameplayProductPlaySurfaceFrame::build(...)` call.
- No `RuntimeGameplayProductInputAdapter::map(...)` call from Qt production
  code.
- No `PlayerInputBinding2D::bind(...)` call from Qt.
- No camera/render config, presentation frames, latest-frame results, product
  frame steps, manual step actions, app tick loop, or frame pump.
- No mouse position to world/tile mapping and no `PrimaryPoint` or
  `PrimaryTile` mapping.
- No raw Qt event or product input event persistence in
  runtime/session/gameplay/product-loop/play-mode state, snapshots, saves,
  settings, or scene/UI models.
- No held-key cadence, input-repeat gameplay policy, command/gate execution,
  pause/retry/reset, completion/failure, save/load productization, package
  scanning/watching/discovery, source mutation, or gameplay semantics.
- Qt types remain inside `engine/apps/qt_shell`.

Verification:
- `git diff --check`
- `git status --short --branch`
