# 43 Qt Product Primary Tile Mouse Status Sync

Status: complete.

Goal: update planning and API docs after Thin Qt Product Mouse PrimaryTile
Consumer integration.

Integrated behavior recorded:
- `productViewport_` installs a viewport-only event filter in product play
  sessions.
- Focused, ready left mouse press on `productViewport_` records exactly one
  transient `RuntimeGameplayProductInputEvent2D` into
  `RuntimeGameplayProductInputAccumulator`.
- Event fields are:
  - `control = RuntimeGameplayProductInputControl2D::PrimaryTile`
  - `kind = RuntimeGameplayProductInputEventKind::Pressed`
  - `hasTile = true`
  - `tile = projection.tile`
- The handler accepts and returns true only when a product event is recorded;
  otherwise it falls through.
- Product Step and Product Frame Pump consume the accumulator later; the mouse
  handler does not execute a frame.

Gating recorded:
- `watched == productViewport_`
- `QEvent::MouseButtonPress`
- left button only
- ready product play through existing focus availability
- `Product Input Focus` enabled
- non-null viewport
- viewport width and height greater than zero
- configured product camera-view axes nonzero

Coordinate/camera correction recorded:
- Qt pixel-local mouse coordinates are normalized into the configured product
  camera-view span before calling `RuntimeGameplayProductPointerProjection`.
- Projection viewport size comes from
  `productPresentationCameraConfig().cameraView.viewportSize`, not raw widget
  pixel size.
- `QMouseEvent::position()` is normalized by `productViewport_` pixel width and
  height into `fabs(viewSize.x/y)`.
- `RuntimeGameplayProductPresentationCamera` is built read-only from
  `productPlayState_` and `productPresentationCameraConfig()`.
- The mouse handler stores no camera or pointer state.
- Qt types are converted at the Qt boundary only; no Qt types enter runtime or
  product APIs.

Boundaries preserved:
- Current mouse policy is `PrimaryTile` only.
- No `PrimaryPoint` or world-point payload.
- No right-click, middle-click, mouse move, wheel, double-click, drag, or hover
  behavior.
- No target discovery/search/reach, selected/hovered target context, interaction
  execution, hover/drag tools, sprites/assets/material policy, pause/retry/
  reset, completion/failure, save/load, package discovery, or source mutation.
- No render command drawing or canvas polish.
- No runtime/product, scene/player, scene/npc, scene/ui, save/load, CMake, or
  test-source changes are part of this docs packet.
- No persistence of raw input, accumulator state, mouse state, camera,
  presentation, render frames, Qt state, actor render config, viewport geometry,
  or projected pointer data in runtime/session/gameplay/product-loop/play-mode
  saves, snapshots, settings, or scene/UI model truth.

Next framing:
- Target discovery/search/reach and interaction execution remain separate gates.
- Render canvas drawing and latest-frame command interpretation remain separate.
- Textured sprites/assets/material registry policy remains separate.
- Point-vs-tile and `PrimaryPoint` behavior remain separate.
- UX/save/completion/package/source mutation gates remain separate.

Verification:
- `git diff --check`
- `git status --short --branch`
