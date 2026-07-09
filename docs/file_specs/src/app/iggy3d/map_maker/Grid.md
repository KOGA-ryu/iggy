# File Spec

Files: `src/app/iggy3d/map_maker/Grid.hpp`, `src/app/iggy3d/map_maker/Grid.cpp`

Verified at: `9efd172d`

## Owns

- Legacy map maker grid config, status, dot, and snapshot packets.
- Grid config validation, snapped grid-dot generation, major-dot marking, layer/dot counts, and grid pitch cycling.

## Does Not Own

- CreativeDocument editing, room editor authoring, map maker HUD/cube presentation, input routing, rendering, or runtime collision.
- Product active-surface ownership for legacy map maker.

## Reads

- `ProductMapMakerGridConfig`: enabled flag, pitch, major step, extents, plane, and anchor world position.

## Writes / Mutates

- Returns `ProductMapMakerGridSnapshot` and pitch values.
- Does not mutate app window state, creative document state, room editor state, or renderer buffers.

## Calls Out To / Wires Out To

- Consumed by `map_maker/Presentation.*` for overlay/HUD/cube presentation.
- Consumed by `ProjectionRefresh.*` for gameplay projection frame map maker fields.

## Called By / Entry Points

- `isValidProductMapMakerGridConfig(...)`.
- `productMapMakerGridStatusName(...)`.
- `buildProductMapMakerGridSnapshot(...)`.
- `nextProductMapMakerGridPitch(...)`.
- `previousProductMapMakerGridPitch(...)`.
- Grep proof: `rg -n "buildProductMapMakerGridSnapshot|nextProductMapMakerGridPitch|previousProductMapMakerGridPitch" src tests/unit`.

## Invariants

- Disabled config must not produce visible grid dots.
- Invalid finite/positive config must report invalid config instead of generating dots.
- Dot generation must be deterministic for a given config.
- Pitch cycling is limited to the configured pitch table.
- This remains legacy map maker support, not the new CreativeDocument ownership seam.

## Tests / Proof Commands

- `rg -n "product_map_maker_grid_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductMapMakerGridSnapshot|nextProductMapMakerGridPitch|previousProductMapMakerGridPitch" tests/unit/product_map_maker_grid_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/map_maker/Presentation.*` unless presentation packets change.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless frame wiring changes.
- `src/app/iggy3d/creative/*` unless legacy/new creative boundaries change.

## Update When

- Grid config fields, validation, dot generation, major-dot policy, status strings, or pitch cycling changes.

## Do Not Update When

- Only map maker HUD rendering, creative document UI, or room editor authoring changes.
