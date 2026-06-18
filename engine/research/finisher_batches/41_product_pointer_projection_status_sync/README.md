# 41 Product Pointer Projection Status Sync

Status: complete.

Goal: update planning and API docs after Product Pointer Tile Input Mapping
Option A integration.

Integrated behavior recorded:
- Added Qt-free `RuntimeGameplayProductPointerProjection`.
- The projection maps viewport-local points through existing `CameraView`
  normalization to a world point plus `tileForPoint(...)` tile.
- It uses `CameraState`, `CameraView`, `CameraViewConfig`, and `TileCoord` only;
  no Qt types.
- It preserves `CameraView` behavior for negative viewport dimensions,
  non-positive zoom using effective camera view behavior, and zero-axis
  degenerate viewports without crashing.
- It returns projection status/flags, world point, and tile so a future consumer
  can choose `PrimaryTile`, `PrimaryPoint`, or both under a separate policy
  gate.
- `RuntimeGameplayProductInputAccumulator` now preserves explicit pressed
  `PrimaryPoint` and `PrimaryTile` events as payload-carrying pending one-shots
  drained on the next `buildFrame(...)`.
- Primary releases are no-ops.
- Existing held movement and existing one-shots `Interact`, `Inspect`, `Wait`,
  and `Cancel` remain unchanged.

Boundaries preserved:
- Runtime/product pointer projection plus accumulator event preservation only.
- No Qt mouse consumer or Qt file changes.
- No `QMouseEvent`, `QPoint`, QWidget coordinate use, or Qt types in runtime,
  scene, or product APIs.
- No product viewport/canvas coordinate owner yet.
- No target discovery/search/reach, hover selection, interaction execution, drag
  tools, or mouse-to-target policy.
- No `PrimaryPoint` or `PrimaryTile` synthesis beyond preserving explicit events
  handed to the accumulator.
- No textured sprite/assets/material registry/package policy.
- No pause/retry/reset/completion/failure/save-load productization.
- No package discovery/source mutation.
- No persistence of raw input, accumulator state, mouse state, camera,
  presentation, render frames, Qt state, actor render config, or projected
  pointer data in runtime/session/gameplay/product-loop/play-mode saves,
  snapshots, settings, or scene/UI model truth.
- Runtime/product remains one normalized semantic frame plus projection-only
  render output; no product loop/frame request/play mode semantic changes.

Next framing:
- A future Qt consumer should likely emit focused ready-play `PrimaryTile` into
  the transient accumulator using this helper, but that product viewport/canvas
  ownership and point-vs-tile policy are separate gates.
- Remaining gates include product viewport/canvas owner plus thin Qt mouse
  consumer, target discovery/search/reach and hover/selection workflows,
  textured sprite/animation/material/asset policy, pause/retry/reset/
  completion/failure, save/load productization, package scanning/watching/
  discovery/source mutation, and UI/render canvas polish.

Verification:
- `git diff --check`
- `git status --short --branch`
