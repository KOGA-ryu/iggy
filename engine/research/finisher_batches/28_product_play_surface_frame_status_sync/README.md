# 28 Product Play Surface Frame Status Sync

Status: complete.

Goal: update planning and API docs after the runtime-only product play-surface
frame facade integrated.

Scope:
- Docs/status sync only.
- Mark `RuntimeGameplayProductPlaySurfaceFrame` as complete for one
  caller-requested, app-neutral play frame.
- Record that it composes existing public surfaces:
  `RuntimeGameplayProductInputAdapter`, `PlayerInputBinding2D`,
  `RuntimeGameplayProductLoop` one-frame step with per-step context override,
  and `RuntimeGameplayProductPresentationFrame`.
- Record focused, not-loaded, exhausted/no-frame, and unfocused behavior plus
  nested adapter/binding/step/presentation results and ignored input count.
- Frame remaining Product Loop work as product shell/play mode and focused input
  ownership, optional Qt/raw-device adapter after product shell/focus ownership
  is scoped, camera lifecycle/presentation policy, player sprite and modern NPC
  actor render projection, automatic app/tick loop, pause/retry/reset policy,
  completion/failure evaluator, and save/load productization.

Hard stops preserved:
- No production code changes.
- App-neutral runtime/product facade only; not a Qt shell, product launch mode,
  automatic tick loop, UI adapter, or CLI behavior.
- No Qt/UI behavior, raw OS event types, or raw input persistence in runtime
  session/gameplay/product-loop state, snapshots, saves, or UI models.
- No camera lifecycle/follow/rig/clamp ownership and no presentation/render-frame
  persistence as gameplay/session/product-loop/save truth.
- No product loader/loop semantic changes beyond composing public surfaces.
- No player sprite or modern `RuntimeGameplayState::npcActors` render projection.
- No pause/retry/reset, completion/failure, save/load productization, package
  scanning/watching/discovery, source mutation, or new gameplay semantics.

Verification:
- `git diff --check`
- `git status --short --branch`
