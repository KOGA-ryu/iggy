# 44 Interaction Target Spatial Query Status Sync

Status: complete.

## Goal

Sync planning and API docs after `InteractionTargetSpatialQuery2D` landed as a
scene-only spatial lookup primitive.

## Integrated Surface

- `InteractionTargetSpatialQuery2D` provides pure spatial lookup over
  `InteractionTarget2DRegistry`.
- Status values are `Found` and `NotFound`.
- `InteractionTargetSpatialQuery2DConfig` carries `extraRadius`, defaulting to
  `0.0F`.
- Results carry status, target id, copied target payload, target index,
  distance, allowed distance, and `hasTarget()`.
- Query entry points are `find(registry, point, config)` and
  `findTileCenter(registry, tile, config)`.

## Query Policy

- Linear scan over `registry.targets()` in registry order.
- Enabled targets only.
- Euclidean distance from the query point to target position.
- `allowedDistance = max(0, target.radius) + max(0, config.extraRadius)`.
- Matches use `distance <= allowedDistance`, including zero-radius
  same-position hits.
- Nearest eligible target wins.
- Exact ties preserve the first registry entry because the best match updates
  only on strict smaller distance.
- Empty registry, no in-radius target, and only disabled in-radius targets return
  `NotFound`.
- `findTileCenter(...)` delegates through `tileCenter(tile)`.

## Tests Covered

- Empty/default result.
- Enabled target hit.
- Exact boundary.
- Outside radius.
- Zero-radius behavior.
- Nearest overlap.
- Registry-order tie.
- Disabled target ignore.
- Positive and negative `extraRadius` clamp behavior.
- Tile-center parity.
- Registry immutability.

## Boundaries Preserved

- Pure scene/interaction primitive only.
- No Qt changes.
- No runtime/product changes.
- No click-to-interact execution.
- No selected/hovered target persistence or product input context projection
  changes.
- No player input binding fallback changes.
- No command/gate/effect semantics changes.
- No actor reach, line-of-sight, occupancy, pathfinding, kind-priority, z-order,
  or layer policy.
- No render drawing, sprite/assets/material policy, pause/retry/reset,
  completion/failure, save/load, package scanning/watching/discovery, or source
  mutation.
- No persistence of spatial query results in runtime/session/gameplay/
  product-loop/play-mode/saves/settings/scene-UI truth.

## Next Framing

- Spatial query is only the lookup primitive.
- Later gates may wire hovered/selected target context, Qt click-to-target
  policy, reach checks, or interaction execution.
- Render command drawing/canvas interpretation, textured sprites/assets/material
  policy, `PrimaryPoint` behavior, UX/save/completion/package/source gates all
  remain separate.

## Verification

- `git diff --check`
- `git status --short --branch`
