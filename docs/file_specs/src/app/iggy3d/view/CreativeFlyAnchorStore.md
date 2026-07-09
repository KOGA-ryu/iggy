# File Spec

Files: `src/app/iggy3d/view/CreativeFlyAnchorStore.hpp`, `src/app/iggy3d/view/CreativeFlyAnchorStore.cpp`

Verified at: `9f9c8b29`

## Owns

- Creative fly camera anchor store, provenance enum, provenance names, availability/freshness predicates, world-epoch bump, and anchor seeding/integration helpers.
- Anchor freshness contract tying a stored anchor to `ProductViewportState.creativeWorldEpoch`.
- Fallback player-position seeding when an active session is available, or origin fallback when not.

## Does Not Own

- Creative fly input integration.
- Creative world launch/open behavior.
- Projection refresh orchestration.
- Runtime session ownership or player entity lifecycle.
- Receipt field emission.

## Reads

- `ProductAppWindowState.viewport.creativeWorldEpoch`.
- Active session player slot and player transform when ensuring an anchor from gameplay state.
- Existing `ProductCreativeFlyAnchorStore` provenance and epoch.

## Writes / Mutates

- Mutates `window.viewport.creativeWorldEpoch` in `bumpCreativeWorldEpoch(...)`.
- Mutates `window.viewport.creativeFlyAnchor` position, provenance, and seeded epoch when seeding or recording integrated fly movement.
- Does not mutate runtime session state.

## Calls Out To / Wires Out To

- `Session::state()`, player slot lookup, and world entity lookup for player-position seeding.
- `ProjectionRefresh.cpp`, `CreativeBlankStageSession.cpp`, menu transitions/actions, and input frame code consume or update creative fly state.

## Called By / Entry Points

- Creative blank-stage setup seeds from origin.
- Projection refresh and input frame paths ensure or record creative fly anchors.
- Tests call store predicates and seed/integration helpers directly.
- Focused proof: `rg -n "CreativeFlyAnchor|creativeFlyAnchor|creativeWorldEpoch|recordCreativeFlyAnchorIntegrated" src/app tests/unit`.

## Invariants

- Unseeded anchors are unavailable and never fresh.
- Freshness requires both non-unseeded provenance and matching world epoch.
- Every write stores the current viewport creative world epoch.
- `ensureFreshCreativeFlyAnchor(...)` returns the existing fresh anchor without rewriting it.
- Origin seeding uses the product blank-stage origin anchor.
- This store is viewport/app camera state, not saved creative document truth.

## Tests / Proof Commands

- `rg -n "product_creative_fly_tests|product_creative_world_launch_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "productCreativeFlyAnchorFreshForEpoch|seedCreativeFlyAnchorFromOrigin|recordCreativeFlyAnchorIntegrated" tests/unit/product_creative_fly_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless anchor consumption changes.
- `src/app/iggy3d/window/InputFrame.*` unless creative fly input integration changes.
- `src/app/iggy3d/creative/CreativeBlankStageSession.*` unless blank-stage seeding changes.
- `src/app/iggy3d/view/ViewportState.hpp` unless stored fields change.

## Update When

- Creative fly anchor provenance, epoch freshness, seed positions, session fallback, or anchor write semantics change.

## Do Not Update When

- Only creative document contents, UI controls, or receipt formatting changes without changing anchor storage behavior.
