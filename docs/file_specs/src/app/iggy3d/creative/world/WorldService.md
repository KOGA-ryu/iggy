# File Spec

Files: `src/app/iggy3d/creative/world/WorldService.hpp`, `src/app/iggy3d/creative/world/WorldService.cpp`

Verified at: `d40a476b`

## Owns

- Creative world create/open/save service API and result packets.
- Creative world/document ID minting for new creative worlds.
- Validation for save root, title, timestamp, template id, save id, and document id.
- Durable creative document save/load calls through the product save bridge.
- Dirty-flag drain after accepted creative saves.

## Does Not Own

- Product gameplay/session transition.
- Facade state reset or active creative identity recording.
- Creative UI, window input, or menu dispatch.
- Runtime save codec/file-store internals.

## Reads

- `CreativeWorldCreateRequest`, `CreativeWorldOpenRequest`, and `CreativeWorldSaveRequest`.
- Existing save catalog through product save bridge world-id/document-id helpers.
- Creative document state for save and restore.

## Writes / Mutates

- Writes creative document saves through `writeCreativeDocumentSaveDurably(...)`.
- Drains dirty flags on the document after an accepted save.
- Returns loaded/created `CreativeDocument` values in result packets.

## Calls Out To / Wires Out To

- `nextProductWorldId(...)`, creative document ID minting, `writeCreativeDocumentSaveDurably(...)`, `loadCreativeDocumentSave(...)`, and runtime save-file path validation helpers.

## Called By / Entry Points

- `CreativeWorldOperations.cpp` calls create/open/save for product launch and save.
- Creative world service tests call the API directly.
- Grep proof: `rg -n "createCreativeWorld|openCreativeWorld|saveCreativeWorld|writeCreativeDocumentSaveDurably|loadCreativeDocumentSave" src tests cmake`.

## Invariants

- Only the `empty` template is accepted by this service.
- Create writes the initial save before accepting.
- Open requires a valid save id and a creative document save.
- Save rejects missing document, invalid save root/id, and invalid document id.
- Dirty flags are drained only after durable save success.

## Tests / Proof Commands

- `rg -n "creative_world_service_tests|product_save_bridge_tests|save_creative_document_section_tests" cmake tests`.
- `rg -n "creative_world_created|creative_world_opened|creative_world_saved" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/SaveBridge.*` unless durable creative save contracts change.
- `src/app/iggy3d/creative/document/Document.*` unless document persistence state changes.
- `src/app/iggy3d/creative/CreativeWorldOperations.*` unless product transition behavior changes.

## Update When

- Creative world validation, ID minting, durable save/load calls, result packets, or dirty-flag save semantics change.

## Do Not Update When

- Only product menu routing or UI around create/open/save changes.
