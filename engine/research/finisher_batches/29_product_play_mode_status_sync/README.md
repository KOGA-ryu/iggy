# 29 Product Play Mode Status Sync

Status: complete.

Goal: update planning and API docs after the runtime-only product play-mode
state wrapper integrated.

Scope:
- Docs/status sync only.
- Mark `RuntimeGameplayProductPlayMode` as complete for durable app-neutral
  play-mode state over `RuntimeGameplayProductLoopState loop` plus
  `hasInputFocus`.
- Record build, focus-toggle, and frame delegation behavior over existing
  product loop and play-surface public surfaces.
- Frame remaining Product Loop work as product shell/UI launch and focused input
  ownership, optional Qt/raw-device adapter after shell/focus ownership is
  scoped, camera lifecycle/presentation policy, player sprite and modern NPC
  actor render projection, automatic app/tick loop/frame pump, pause/retry/reset
  policy, completion/failure evaluator, and save/load productization.

Hard stops preserved:
- No production code changes.
- Runtime/product app-neutral state wrapper only; not Qt shell behavior, product
  launch mode, UI adapter, or automatic app/tick loop.
- No raw Qt/OS event types.
- No raw input persisted in runtime session/gameplay/product-loop/play-mode
  state, snapshots, saves, or UI models; only durable focus is stored.
- No camera/presentation/render-frame data persisted as
  gameplay/session/product-loop/play-mode/save truth.
- No camera lifecycle/follow/rig/clamp ownership.
- No product loader/file/package/TOML calls from play mode.
- No product loop semantic changes beyond carrying returned loop state.
- No player sprite or modern `RuntimeGameplayState::npcActors` render projection.
- No pause/retry/reset, completion/failure/win/lose semantics, save/load
  productization, package scanning/watching/discovery, source mutation, or new
  gameplay semantics.

Verification:
- `git diff --check`
- `git status --short --branch`
