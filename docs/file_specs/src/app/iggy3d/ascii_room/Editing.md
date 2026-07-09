# File Spec

Files: `src/app/iggy3d/ascii_room/Editing.hpp`, `src/app/iggy3d/ascii_room/Editing.cpp`

Verified at: `93b82ea7`

## Owns

- Product ASCII room editing build pipeline from authoring request to editable-room authoring controller.
- Stage-by-stage result packet for source, grid, editable room, authoring controller snapshot, counts, diagnostics, and failure stage.
- Default room id and source name fallback for inline ASCII authoring.

## Does Not Own

- Low-level source parsing, grid building, authored-room conversion, editable-room edit commands, active room state, or product menu flow.
- Rendering or save-file persistence.

## Reads

- `ProductAsciiRoomAuthoringRequest`, source text, tile/wall/floor config, grid result, editable room conversion result, and room authoring controller snapshot.

## Writes / Mutates

- Returns `ProductAsciiRoomEditingResult` with controller and snapshot when all stages succeed.
- Does not mutate app window state, active room state, files, or runtime sessions.

## Calls Out To / Wires Out To

- `parseAsciiRoomSource(...)`.
- `buildAsciiRoomGrid(...)`.
- `buildEditableRoomFromAsciiRoom(...)`.
- `ProductRoomAuthoringController`.
- Consumed by room editing state, active room state tests, automation room editing, and product ASCII editing tests.

## Called By / Entry Points

- `buildProductAsciiRoomEditing(...)`.
- Grep proof: `rg -n "buildProductAsciiRoomEditing" src tests/unit`.

## Invariants

- Pipeline stops at the first failed stage and reports that stage.
- Count fields are copied from the last available valid intermediate result.
- A ready result requires source, grid, editable-room conversion, and authoring snapshot all to be valid.
- This surface prepares editable room authoring; it does not itself activate gameplay room state.

## Tests / Proof Commands

- `rg -n "product_ascii_room_editing_tests|product_room_editing_state_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductAsciiRoomEditing|failedStage|product_ascii_room_editing_ready" tests/unit/product_ascii_room_editing_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/AuthoringController.*` unless controller snapshot semantics change.
- `src/app/iggy3d/ascii_room/AsciiRoomToEditableRoom.*` unless editable-room conversion changes.
- `src/app/iggy3d/gameplay/ActiveRoomState.*` unless activation from editing snapshot changes.

## Update When

- ASCII editing pipeline stages, failure status, counts, diagnostics, controller wiring, or fallback id/source rules change.

## Do Not Update When

- Only active room loading, save/load, render presentation, or source parser internals change without this pipeline contract changing.
