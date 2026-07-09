# File Spec

Files: `src/app/iggy3d/room_editor/Preview.hpp`, `src/app/iggy3d/room_editor/Preview.cpp`

Verified at: `9efd172d`

## Owns

- Room editor placement preview request/result packets.
- Dry-run preview of the current cursor placement command against an editable room document.
- Candidate command capture and before/after authored-room geometry optimization counts.
- Preview status/reason reporting for not-ready, missing document, rejected cursor command, and rejected dry-run edit.

## Does Not Own

- Final document mutation, undo/redo state, input routing, HUD rendering, or room mesh optimization implementation.
- Cursor movement or mouse picking.

## Reads

- `ProductRoomEditorCursorState`, `EditableRoomDocument`, cursor placement command builder output, room edit results, and geometry optimization reports.

## Writes / Mutates

- Copies the document into a local dry-run document.
- Returns `ProductRoomEditorPlacementPreviewResult`.
- Does not mutate the live editing document or app window state.

## Calls Out To / Wires Out To

- `buildProductRoomEditorPlaceCommand(...)` from `Cursor.*`.
- `applyRoomEditCommand(...)` from content authoring.
- `buildProductRoomGeometryOptimizationReport(...)` from app room optimization.
- `Presentation.*` consumes the result for overlay/HUD packets.

## Called By / Entry Points

- `buildProductRoomEditorPlacementPreview(...)`.
- App projection refresh and automation room-editing helpers.
- Grep proof: `rg -n "buildProductRoomEditorPlacementPreview" src tests/unit`.

## Invariants

- Preview must not commit edits to the live document.
- `candidateCommandReady` is true only when a command exists and dry-run apply succeeds.
- Count and optimization deltas are computed from before/after dry-run documents.
- Preview result status must expose the first failing gate.

## Tests / Proof Commands

- `rg -n "product_room_editor_preview_tests|product_room_editor_overlay_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductRoomEditorPlacementPreview|candidateCommandReady|optimizedDrawDelta" tests/unit/product_room_editor_preview_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/ActionController.*` unless final apply behavior changes.
- `src/app/iggy3d/room_editor/Cursor.*` unless candidate command construction changes.
- `src/app/iggy3d/room/GeometryOptimization.*` unless optimization report semantics change.

## Update When

- Placement preview fields, dry-run validation, candidate command status, or before/after metric contracts change.

## Do Not Update When

- Only final apply/undo/redo behavior or HUD string formatting changes without preview packet changes.
