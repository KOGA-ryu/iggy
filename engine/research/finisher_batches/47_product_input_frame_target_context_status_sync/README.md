# 47 Product Input Frame Target Context Status Sync

Status: complete.

## Goal

Sync planning and API docs after
`RuntimeGameplayProductInputFrameTargetContext` landed as a runtime/product
opt-in pre-frame enrichment helper.

## Integrated Surface

- Status enum:
  `RuntimeGameplayProductInputFrameTargetContextStatus::{Unchanged, NoEligiblePrimaryTile, TargetProjected}`.
- Input carries `RuntimeGameplayProductPlayModeState`,
  `RuntimeGameplayProductInputFrame2D`, `InteractionTargetSpatialQuery2DConfig`,
  and `InteractionReach2DConfig`.
- Result carries status, copied/enriched frame, primary tile diagnostics
  (`hasPrimaryTileEvent`, event index, tile), nested
  `RuntimeGameplayProductInteractionTargetQueryResult`, and nested
  `RuntimeGameplayProductInputTargetContextResult`.

## Behavior And Policy

- Starts by copying `input.frame`.
- Scans frame events in order for eligible events where control is `PrimaryTile`,
  kind is `Pressed`, and `hasTile = true`.
- Latest eligible `PrimaryTile` wins; event index and tile are recorded.
- If no eligible event exists, returns `NoEligiblePrimaryTile` with copied
  frame/context/events unchanged.
- `PrimaryTile` with missing tile payload and `PrimaryTile` release are
  ineligible and preserved for existing adapter behavior.
- Eligible event queries `RuntimeGameplayProductInteractionTargetQuery` with
  `kind = TileCenter`, the chosen tile, and forwarded spatial/reach configs.
- Applies `RuntimeGameplayProductInputTargetContext` with the input frame's base
  binding context and the query result.
- Replaces only copied frame binding context; preserves all events unchanged and
  in order.
- Returns `TargetProjected` only when target-context projection succeeds;
  otherwise returns `Unchanged` while preserving diagnostics.
- Reach is diagnostic only, not a projection gate.
- Does not synthesize `Interact`/`Inspect`, does not inject target ids into
  events, and does not change `PrimaryTile` semantics.
- Does not mutate input frame, play state, accumulator state, product/gameplay
  state, target query result, or base binding context.

## Tests Covered

- No `PrimaryTile`.
- Missing tile payload and release ineligible.
- Valid target tile projection.
- Tile miss diagnostics.
- Disabled/out-of-range unchanged paths.
- Latest eligible tile wins.
- Selected target preservation and selected-over-hover binding priority.
- Hover replacement and preservation.
- Reach not gating projection.
- Event order and payload preservation.
- Frame/play-state immutability.
- Targetless `Interact` binding through projected hover without adapter changes.

## Boundaries Preserved

- Runtime/product opt-in pre-frame helper only.
- No Qt changes.
- No automatic call from `RuntimeGameplayProductFrameRequest` or
  `RuntimeGameplayProductPlaySurfaceFrame`.
- No `PrimaryTile` to `Interact` conversion.
- No target id injection into `Interact`/`Inspect` events.
- No selected target state.
- No persistent hover state.
- No input adapter behavior changes.
- No command/effect execution changes.
- No render drawing, target highlighting, sprite/material policy,
  pause/retry/reset, completion/failure, save-load,
  package scanning/watching/discovery, or source mutation.
- No persistence of query/context/enriched-frame results in runtime/session/
  gameplay/product-loop/play-mode/saves/settings/scene-UI truth.

## Next Framing

- This helper is only an opt-in pre-frame enrichment step between accumulator
  output and frame request.
- Future gates may decide whether Qt/manual Step/pump call it, whether frame
  request/play surface should ever own enrichment, whether hover lifecycle
  clearing is needed, whether click target means selection/interact, whether
  reach gates interaction, and how UI/render highlighting should consume
  diagnostics.
- Render drawing, `PrimaryPoint` behavior, sprites/assets/materials,
  UX/save/completion/package/source gates remain separate.

## Verification

- `git diff --check`
- `git status --short --branch`
