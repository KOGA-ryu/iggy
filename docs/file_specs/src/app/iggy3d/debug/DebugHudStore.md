# File Spec

File: `src/app/iggy3d/debug/DebugHudStore.hpp`

Verified at: `3dab1aa4`

## Owns

- `DebugHudStore` aggregation of product debug HUD state mirrors.
- Window-state grouping for top-down map, collision overlay, NPC behavior HUD, physics HUD, and position HUD.

## Does Not Own

- HUD construction, projection source data, rendering, or input toggles.
- Runtime debug truth.
- Save/load persistence.

## Reads

- Included HUD state type definitions only.

## Writes / Mutates

- No direct mutation; callers mutate fields through `ProductAppWindowState`.

## Calls Out To / Wires Out To

- No functions.
- Provides a nested ownership bucket inside `ProductAppWindowState`.

## Called By / Entry Points

- `ProductAppWindowState.hpp` embeds `DebugHudStore`.
- Projection refresh copies HUD states into this store.
- Grep proof: `rg -n "DebugHudStore|debugHud\\." src/app/iggy3d tests/unit`.

## Invariants

- Store remains a passive state aggregate.
- New debug HUD mirrors should be grouped here only if they are app/window debug state, not runtime truth.

## Tests / Proof Commands

- `rg -n "product_god_struct_ownership_coverage_tests|DebugHudStore" tests/unit src/app/iggy3d`.

## Nearby Files Usually Not Touched

- Individual HUD builders unless their state packet changes.
- Runtime/projection files unless source facts change.

## Update When

- Debug HUD state buckets are added, removed, renamed, or moved.

## Do Not Update When

- HUD rendering/layout changes with the same store fields.
