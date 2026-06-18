# 45 Product Interaction Target Query Status Sync

Status: complete.

## Goal

Sync planning and API docs after
`RuntimeGameplayProductInteractionTargetQuery` landed as a runtime/product
read-only report surface.

## Integrated Surface

- Query kind enum: `None`, `Point`, `TileCenter`.
- Status enum: `NotLoaded`, `MissingQuery`, `TargetNotFound`, `TargetFound`.
- Input carries `RuntimeGameplayProductPlayModeState`, query kind, point, tile,
  `InteractionTargetSpatialQuery2DConfig`, and `InteractionReach2DConfig`.
- Result carries status, kind, nested spatial result, target id, copied target
  payload, `hasTarget`, `hasPlayer`, `hasReach`, `reachable`, target query
  result, and reach result.

## Behavior And Policy

- Not-loaded returns `NotLoaded` and does not query targets.
- `kind = None` returns `MissingQuery` and does not query targets.
- `TileCenter` queries `state.loop.currentState.interaction.targets` via
  `InteractionTargetSpatialQuery2D::findTileCenter(...)`.
- `Point` queries via `InteractionTargetSpatialQuery2D::find(...)`.
- Spatial `NotFound` returns `TargetNotFound`, preserves the nested spatial
  result, and has no target or reach.
- Spatial `Found` returns `TargetFound`, copies target id/payload, and sets
  `hasTarget = true`.
- Reach annotates found targets only; it does not block target visibility.
- If current state has a player, the report constructs a found
  `InteractionTargetQuery2DResult`, evaluates `InteractionReach2D` from current
  player position with reach config, and sets `hasPlayer`, `hasReach`, and
  `reachable`.
- If there is no player, lookup remains `TargetFound` with `hasTarget = true`,
  `hasPlayer = false`, `hasReach = false`, and `reachable = false`.
- There is no top-level `LoadedWithoutPlayer` status.
- It does not use `InteractionPlan2D` for command planning.
- It does not mutate play state, gameplay state, interaction registry, target
  payload, input frames, accumulator, binding context, or selected/hovered
  fields.

## Tests Covered

- Not-loaded.
- Missing query.
- Tile query.
- Point query.
- Disabled, out-of-range, and empty-registry target-not-found paths.
- `extraRadius` forwarding.
- Player reach reachable and out-of-range paths.
- No-player visibility without reach.
- Immutability.

## Boundaries Preserved

- Runtime/product read-only report only.
- No Qt changes.
- No selected/hovered target context mutation or projection.
- No product input adapter behavior changes.
- No `PrimaryTile` to `Interact` conversion.
- No click-to-interact execution.
- No command/gate/effect semantics changes.
- No persistent selection, hover, mouse, pointer, camera, render-frame,
  query-result, or target-search state.
- No render drawing/canvas work.
- No textured sprite/material/asset policy.
- No pause/retry/reset/completion/failure/save-load/package/source mutation.

## Next Framing

- This provides an app-neutral query/report surface only.
- Later gates may choose how query results feed transient hovered/selected target
  context, Qt click-to-target policy, explicit interact target synthesis,
  reach-gated interaction execution, panel/render highlighting, and related UI
  behavior.
- Render command drawing, `PrimaryPoint` behavior, sprites/assets/materials,
  UX/save/completion/package/source gates remain separate.

## Verification

- `git diff --check`
- `git status --short --branch`
