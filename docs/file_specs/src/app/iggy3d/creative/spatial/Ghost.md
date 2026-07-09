# File Spec

Files: `src/app/iggy3d/creative/spatial/Ghost.hpp`, `src/app/iggy3d/creative/spatial/Ghost.cpp`

Verified at: `c5537031`

## Owns

- Creative placement/selection ghost preview state.
- `CreativeGhostState`, receipt packet, change kinds, and preview update/hide helpers.
- Conversion from tool pointer packets to raw and snapped ghost points.
- Ghost change/no-change receipts with before/after state mirrors.

## Does Not Own

- Tool command execution.
- Object/document mutation.
- Snap scalar math or snap settings storage.
- UI drawing, hit testing, or renderer presentation.
- Viewport projection/picking.

## Reads

- Current ghost state.
- `CreativeToolPointerPacket`, `CreativeToolIntent`, source tool, target ref, and creative snap settings.
- `snapPoint(...)` receipt from creative spatial snap.

## Writes / Mutates

- Mutates caller-owned `CreativeGhostState`.
- Increments ghost update count only when preview state changes.
- Writes receipts describing accepted, changed, hidden, unchanged, invalid snap, or non-preview intent outcomes.

## Calls Out To / Wires Out To

- Calls `snapPoint(...)` from creative spatial snap.
- Called by `creative::Facade` when dispatching tool intents and hiding ghost state.
- Read by creative UI model/projection for visible ghost facts.

## Called By / Entry Points

- `makeDefaultCreativeGhostState()`.
- `hideGhost(...)`.
- `updateGhostPreview(...)`.
- `applyGhostToolIntent(...)`.
- Grep proof: `rg -n "CreativeGhost|hideGhost|updateGhostPreview|applyGhostToolIntent" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Non-preview intents do not mutate ghost state.
- Invalid snap settings reject preview acceptance and do not write a visible preview.
- Repeating an identical preview returns unchanged and does not increment update count.
- Hiding an already hidden ghost is accepted but not changed.
- Receipts must preserve before/after mirrors for UI and tests.

## Tests / Proof Commands

- `creative_ghost_tests`.
- `creative_facade_tests`.
- `creative_ui_tests`.
- `rg -n "creative_ghost_tests|creative_facade_tests|creative_ui_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/spatial/Snap.*` unless snap receipt semantics change.
- `src/app/iggy3d/creative/tools/Tools.*` unless tool intent packets change.
- `src/app/iggy3d/creative/Facade.*` unless facade dispatch or ghost storage changes.
- `src/app/iggy3d/creative/ui/UiFrame.*` unless ghost UI projection changes.

## Update When

- Ghost state fields, preview/hide state transitions, receipt semantics, snap consumption, or tool-intent handling changes.

## Do Not Update When

- Only object creation, mutation execution, or UI rendering changes around unchanged ghost state behavior.
