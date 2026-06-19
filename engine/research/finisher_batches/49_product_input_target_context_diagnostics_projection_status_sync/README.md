# 49 Product Input Target Context Diagnostics Projection Status Sync

Status: complete.

## Goal

Sync planning and API docs after compact read-only product input target-context
diagnostics projection landed.

## Integrated Surface

- Qt stores the latest transient
  `RuntimeGameplayProductInputFrameTargetContextResult` as replaceable app-shell
  state beside existing product transient input, latest-frame, and camera state.
- During product play setup, Qt initializes the diagnostics pointer to null.
- In `runProductFrameRequestOnce()`, after enrichment, Qt stores the helper
  result, sets the has flag, wires
  `context_.latestProductInputFrameTargetContext` to the stable member, and
  passes the stored copied/enriched frame into the frame request.
- Product Input Focus disable clears accumulator state plus latest target-context
  diagnostics/pointer. Enabling focus does not synthesize diagnostics.
- Widget/body refreshes do not clear the member; the pointer remains stable
  through rebuilds.
- `UiFeatureContext` carries a const diagnostics pointer, but
  `uiFeatureContextHasProductPlayMode(...)` does not include it, so diagnostics
  alone do not create product play context.
- `UiRuntimeWorkspaceModel` passes diagnostics into the product panel only when
  product play context is already present.
- `UiProductPlayModePanelModel` adds compact `targetContext` rows and does not
  require latest-frame rows.

## Row Behavior

- No diagnostics pointer means no target-context rows.
- Rows are source-shaped and compact:
  - `targetContext.status`
  - `targetContext.hasPrimaryTileEvent`
  - `targetContext.primaryTileEventIndex` only when a primary tile event exists
  - `targetContext.primaryTile` only when a primary tile event exists
  - `targetContext.target.status`
  - `targetContext.target.hasTarget`
  - `targetContext.target.targetId` only when a target exists and id is non-empty
  - `targetContext.target.hasPlayer`
  - `targetContext.target.hasReach`
  - `targetContext.target.reachable` only when reach exists
  - `targetContext.projection.status`
- The projection does not dump copied frames, all events, full target payloads,
  or nested structs.

## Boundaries Preserved

- Read-only diagnostics projection only.
- No `PrimaryTile` to `Interact` conversion.
- No target-id event injection.
- No selected target or durable hover lifecycle.
- No reach gating or interaction execution changes.
- No render/canvas drawing or target highlighting.
- No persistence of raw input, accumulator state, mouse state, camera,
  presentation, render frames, Qt state, viewport geometry, projected pointer
  data, spatial/query/enriched diagnostics, or target derivations in runtime/
  session/gameplay/product-loop/play-mode/saves/settings.
- No Qt types outside Qt shell.
- Diagnostics pointer alone must not mean product play mode exists.
- No changes to `RuntimeGameplayProductInputFrameTargetContext` semantics.

## Next Framing

- This only exposes diagnostics.
- Future gates may choose click-to-interact behavior, reach-gated execution,
  selected target state, durable hover lifecycle, target highlighting, render
  drawing/canvas interpretation, `PrimaryPoint`, sprites/assets/materials, and
  UX/save/completion/package/source.

## Verification

- `git diff --check`
- `git status --short --branch`
