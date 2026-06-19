# 50 Qt Product Viewport Render Drawer Status Sync

Status: complete.

## Goal

Sync planning and API docs after the thin Qt-only product viewport render command
drawer landed.

## Integrated Surface

- `QFrame#productViewport` is now a Qt-local paint-capable widget that draws
  existing latest product play frame render commands.
- The drawer consumes only stable, transient app-shell latest-frame data from
  `latestProductPlayModeFrame_`:
  - `surface.presentation.levelFrame.commands.commands`
  - `surface.presentation.levelFrame.cameraView.bounds`
- Latest-frame `levelFrame.cameraView.bounds` is the visible world bounds, and
  current widget size maps command world bounds to pixel rectangles.
- Command world bounds and mapped `QRectF` values are normalized defensively
  before painting.
- X and y are mapped directly with no flip, preserving tile/world orientation.
- Commands are skipped when there is no latest frame, widget size is
  non-positive, or camera view width/height is zero; the background still paints
  normally.
- The drawer iterates the command list in existing vector order with no Qt-side
  sorting by layer or order.
- Only `RenderCommand2DType::Quad` commands are drawn, as untextured flat
  rectangles.
- Texture payloads are ignored; textures are not loaded or sampled.
- Material ids use Qt-local hardcoded debug colors for floor, wall, player, NPC
  actor, legacy NPC, and fallback materials.
- This remains a temporary app-shell/debug-material renderer over existing
  latest-frame data.

## Boundaries Preserved

- Qt-only presentation consumer.
- No runtime/product/scene/render command API changes.
- No frame execution from paint events.
- No automatic tick or pump changes.
- No new render commands or gameplay semantic changes.
- No input behavior changes, new mouse behavior, target highlighting,
  selected/hovered overlay, click-to-interact, or reach-gated behavior.
- No texture loading, sprite animation, material registry, asset package policy,
  package discovery, or asset loading.
- No persistence of viewport geometry, painter state, render frames, camera,
  presentation state, raw input, diagnostics, or Qt state in runtime/session/
  gameplay/product-loop/play-mode/saves/settings/scene-UI truth.
- No Qt types outside Qt shell.
- This is not the long-term renderer.

## Next Framing

- Future gates may choose target highlighting/overlays now that a drawing surface
  exists.
- Real renderer ownership, texture/sprite/material asset policy,
  click-to-interact/reach-gated behavior, selected/durable hover state,
  `PrimaryPoint` policy, UX/save/completion/package/source, and canvas polish
  remain separate.

## Verification

- `git diff --check`
- `git status --short --branch`
