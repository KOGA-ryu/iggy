# File Spec

Files: `src/app/iggy3d/map_maker/Presentation.hpp`, `src/app/iggy3d/map_maker/Presentation.cpp`

Verified at: `9efd172d`

## Owns

- Legacy map maker grid overlay, cube preview, and HUD presentation packets.
- Projection of grid snapshot dots into overlay rows.
- Camera-facing unit cube preview position snapped to grid pitch.
- One-line HUD summary of grid pitch, major step, layers, and cube visibility.

## Does Not Own

- Grid dot generation, map maker active-surface gating, CreativeDocument tools, room editor placement, render primitive drawing, or input bindings.

## Reads

- `ProductMapMakerGridSnapshot`, map maker live flag, anchor world position, camera yaw, and cube/grid display values.

## Writes / Mutates

- Returns `ProductMapMakerGridOverlay`, `ProductMapMakerCubePreview`, and `ProductMapMakerHud`.
- Does not mutate app window state, projection frame state, creative document state, or renderer buffers.

## Calls Out To / Wires Out To

- Uses `productMapMakerGridStatusName(...)` from `Grid.*`.
- Consumed by `ProjectionRefresh.*` and then `FramePresenter.*` for UI/draw bridging.

## Called By / Entry Points

- `buildProductMapMakerGridOverlay(...)`.
- `buildProductMapMakerCubePreview(...)`.
- `buildProductMapMakerHud(...)`.
- Grep proof: `rg -n "buildProductMapMakerGridOverlay|buildProductMapMakerCubePreview|buildProductMapMakerHud" src tests/unit`.

## Invariants

- Hidden grid snapshots must not copy grid dots into the overlay.
- Cube preview is visible only when map maker is live and grid snapshot is ok.
- Cube center must be snapped to grid pitch and placed ahead of the camera anchor.
- This surface must remain separate from CreativeDocument UI and room editor presentation.

## Tests / Proof Commands

- `rg -n "product_map_maker_presentation_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductMapMakerGridOverlay|buildProductMapMakerCubePreview|buildProductMapMakerHud" tests/unit/product_map_maker_presentation_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/map_maker/Grid.*` unless grid packet/status fields change.
- `src/app/iggy3d/window/FramePresenter.*` unless draw bridging changes.
- `src/app/iggy3d/creative/*` unless legacy/new creative boundary changes.

## Update When

- Overlay/HUD/cube packet fields, cube positioning, grid-dot copy policy, or presentation status strings change.

## Do Not Update When

- Only grid generation internals, input routing, active-surface gating, or CreativeDocument behavior changes.
