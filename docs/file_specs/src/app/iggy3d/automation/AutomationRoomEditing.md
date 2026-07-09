# File Spec

Files: `src/app/iggy3d/automation/AutomationRoomEditing.hpp`, `src/app/iggy3d/automation/AutomationRoomEditing.cpp`

Verified at: `49d04797`

## Owns

- Room-editing automation handler for starting authoring from ASCII draft or active room, direct floor/wall edit commands, delete, undo, redo, cursor movement, tool changes, wall direction, mouse pick, placement preview, preview confirm/cancel, place, and `editor.input` sequences.
- Mirroring room editing, room editor cursor, preview, overlay/HUD reset, active room revision, and interaction-mode proof into `ProductAppWindowState`.
- Automation wrappers around room authoring and room editor controller functions.
- Room editor preview input state machine for build, confirm, place, and cancel actions.

## Does Not Own

- Room authoring document semantics.
- Room editor cursor/action controller algorithms.
- Window input polling or live mouse coordinate capture.
- Rendering of room editor HUD, overlay, cursor, preview, or props.
- Save/load persistence of edited rooms.

## Reads

- `FrontendState`, `ProductAppWindowState.creativeAuthoring`, active session pointer, active room state, viewport camera framing, and automation command value.
- Room editor cursor/action/preview controller results and room authoring operation results.
- Parsed automation values from `Automation.*`.

## Writes / Mutates

- Mutates `window.creativeAuthoring.roomEditing`, room editor cursor/status/proof fields, preview proof fields, overlay/HUD visibility reset fields, active room state, active room revision, viewport render-bridge proof fields, interaction mode, input last-action proof, and automation-control status.
- May mutate room editing state by applying edit commands, undo, redo, cursor place, preview confirm, or editor input.

## Calls Out To / Wires Out To

- `startProductRoomAuthoringFromAsciiDraft(...)` and `startProductRoomAuthoringFromActiveRoom(...)`.
- `applyProductRoomAuthoringEditCommand(...)`, `undoProductRoomAuthoringEdit(...)`, and `redoProductRoomAuthoringEdit(...)`.
- `moveProductRoomEditorCursor(...)`, `setProductRoomEditorTool(...)`, `cycleProductRoomEditorTool(...)`, `setProductRoomEditorWallDirection(...)`.
- `applyProductRoomAuthoringCursorPlace(...)`, `applyProductRoomEditorActions(...)`, `applyProductRoomEditorMousePick(...)`, and `buildProductRoomEditorPlacementPreview(...)`.
- `ensureActiveRoomCollisionFresh(...)`, `activeRoom(...)`, and `bumpActiveRoomRevision(...)`.
- Caller-provided `routeEditorInput(...)`.

## Called By / Entry Points

- `AutomationDispatch.cpp` calls `applyProductRoomEditingAutomationCommand(...)`.
- Window/input paths call helper functions such as `applyProductRoomEditorPreviewInputAction(...)` and room editing receipt helpers.
- Focused proof: `rg -n "applyProductRoomEditingAutomationCommand|applyProductRoomEditorPreviewInputAction|recordProductRoomEditingStart|room_edit.start_active|room_editor.preview_confirm" src/app tests`.

## Invariants

- Room editor command execution must fail as not-ready when room editing state is not ready.
- `editor.input` requires gameplay screen, active gameplay, active session availability, ready room editing, and editor input owner.
- Preview confirm requires an active, ok, command-ready preview matching current cursor state.
- Starting room edit from a valid source enters creative interaction mode and resets cursor/proof fields.
- Leaving room edit clears editor visibility/proof fields and returns interaction mode to player.
- Active room collision freshness is bumped when ready editing state is copied into active room state.

## Tests / Proof Commands

- `rg -n "product_room_editing_automation_smoke|product_room_editor_action_controller_tests|product_room_editor_preview_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "room_edit.start_active|room_editor.preview|room_editor.preview_confirm|room_editor.mouse_pick|editor.input" tests/smoke tests/unit`.
- `rg -n "room_editor_preview_confirmed|room_editor_preview_cancelled|room_editor_not_ready" tests/unit/product_room_editor_action_controller_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/*` unless controller or preview semantics change.
- `src/content/authoring/EditableRoomDocument.*` unless edit command persistence semantics change.
- `src/app/iggy3d/gameplay/ActiveRoom*.{hpp,cpp}` unless active room revision/collision refresh ownership changes.
- `src/app/iggy3d/window/InputFrame.*` unless live editor input routing changes.

## Update When

- Room editing automation commands, readiness gates, editor input routing, preview behavior, proof mirroring, active room refresh, or interaction-mode transitions change.

## Do Not Update When

- Only room editor rendering, unrelated gameplay movement, or persistence internals change behind the same room editing automation contract.
