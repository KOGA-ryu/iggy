# 38 Product Input Accumulator Status Sync

Status: complete.

Goal: update planning and API docs after Product Held Input Accumulator
integration.

Integrated behavior recorded:
- Added shell-neutral `RuntimeGameplayProductInputAccumulator` with
  return-by-value transient state.
- Accumulator state stores product-level data only: held
  `RuntimeGameplayProductInputControl2D` values and pending
  `RuntimeGameplayProductInputEvent2D` one-shot events.
- Held movement controls are exactly `MoveNorth`, `MoveSouth`, `MoveWest`, and
  `MoveEast`.
- One-shot controls are exactly `Interact`, `Inspect`, `Wait`, and `Cancel`.
- Held movement press adds control, duplicate held press is suppressed, release
  removes held movement, and release of non-held movement is a no-op.
- One-shot press queues an event and one-shot release is a no-op.
- Frame output emits ordinary `Pressed` events for held movement controls each
  frame and pending one-shot events once, preserving held press order before
  one-shot press order.
- `buildFrame(...)` carries supplied `PlayerInputBindingContext2D`, preserves
  held controls, and drains one-shots in returned state.
- `clear(...)` returns empty transient state.
- `PrimaryPoint` and `PrimaryTile` remain out of accumulator v1.
- Qt stores accumulator state instead of latest-edge `productInputFrame_`.
- Qt key recording maps keys to product controls/events without clearing per key
  event.
- Focus-disabled state clears accumulator state.
- Manual Step builds request frame from accumulator output plus projected binding
  context, stores returned accumulator state before executing the frame request,
  calls `RuntimeGameplayProductFrameRequest` exactly once, preserves held
  movement across steps, and drains one-shots after each executed Step.

Boundaries preserved:
- No automatic app/tick loop or frame pump.
- No held-key timing/cadence/rate policy beyond maintaining held product
  controls and emitting them when a caller asks for a frame.
- No mouse screen-to-world/tile mapping.
- No `PrimaryPoint` or `PrimaryTile` synthesis.
- No selected/hovered target discovery, interaction reach lookup, or target
  search.
- No player sprite or modern NPC actor render projection expansion.
- No pause/retry/reset/completion/failure/save-load productization.
- No package scanning/watching/discovery or source mutation.
- No raw Qt key/event persistence; accumulator state stores product
  controls/events only.
- Accumulator state is not persisted in runtime/session/gameplay/product-loop/
  play-mode state, snapshots, saves, settings, or scene/UI models.
- No camera/presentation/render-frame data persistence as
  gameplay/session/product-loop/play-mode/save truth.
- No product loop, frame request, or play mode stepping semantic changes.
- No UI model/settings exposure for accumulator counters or state.

Verification:
- `git diff --check`
- `git status --short --branch`
