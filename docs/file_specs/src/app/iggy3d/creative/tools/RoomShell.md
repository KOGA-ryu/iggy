# File Spec

Files: `src/app/iggy3d/creative/tools/RoomShell.hpp`, `src/app/iggy3d/creative/tools/RoomShell.cpp`

Verified at: `7b40370f`

## Owns

- Generated room-shell request and remove planning for creative room objects.
- Room-shell status enum, receipts, provenance tags, and child lookup helpers.
- Construction of one floor and four wall create requests from a selected room bounds.
- Discovery of generated shell children for removal.

## Does Not Own

- Applying create/remove requests to the document.
- UI command dispatch or receipt copying.
- Room bake/render behavior after shell creation.
- Creative document object schema.
- Generic object delete policy.

## Reads

- `CreativeDocument` and selected room object id.
- Room object kind, bounds, visibility, parent id, and tags.
- Floor descriptor default height through `describeObject(CreativeObjectKind::Floor)`.

## Writes / Mutates

- Builds vectors of `CreativeDocumentCreateRequest` or `CreativeObjectId`.
- Writes build/remove receipts.
- Does not mutate the document directly.

## Calls Out To / Wires Out To

- Calls `describeObject(...)` for floor height.
- Consumed by `ProductCreativeUiCommandFrame` generate/remove room-shell handlers.
- Provenance helpers are used by facade summaries, bake tests, and product command tests.

## Called By / Entry Points

- `buildCreativeRoomShellCreateRequests(...)`.
- `findCreativeRoomShellChildren(...)`.
- `creativeRoomShellCreateRequestHasProvenance(...)`, `creativeRoomShellObjectHasProvenance(...)`, `collectCreativeRoomShellChildIds(...)`.
- `creativeRoomHasGeneratedShellChildren(...)`.
- Grep proof: `rg -n "CreativeRoomShell|buildCreativeRoomShellCreateRequests|findCreativeRoomShellChildren|generatedRoomShellTag" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Missing document, missing selection, non-room selection, invalid bounds, and duplicate shell return explicit non-accepted statuses.
- Generated shell requests carry parent id plus generated/source tags.
- Generated shell is exactly one floor and four walls.
- Removal finds only generated shell children with matching parent and tags.
- This planner must not apply mutations directly.

## Tests / Proof Commands

- `creative_room_shell_tests`.
- `product_creative_ui_command_frame_tests`.
- `creative_document_room_bake_tests`.
- `product_creative_world_launch_tests`.
- `rg -n "creative_room_shell_tests|creative_document_room_bake_tests|GenerateSelectedRoomShell|RemoveSelectedRoomShell" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless command application policy changes.
- `src/app/iggy3d/creative/document/DocumentMutation.*` unless create/remove request schema changes.
- `src/app/iggy3d/creative/adapters/RoomBake.*` unless generated shell bake semantics change.
- `src/app/iggy3d/creative/ui/UiFrame.*` unless inspector row availability changes.

## Update When

- Shell generation geometry, provenance tags, duplicate detection, remove discovery, receipt/status semantics, or selected-room requirements change.

## Do Not Update When

- Only command UI labels, bake implementation, or document mutation application changes without changing shell planning.
