# File Spec

Files: `src/app/iggy3d/creative/document/DocumentMutation.hpp`, `src/app/iggy3d/creative/document/DocumentMutation.cpp`

Verified at: `1ffca70b`

## Owns

- Document-level bridge from `CreativeMutationRequest` to `CreativeDocument` mutation.
- `CreativeDocumentMutationStatus`, options, single mutation receipts, and batch receipts.
- Target object lookup, document validity/request validation, relationship parent graph validation, mutation apply delegation, and revision increment policy.
- Convenience wrappers for rename, move, rotate, resize, bounds, visibility, lock, layer, add tag, and remove tag mutations.

## Does Not Own

- Single-object mutation payload application details.
- Creative document object storage internals.
- UI command routing, input handling, or undo stack ownership.
- Save/load serialization.
- Renderer, bake, or spatial projection refresh.

## Reads

- `CreativeDocument` validity, revision, objects, and object lookup.
- Mutation request kind, object id, and payload.
- Descriptor mutation allowance through the object mutation pipeline.
- Parent graph validity through document validation helpers.

## Writes / Mutates

- Mutates the target object through `applyMutation(...)`.
- Increments document revision through `markObjectMutationChanged(...)` when an accepted mutation changes object state and options allow revision increments.
- Writes single and batch mutation receipts with revision before/after, dirty flags, counts, and object receipts.

## Calls Out To / Wires Out To

- Calls `applyMutation(...)` from `MutationApply.*`.
- Calls `canMutate(...)`, mutation payload builders, and document parent graph validation.
- Called by facade mutation commands, creative tools, spatial projection tests, room shell/bake scenarios, and no-window creative flows.

## Called By / Entry Points

- `applyDocumentMutation(...)`.
- `applyDocumentMutations(...)`.
- Convenience wrappers such as `renameDocumentObject(...)` and `moveDocumentObject(...)`.
- Grep proof: `rg -n "applyDocumentMutation|applyDocumentMutations|renameDocumentObject|moveDocumentObject|DocumentMutation" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Invalid documents, invalid requests, and missing objects reject without revision changes.
- Relationship mutations must validate the proposed parent graph before mutating the live document.
- Object mutation remains delegated to the shared mutation pipeline.
- Revision changes happen only through the document revision bridge.
- Batch receipts must merge dirty flags and count applied/no-change/failed items consistently.
- `stopBatchOnFailure` must stop after recording the failed receipt.

## Tests / Proof Commands

- `creative_document_mutation_tests`.
- `creative_document_dirty_tests`.
- `creative_facade_mutation_tests`.
- `product_creative_ui_command_frame_tests`.
- `rg -n "creative_document_mutation_tests|creative_document_dirty_tests|creative_facade_mutation_tests|product_creative_ui_command_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/mutation/MutationApply.*` unless object mutation execution changes.
- `src/app/iggy3d/creative/document/Document.*` unless document storage/revision behavior changes.
- `src/app/iggy3d/creative/Facade.*` unless facade command behavior changes.
- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless UI command dispatch changes.

## Update When

- Document mutation statuses, receipts, revision policy, relationship validation, batch semantics, or convenience wrapper routing changes.

## Do Not Update When

- Only a specific mutation payload implementation changes inside the object mutation executor.
