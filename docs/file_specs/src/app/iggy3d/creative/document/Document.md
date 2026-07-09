# File Spec

Files: `src/app/iggy3d/creative/document/Document.hpp`, `src/app/iggy3d/creative/document/Document.cpp`

Verified at: `d40a476b`

## Owns

- Durable creative document content truth: document id, name, revision, units, grid settings, snap settings, world bounds, object vector/index, next object id, and dirty flags.
- Create/remove/restore receipts and validation statuses.
- Object parent graph validation and document restore validation.

## Does Not Own

- Editor tool state, selection, ghost, measurement, UI, rendering, filesystem paths, command routing, or product launch policy.
- Save envelope encoding/decoding.

## Reads

- Creative document create/remove/restore requests.
- Object descriptors, transforms, bounds, parent ids, path points, layer/visibility/lock overrides, and dirty flag inputs.

## Writes / Mutates

- Document identity/settings, revision, dirty flags, object storage, object index, and next object id.
- Drains dirty flags when explicitly requested by save service.

## Calls Out To / Wires Out To

- Object descriptor/default helpers, snap defaults, spatial validation helpers, and parent graph validation.
- No app/window/render/save dependencies.

## Called By / Entry Points

- `Facade.*`, creative world service, save document sections, patrol route waypoint extraction, document mutation helpers, and tests.
- Grep proof: `rg -n "CreativeDocument::create|restoreForLoad|createObject|removeDocumentObject|drainDirtyFlags|documentForPersistence" src tests cmake`.

## Invariants

- Invalid document id is `0`; assigned document ids must be valid.
- Create/remove/restore receipts report requested/accepted/changed and revision transitions.
- Locked objects cannot be removed through normal remove flow.
- Parent graph restore rejects cycles, duplicate ids, invalid objects, and invalid next-object id.
- Revision and dirty flags advance only through accepted content/settings mutations.

## Tests / Proof Commands

- `rg -n "creative_document_create_tests|creative_document_remove_tests|creative_document_dirty_tests|creative_document_identity_tests|creative_document_persistence_state_tests|creative_document_path_tests" cmake tests`.
- `rg -n "restoreForLoad|validateCreativeObjectParentGraph|drainDirtyFlags" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/Facade.*` unless editor-facing mutation wrappers change.
- `src/app/iggy3d/creative/document/DocumentMutation.*` unless object mutation helpers change.
- `src/app/iggy3d/save/*` unless persistence contract changes.

## Update When

- Document durable fields, validation, create/remove/restore behavior, revision/dirty semantics, or object storage/index rules change.

## Do Not Update When

- Only UI tools or product launch/save orchestration changes around the same document API.
