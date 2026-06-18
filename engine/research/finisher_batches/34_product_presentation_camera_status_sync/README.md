# 34 Product Presentation Camera Status Sync

Status: complete.

Goal: update planning and API docs after Product Loop/UI 4-H product
presentation camera policy integration.

Integrated behavior recorded:
- `RuntimeGameplayProductPresentationCamera` is an app-neutral runtime/product
  policy.
- It chooses transient caller-owned `CameraState` plus
  `LevelRenderFrame2DConfig` from product play state and caller-owned config.
- It supports not-loaded previous/fallback camera selection.
- It supports loaded player initialization.
- It supports previous-camera player follow through existing `CameraRig`.
- It supports follow-disabled previous/fallback behavior.
- It reports clamp behavior through result flags from `CameraRig`.
- It forwards render config fields including view/camera view config, NPC
  command flag, tile chunk cache flag, and cache pointer.

Boundaries preserved:
- No Qt/UI/CLI behavior.
- No step button, manual frame execution, app tick loop, or frame pump.
- No `RuntimeGameplayProductPlayMode::frame(...)` call.
- No `RuntimeGameplayProductPlaySurfaceFrame::build(...)` call from the policy.
- No product input adapter or binding execution.
- No raw input persistence or Qt input mapping changes.
- No mouse screen-to-world/tile mapping.
- No camera, presentation, or render-frame data stored in
  `RuntimeGameplayState`, `RuntimeSessionState`,
  `RuntimeGameplayProductLoopState`, `RuntimeGameplayProductPlayModeState`,
  snapshots, saves, scene/UI models, or settings.
- No player sprite or modern `RuntimeGameplayState::npcActors` render
  projection.
- No pause/retry/reset, completion/failure, save/load productization, package
  scanning/watching/discovery, source mutation, or gameplay semantics.

Verification:
- `git diff --check`
- `git status --short --branch`
