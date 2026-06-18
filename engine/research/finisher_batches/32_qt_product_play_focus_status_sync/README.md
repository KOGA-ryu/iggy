# 32 Qt Product Play Focus Status Sync

Status: complete.

Goal: update planning and API docs after the Qt product play focus toggle
integrated.

Scope:
- Docs/status sync only.
- Mark the View-menu `Product Input Focus` action as complete for ready `--play`
  sessions.
- Record that toggling updates only durable current `productPlayState_` via
  `RuntimeGameplayProductPlayMode::withInputFocus`, keeps product play context
  pointers stable, clears latest frame to null, and refreshes the read-only
  product play panel.
- Record failed-load/non-ready behavior as disabled/non-applicable without
  inventing ready state.
- Frame remaining work as Qt/raw-device input mapping, product frame stepping/
  frame pump, camera lifecycle/presentation policy, player sprite and modern NPC
  actor render projection, pause/retry/reset policy, completion/failure
  evaluator, and save/load productization.

Hard stops preserved:
- No production code changes.
- No `RuntimeGameplayProductPlayMode::frame(...)` call.
- No `RuntimeGameplayProductPlaySurfaceFrame::build(...)` call.
- No Qt key/mouse/focus event routing into `RuntimeGameplayProductInputEvent2D`.
- No product input events, default camera/render config, presentation frames,
  latest-frame synthesis, frame stepping/manual step, app tick loop, or frame
  pump.
- No raw input persistence in runtime/session/gameplay/product-loop/play-mode
  state, snapshots, saves, or UI models.
- No camera/presentation/render-frame data persistence as
  gameplay/session/product-loop/play-mode/save truth.
- No pause/retry/reset, completion/failure, save/load productization, package
  scanning/watching/discovery, source mutation, or gameplay semantics.
- No settings persistence or keyboard shortcut.

Verification:
- `git diff --check`
- `git status --short --branch`
