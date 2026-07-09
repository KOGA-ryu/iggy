# File Spec

File: `src/app/iggy3d/room_editor/RoomEditorOverlayState.hpp`

Verified at: `f27cc3fc`

## Owns

- App/window mirror packet for room-editor cursor overlay visibility and status.
- `ProductRoomEditorOverlayState`.
- Receipt-facing item count and world-space cursor position fields.

## Does Not Own

- Overlay geometry construction.
- Primitive draw-list rendering.
- Cursor movement or command generation.
- Room-editor automation or transition cleanup behavior.

## Reads

- This header defines the packet only.
- Consumers read/write it through `CreativeAuthoringStore`, projection refresh, transitions, automation, and receipt fields.

## Writes / Mutates

- No functions in this header mutate state.
- Producers write the packet in projection refresh, transition cleanup, and automation state-copy paths.

## Calls Out To / Wires Out To

- Included by `CreativeAuthoringStore.hpp`.
- Mirrored from `ProductRoomEditorOverlay` in `ProjectionRefresh.cpp`.
- Emitted to receipts by `SaveStateFields.cpp`.

## Called By / Entry Points

- `ProductRoomEditorOverlayState`.
- Focused proof: `rg -n "RoomEditorOverlayState|roomEditorOverlay|room_editor_overlay" cmake/iggy3d_tests.cmake tests/unit src/app/iggy3d --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Default state is hidden and not ready.
- `status` and `reasonCode` default to the same stable string.
- This packet is observability mirror state; overlay source truth remains in presentation/projection builders.
- Field additions must identify producer and receipt consumer.

## Tests / Proof Commands

- `product_room_editor_overlay_tests` covers overlay builder facts.
- `product_menu_transitions_tests`, `product_interaction_mode_state_tests`, and `product_vulkan_room_frame_tests` cover reset/projection consumers.
- `rg -n "product_room_editor_overlay_tests|product_menu_transitions_tests|product_interaction_mode_state_tests|product_vulkan_room_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/room_editor/Presentation.*` unless overlay source facts change.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless mirror writing changes.
- `src/app/iggy3d/receipt/SaveStateFields.cpp` unless receipt keys change.
- `src/app/iggy3d/menu/Transitions.*` unless cleanup defaults change.

## Update When

- Overlay packet fields, defaults, producers, reset policy, or receipt keys change.

## Do Not Update When

- Only overlay visual geometry or primitive draw rendering changes without changing this mirror packet.
