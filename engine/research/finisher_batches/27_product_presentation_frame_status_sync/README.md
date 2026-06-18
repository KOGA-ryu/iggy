# 27 Product Presentation Frame Status Sync

Status: complete.

Goal: update planning and API docs after Product Loop Packet 4-B integrated the
projection-only product presentation frame wrapper.

Scope:
- Docs/status sync only.
- Mark `RuntimeGameplayProductPresentationFrame` as complete for projecting
  loaded product loop state plus caller-owned `CameraState` and
  `LevelRenderFrame2DConfig` into a `LevelRenderFrame2DResult`.
- Record not-loaded behavior as `NotLoaded`, camera echo/copy, and default
  level frame.
- Frame remaining Product Loop work as product shell/play surface, optional
  Qt/raw-device adapter after product surface ownership is scoped, camera
  lifecycle/presentation policy, player sprite and modern NPC actor render
  projection, pause/retry/reset policy, completion/failure evaluator, and
  save/load productization.

Hard stops preserved:
- No production code changes.
- No gameplay stepping, product-loop state/index/context changes, or new
  gameplay semantics.
- Camera and presentation remain caller-owned and are not
  gameplay/session/product-loop/save truth.
- No Qt shell/play mode, raw input mapping, camera lifecycle/follow/rig/clamp
  ownership, screen/world transforms, player/modern NPC actor render projection,
  save/load productization, pause/retry/reset/completion/failure semantics,
  package scanning/watching, source mutation, UI/CLI, or IO.

Verification:
- `git diff --check`
- `git status --short --branch`
