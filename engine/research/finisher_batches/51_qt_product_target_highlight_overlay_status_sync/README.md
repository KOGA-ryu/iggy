# 51 Qt Product Target Highlight Overlay Status Sync

Status: complete.

## Goal

Sync planning and API docs after the thin Qt-only product target highlight
overlay landed.

## Integrated Surface

- `ProductViewportWidget` accepts a const pointer to the latest transient
  `RuntimeGameplayProductInputFrameTargetContextResult` diagnostics.
- `buildMainSlot()` passes
  `hasLatestProductInputFrameTargetContext_ ? &latestProductInputFrameTargetContext_ : nullptr`
  into the viewport.
- The paint path draws existing render command quads first, then draws the
  target overlay afterward.
- The overlay draws only when all of these are available:
  - latest frame
  - diagnostics pointer
  - diagnostics target with `hasTarget = true`
  - non-degenerate camera view bounds
  - positive widget size
- The overlay uses copied diagnostics target payload only: target position and
  non-negative radius.
- Paint does not run target queries.
- The overlay builds a world-space marker from position plus/minus radius, maps
  it through the existing viewport world-to-pixel helper, and uses a Qt-local
  minimum marker for tiny or zero-radius targets.
- Reach is visual annotation only: reachable, unreachable, and no-reach choose
  different local styles; unreachable targets are still shown.
- The overlay is outline/tint only: no label, target id text, selected marker,
  trail, click animation, command execution, or richer overlay.

## Boundaries Preserved

- Qt-only visual overlay.
- No runtime/product/scene/UI/render-command API changes.
- No frame execution from paint events.
- No target query from paint events.
- No click-to-interact, target-id injection, selected target state, durable
  hover lifecycle, or reach-gated execution.
- No `PrimaryPoint`, right/middle/move/wheel/double-click/drag/hover behavior
  changes.
- No new render commands, texture loading, sprite animation, material registry,
  asset policy, package discovery, pause/retry/reset/completion/failure, or
  save/load productization.
- No persistence of target diagnostics, highlight state, viewport geometry,
  camera/presentation/render-frame data, raw input, accumulator state, Qt state,
  selected/hovered target derivations, or projected pointer data in runtime/
  session/gameplay/product-loop/play-mode/saves/settings/scene-UI truth.
- No Qt types outside Qt shell.

## Next Framing

- Future gates may choose click-to-interact behavior, explicit target-id
  synthesis, selected/durable hover lifecycle, reach-gated execution, richer
  overlays/labels, real renderer ownership, texture/sprite/material assets,
  `PrimaryPoint` policy, UX/save/completion/package/source.
- None of those follow-up gates are complete in this packet.

## Verification

- `git diff --check`
- `git status --short --branch`
