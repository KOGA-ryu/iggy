# 37 Product Input Context Projection Status Sync

Status: complete.

Goal: update planning and API docs after Product Play Input Binding Context
Projection integration.

Integrated behavior recorded:
- Added app-neutral runtime/product `RuntimeGameplayProductInputContext`
  projection.
- Statuses are `NotLoaded`, `LoadedWithoutPlayer`, and `Projected`.
- Projection returns default `PlayerInputBindingContext2D` gates.
- Projection sets only `hasCurrentPlayerTile/currentPlayerTile` when product
  play state is loaded and `currentState.session.hasPlayer` is true, using
  existing `playerTile(...)`.
- Qt manual Step enriches only a local request-frame copy with projected
  binding context before `RuntimeGameplayProductFrameRequest`.
- Qt does not persist derived binding context back into `productInputFrame_`;
  that member remains latest raw/product event storage only and is still cleared
  after executed Step.
- Projected context is transient request input only.

Boundaries preserved:
- No automatic app/tick loop or frame pump.
- No held-key cadence, repeat behavior, or input accumulator.
- No mouse screen-to-world/tile mapping, `PrimaryPoint`, or `PrimaryTile`
  synthesis.
- No selected/hovered target discovery, interaction target search, or reach
  lookup.
- No player sprite or modern NPC render projection expansion.
- No pause/retry/reset/completion/failure/save-load productization.
- No package scanning/watching/discovery or source mutation.
- No raw input or binding context persistence in
  runtime/session/gameplay/product-loop/play-mode state, snapshots, saves,
  settings, or scene/UI models.
- No camera/presentation/render-frame data persistence as
  gameplay/session/product-loop/play-mode/save truth.
- No product loop, frame request, or play mode stepping semantic changes.

Verification:
- `git diff --check`
- `git status --short --branch`
