# File Spec

Files: `src/app/iggy3d/creative/mutation/MutationApply.hpp`, `src/app/iggy3d/creative/mutation/MutationApply.cpp`

Verified at: `3b59f65c`

## Owns

- Single-`CreativeObject` mutation execution.
- `CreativeMutationApplyStatus`, apply receipt, and apply options.
- Generic `applyMutation(...)` admission and dispatch to shared mutation helpers.
- Stored-object changes for identity, transform, bounds/shape, parent/layer/tag, and path/line endpoint mutations.
- Future-storage no-change receipts for mutation kinds without object fields yet.

## Does Not Own

- Document object lookup, revision increments, batch mutation, or parent graph validation.
- Mutation vocabulary metadata.
- Descriptor table rows.
- UI command routing, undo, save/load, renderer, or filesystem behavior.

## Reads

- Target `CreativeObject`, mutation kind, payload, and apply options.
- Descriptor mutation allowance and dirty flag policy.
- Payload variants selected by `Mutation.*`.

## Writes / Mutates

- Mutates one caller-owned `CreativeObject`.
- Writes `CreativeMutationApplyReceipt` with status, object id/kind, mutation kind, dirty flags, change flag, allowed flag, and message.
- Does not mutate document revision or collections.

## Calls Out To / Wires Out To

- Calls `descriptorAllowsMutation(...)`, `dirtyFlagsForMutation(...)`, payload-shape helpers, and object capability predicates.
- Called by `DocumentMutation.*` and by tests that exercise direct object mutation helpers.

## Called By / Entry Points

- `applyMutation(...)`.
- Direct helpers such as `applyRenameMutation(...)`, `applyMoveMutation(...)`, `applySetBoundsMutation(...)`, and `applyPathPointsMutation(...)`.
- Grep proof: `rg -n "applyMutation|applyRenameMutation|applyMoveMutation|applyPathPointsMutation|CreativeMutationApply" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Invalid object ids/kinds reject.
- Unknown mutation rejects.
- Locked objects reject when options require it, except `SetLocked`.
- Descriptor and payload validation run when options require them.
- No-change receipts remain successful unless options reject no-change.
- Move translates bounds along with transform when the object has bounds; bounds-only objects move by bounds minimum.
- Path mutations require valid path or line endpoint shapes before writing `pathPoints`.
- Future-storage mutations must not pretend to change stored object state.

## Tests / Proof Commands

- `creative_document_mutation_tests`.
- `creative_document_path_tests`.
- `creative_document_dirty_tests`.
- `creative_facade_mutation_tests`.
- `rg -n "creative_document_mutation_tests|creative_document_path_tests|creative_document_dirty_tests|creative_facade_mutation_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/mutation/Mutation.*` unless mutation vocabulary or payloads change.
- `src/app/iggy3d/creative/document/DocumentMutation.*` unless document-level flow changes.
- `src/app/iggy3d/creative/document/ObjectDescriptor.*` unless descriptor allowance or dirty flags change.
- `src/app/iggy3d/creative/document/Object.*` unless object storage fields change.

## Update When

- Mutation admission, execution helpers, no-change/reject behavior, stored fields, future-storage handling, or apply receipt semantics change.

## Do Not Update When

- Only document revision, UI command routing, or save/load behavior changes around unchanged object mutation execution.
