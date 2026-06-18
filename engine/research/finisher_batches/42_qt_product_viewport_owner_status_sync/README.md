# 42 Qt Product Viewport Owner Status Sync

Status: complete.

Goal: update planning and API docs after Qt Product Viewport Owner integration.

Integrated behavior recorded:
- Added Qt-shell-owned `QFrame *productViewport_` in `IggyQtShellWindow`.
- `buildMainSlot()` creates an inert `QFrame#productViewport` only when
  `hasProductPlayMode_` is true.
- The viewport is present for product play sessions, including failed/not-ready
  play sessions, as an inert surface only; it does not imply input readiness.
- `productViewport_` is reset to `nullptr` during main-slot rebuild when no
  viewport is created, so refresh/rebuild does not intentionally leave a stale
  pointer.
- The viewport uses zero-margin/zero-spacing layout and expanding size policy
  inside `QFrame#mainSlot`.
- Styling is local to `QFrame#productViewport`.
- The viewport creates a stable future event/render target boundary for product
  play shell work.

Boundaries preserved:
- Qt-only, temporary app-shell viewport/canvas ownership for product play
  sessions.
- No Qt mouse consumer.
- No event filter or mouse handler installed on `productViewport_`.
- No calls to `RuntimeGameplayProductPointerProjection` from Qt.
- No `QMouseEvent` to product input mapping.
- No `PrimaryPoint` or `PrimaryTile` synthesis from Qt.
- No render command drawing or canvas polish.
- No target discovery/search/reach, hover/selection, drag tools, sprites/assets/
  material policy, pause/retry/reset, completion/failure, save/load, package
  discovery, or source mutation.
- No runtime/product/scene/UI API changes.
- No persistence of raw input, accumulator state, mouse state, camera,
  presentation, render frames, Qt state, actor render config, viewport geometry,
  or projected pointer data in runtime/session/gameplay/product-loop/play-mode
  saves, snapshots, settings, or scene/UI model truth.
- No Qt types into runtime, scene/player, scene/npc, scene/ui, or product
  runtime APIs.

Next framing:
- A future thin Qt mouse consumer should target `productViewport_` and may use
  `RuntimeGameplayProductPointerProjection` to emit focused ready-play
  `PrimaryTile` into the transient accumulator.
- Point-vs-tile policy, target discovery/search/reach and hover/selection
  workflows, rendering/canvas drawing, sprites/assets/material policy,
  pause/retry/reset/completion/failure, save/load productization, package
  scanning/watching/discovery, and source mutation remain separate gates.

Verification:
- `git diff --check`
- `git status --short --branch`
