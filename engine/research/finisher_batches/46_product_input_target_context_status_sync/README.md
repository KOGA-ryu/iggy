# 46 Product Input Target Context Status Sync

Status: complete.

## Goal

Sync planning and API docs after `RuntimeGameplayProductInputTargetContext`
landed as a runtime/product projection helper.

## Integrated Surface

- Status enum:
  `RuntimeGameplayProductInputTargetContextStatus::{Unchanged, TargetProjected}`.
- Input carries a base `PlayerInputBindingContext2D` and a
  `RuntimeGameplayProductInteractionTargetQueryResult`.
- Result carries status, projected `PlayerInputBindingContext2D`, and diagnostic
  hovered target id.

## Behavior And Policy

- Starts from a copied base `PlayerInputBindingContext2D`.
- For `RuntimeGameplayProductInteractionTargetQueryStatus::TargetFound` with
  `hasTarget = true` and a non-empty `targetId`, sets
  `bindingContext.hasHoveredTargetId = true`, sets
  `bindingContext.hoveredTargetId = targetId`, returns `TargetProjected`, and
  copies the diagnostic hovered id.
- For `TargetNotFound`, `MissingQuery`, `NotLoaded`, `TargetFound` with
  `hasTarget = false`, or `TargetFound` with an empty target id, returns
  `Unchanged` with the base context copied exactly.
- Non-found or invalid statuses do not clear existing hover. This helper is
  enrichment-only and is not a hover lifecycle owner.
- Never sets or clears selected target fields.
- Preserves existing selected target, existing hover on unchanged paths, current
  player tile, and input gate flags from the base context.
- Replaces hover only on a valid found target.
- Does not require `reachable`; reach remains query/report annotation only.
- Does not mutate query report, product state, gameplay state, input frame,
  accumulator, runtime interaction state, or binding input.

## Tests Covered

- Unchanged paths for `NotLoaded`, `MissingQuery`, and `TargetNotFound`.
- Valid `TargetFound` projection.
- Invalid `TargetFound` variants.
- Reachable and out-of-range found targets both projecting.
- Selected target preservation.
- Hover preservation and replacement.
- Current player tile and gate preservation.
- Selected-over-hover binding priority.
- Query/base immutability.

## Boundaries Preserved

- Runtime/product projection helper only.
- No Qt changes.
- No `PrimaryTile` behavior change.
- No `PrimaryTile` to `Interact` conversion.
- No explicit `Interact`/`Inspect` target synthesis.
- No selected target persistence or durable hover state.
- No frame request, play surface, or input adapter behavior changes.
- No changes to `RuntimeGameplayProductInputContext::build(state)` behavior.
- No interaction command/effect execution changes.
- No render drawing, target highlight panel, sprites/assets, pause/retry/reset,
  completion/failure, save/load, package scanning/watching/discovery, or source
  mutation.
- No persistence of query results or derived context in runtime/session/gameplay/
  product-loop/play-mode/saves/settings/scene-UI truth.

## Next Framing

- This helper only projects an already-computed query result into transient
  hovered target context.
- Later gates may decide how or when Qt click, current pointer, selected target,
  explicit interact synthesis, event ordering, reach policy, click-to-interact,
  panel/render highlighting, or frame request/play-surface context wiring should
  consume it.
- Render command drawing, `PrimaryPoint` behavior, sprites/assets/materials,
  UX/save/completion/package/source gates remain separate.

## Verification

- `git diff --check`
- `git status --short --branch`
