# 30 Product Play UI Projection Status Sync

Status: complete.

Goal: update planning and API docs after the read-only product play UI
projection and context seam integrated.

Scope:
- Docs/status sync only.
- Mark `UiProductPlayModePanelModel` as complete for read-only product play
  build/state/latest-frame projection.
- Record `UiFeatureContext` product play pointers/presence helper and
  `feature:product_play` / `panel:product_play` runtime workspace registration.
- Record that product play panel population requires product play context,
  emits no missing-context diagnostic, and is hidden by default through existing
  assignments/settings behavior.
- Frame remaining Product Loop/UI work as product shell/UI launch and focused
  input ownership, optional Qt/raw-device adapter after shell/focus ownership is
  scoped, camera lifecycle/presentation policy, player sprite and modern NPC
  actor render projection, automatic app/tick loop/frame pump, pause/retry/reset
  policy, completion/failure evaluator, and save/load productization.

Hard stops preserved:
- No production code changes.
- Scene/UI read-only projection and context seam only; not Qt shell behavior,
  product launch mode, UI adapter executing frames, or automatic app/tick loop.
- No `--play`, Qt launch behavior, app shell behavior, CLI behavior, or raw
  Qt/device event mapping.
- No product loader/file/package/TOML APIs called from scene/UI model code.
- No calls to `RuntimeGameplayProductPlayMode::frame`,
  `RuntimeGameplayProductPlaySurfaceFrame::build`, loader APIs, or product
  step/run functions from scene/UI.
- No mutation of product play mode state or runtime/gameplay state.
- No raw input/camera/presentation/render-frame persistence in
  gameplay/session/product-loop/play-mode/save truth.
- No hidden default camera/render config inside the UI model.
- No automatic frame pump/tick loop, pause/retry/reset, completion/failure,
  save/load productization, package scanning/watching/discovery, source
  mutation, or new gameplay semantics.

Verification:
- `git diff --check`
- `git status --short --branch`
