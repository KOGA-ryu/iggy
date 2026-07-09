# File Spec

Files: `src/app/iggy3d/room_editor/Presentation.hpp`, `src/app/iggy3d/room_editor/Presentation.cpp`

Verified at: `9efd172d`

## Owns

- Room editor overlay, preview overlay, and HUD presentation packets.
- Cursor-to-world overlay placement for floor/object cursor and wall-edge cursor.
- Preview result projection into visible overlay facts.
- HUD line construction for tool, grid/story, wall direction, last operation, and optimization deltas.

## Does Not Own

- Room edit command creation, document mutation, cursor movement rules, input routing, rendering, or authored-room persistence.
- Map maker grid/cube presentation.

## Reads

- `ProductRoomEditorCursorState`, `ProductRoomEditingState`, `ProductRoomEditorPlacementPreviewResult`, cursor tool/direction names, and preview optimization counts.

## Writes / Mutates

- Returns `ProductRoomEditorOverlay`, `ProductRoomEditorPreviewOverlay`, and `ProductRoomEditorHud`.
- Does not mutate editor state, app window state, runtime room data, or draw lists.

## Calls Out To / Wires Out To

- Uses cursor naming helpers from `Cursor.*`.
- Consumes preview result packets from `Preview.*`.
- Feeds `ProjectionRefresh.*` and `FramePresenter.*` through gameplay projection frame data.

## Called By / Entry Points

- `buildProductRoomEditorOverlay(...)`.
- `buildProductRoomEditorPreviewOverlay(...)`.
- `buildProductRoomEditorHud(...)`.
- Grep proof: `rg -n "buildProductRoomEditorOverlay|buildProductRoomEditorPreviewOverlay|buildProductRoomEditorHud" src tests/unit`.

## Invariants

- Presentation packets report not-ready/not-requested status instead of fabricating visible UI.
- Invalid cursor cell size or invalid wall direction must not produce visible overlay items.
- HUD line capacity is bounded by the fixed line array.
- Preview overlays mirror preview result facts; they do not re-run document edits.

## Tests / Proof Commands

- `rg -n "product_room_editor_overlay_tests|product_room_editor_hud_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductRoomEditorOverlay|buildProductRoomEditorPreviewOverlay|buildProductRoomEditorHud" tests/unit/product_room_editor_overlay_tests.cpp tests/unit/product_room_editor_hud_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/Preview.*` unless preview packet fields change.
- `src/app/iggy3d/window/FramePresenter.*` unless draw bridging changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless projection frame wiring changes.

## Update When

- Room editor overlay/HUD packet fields, visibility status, cursor-to-overlay projection, or HUD proof lines change.

## Do Not Update When

- Only edit command semantics, cursor input bindings, or render primitive drawing change without presentation-packet changes.
