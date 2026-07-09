# File Spec

Files: `src/app/iggy3d/room_editor/Cursor.hpp`, `src/app/iggy3d/room_editor/Cursor.cpp`

Verified at: `ac496769`

## Owns

- Room-editor cursor state, tool selection, wall direction, cursor movement, and mouse screen-to-grid picking.
- Conversion from cursor/tool state into `RoomEditCommand` placement commands.
- `ProductRoomEditorCursorState`, cursor result packets, and mouse-pick request/result packets.

## Does Not Own

- Applying edit commands to a document.
- Room-editor action orchestration, automation recording, preview rendering, or HUD presentation.
- CreativeDocument tool state.
- Viewport camera implementation beyond using supplied framing config.

## Reads

- Cursor grid/story/tool/object-asset state.
- `EditableRoomDocument` id counters and default room semantics.
- `ProductViewportFrameConfig`, screen coordinates, and anchor world point for mouse picking.

## Writes / Mutates

- Returns updated cursor state and optional `RoomEditCommand`.
- Does not mutate the editable room document.
- Does not write window, automation, or presentation state directly.

## Calls Out To / Wires Out To

- Calls editable room command builders: `addFloorCommand`, `addWallCommand`, and `addObjectCommand`.
- Used by room-editor action controller, preview, presentation, automation, and window input tests.

## Called By / Entry Points

- `productRoomEditorToolName`.
- `productRoomEditorDirectionName`.
- `moveProductRoomEditorCursor`.
- `cycleProductRoomEditorTool`.
- `setProductRoomEditorTool`.
- `setProductRoomEditorWallDirection`.
- `rotateProductRoomEditorWallDirectionClockwise`.
- `buildProductRoomEditorPlaceCommand`.
- `pickProductRoomEditorCursorFromScreen`.
- Focused proof: `rg -n "ProductRoomEditorCursor|moveProductRoomEditorCursor|buildProductRoomEditorPlaceCommand|pickProductRoomEditorCursorFromScreen|room_editor_cursor" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Cursor movement is grid-based and returns `room_editor_cursor_moved`.
- Tool cycling order is Floor to Wall to Object to Floor.
- Placement requires a non-null document and positive finite cell size.
- Wall placement uses the selected wall direction to choose a cell edge.
- Mouse picking rejects not-ready, invalid cell size, invalid pixels-per-meter, non-finite screen/config, and non-finite anchor inputs.

## Tests / Proof Commands

- `product_room_editor_cursor_tests` covers movement, tool changes, wall direction rotation, command creation, invalid input, and mouse picking.
- Action/presentation consumers are covered by room-editor action, overlay, preview, HUD, automation, and window input tests.
- `rg -n "product_room_editor_cursor_tests|product_room_editor_action_controller_tests|product_room_editor_preview_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/ActionController.*` unless orchestration changes.
- `src/app/iggy3d/room_editor/Preview.*` unless preview command generation changes.
- `src/app/iggy3d/automation/AutomationRoomEditing.*` unless automation mappings change.
- `src/app/iggy3d/view/ViewportFraming.*` unless screen-to-grid camera math changes.

## Update When

- Cursor state fields, movement/tool/direction policy, placement command generation, mouse-pick math, or result statuses change.

## Do Not Update When

- Only command application, preview rendering, or HUD formatting changes.
