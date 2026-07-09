# File Spec

File: `src/app/iggy3d/room_editor/RoomEditorPreviewState.hpp`

Verified at: `f27cc3fc`

## Owns

- App/window mirror packet for room-editor placement preview state.
- `ProductRoomEditorPreviewState`.
- Receipt-facing preview candidate, tool, grid, draw-count, triangle-count, and optimization delta fields.

## Does Not Own

- Placement preview generation.
- Preview confirmation/cancel action handling.
- Primitive draw-list rendering.
- Room-edit command application.

## Reads

- This header defines the packet only.
- Consumers read/write it through `CreativeAuthoringStore`, automation, projection refresh, transition cleanup, window input, and receipt fields.

## Writes / Mutates

- No functions in this header mutate state.
- Producers write the packet from preview automation, projection refresh, and transition cleanup paths.

## Calls Out To / Wires Out To

- Included by `CreativeAuthoringStore.hpp`.
- Mirrors `ProductRoomEditorPlacementPreviewResult` and `ProductRoomEditorPreviewOverlay` facts.
- Emitted to receipts by `SaveStateFields.cpp`.

## Called By / Entry Points

- `ProductRoomEditorPreviewState`.
- Focused proof: `rg -n "RoomEditorPreviewState|roomEditorPreview|room_editor_preview" cmake/iggy3d_tests.cmake tests/unit src/app/iggy3d --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Default state is inactive, hidden, and not requested.
- Candidate id defaults to `none`; tool defaults to `floor`.
- Preview count/delta fields are mirrors of preview computation, not renderer truth.
- Confirm/cancel paths must clear active/visible state and reset stale counts when appropriate.
- Field additions must identify producer and receipt consumer.

## Tests / Proof Commands

- `product_room_editor_preview_tests` covers preview generation.
- `product_room_editor_action_controller_tests` and `product_window_input_frame_tests` cover preview build/confirm/cancel routing.
- `product_menu_transitions_tests` covers transition cleanup defaults.
- `rg -n "product_room_editor_preview_tests|product_room_editor_action_controller_tests|product_window_input_frame_tests|product_menu_transitions_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/Preview.*` unless preview result facts change.
- `src/app/iggy3d/automation/AutomationRoomEditing.*` unless preview action recording changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless mirror writing changes.
- `src/app/iggy3d/receipt/SaveStateFields.cpp` unless receipt keys change.

## Update When

- Preview packet fields, defaults, reset policy, producers, or receipt keys change.

## Do Not Update When

- Only preview primitive rendering or room-edit command semantics change without changing this mirror packet.
