# File Spec

Files: `src/app/iggy3d/creative/spatial/Snap.hpp`, `src/app/iggy3d/creative/spatial/Snap.cpp`

Verified at: `3b59f65c`

## Owns

- Creative tool-space 2D snap wrapper.
- `CreativeSnapMode`, axis masks, 2D snap point/settings/receipt packets.
- Validation and receipt status for disabled, no-axis, invalid, changed, and already-snapped cases.
- `snapPointerPacket(...)` conversion for `CreativeToolPointerPacket`.

## Does Not Own

- Core scalar snap math.
- 3D document snap settings.
- Ghost placement, tool selection, UI display, or facade state storage.
- Save/load of snap settings.

## Reads

- Creative 2D snap settings and pointer packets.
- Core `snapScalarToGrid(...)` result.

## Writes / Mutates

- Writes `CreativeSnapReceipt`.
- Returns snapped pointer packet copies.
- Does not mutate facade/tool state directly.

## Calls Out To / Wires Out To

- Calls `iggy3d::snapScalarToGrid(...)`.
- Used by creative ghost placement, facade snap settings, and creative UI/status rows.

## Called By / Entry Points

- `makeDefaultCreativeSnapSettings()`.
- `isValidSnapSettings(...)`.
- `snapScalar(...)`.
- `snapPoint(...)`.
- `snapPointerPacket(...)`.
- Grep proof: `rg -n "CreativeSnap|snapPoint|snapPointerPacket|snapScalar\\(" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Disabled mode is valid and accepted but does not snap.
- No-axis grid mode is valid and accepted but does not snap.
- Active axes require positive finite steps.
- Only active axes are modified.
- Receipt message must distinguish invalid settings, disabled snap, disabled axes, snapped, and already snapped.
- Pointer snapping returns a copy so callers decide whether to store it.

## Tests / Proof Commands

- `creative_snap_tests`.
- `creative_ghost_tests`.
- `creative_facade_tests`.
- `rg -n "creative_snap_tests|creative_ghost_tests|creative_facade_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/core/math/Snap.*` unless core rounding/cell/pivot snap behavior changes.
- `src/app/iggy3d/creative/spatial/Ghost.*` unless ghost consumption changes.
- `src/app/iggy3d/creative/Facade.*` unless snap settings storage changes.
- `src/app/iggy3d/creative/document/DocumentSnap.*` unless 3D document snap changes.

## Update When

- Creative 2D snap settings, validation, receipt semantics, pointer packet snapping, or core snap wiring changes.

## Do Not Update When

- Only ghost rendering, UI presentation, or document 3D snap settings change without altering this 2D snap wrapper.
