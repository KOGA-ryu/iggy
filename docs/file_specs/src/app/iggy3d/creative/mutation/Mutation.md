# File Spec

Files: `src/app/iggy3d/creative/mutation/Mutation.hpp`, `src/app/iggy3d/creative/mutation/Mutation.cpp`

Verified at: `3b59f65c`

## Owns

- Creative-mode mutation vocabulary and metadata.
- `CreativeMutationKind`, categories, payload structs, request/rule/descriptor packets, and storage policy.
- Per-kind metadata for category, payload shape, geometry/relationship/runtime meaning, and stored/future/payload-dependent effects.
- Allowed mutation lists derived from object descriptors and profiles.
- Payload builders and payload-shape validation.

## Does Not Own

- Applying mutations to objects.
- Document storage, revision, parent graph validation, or undo.
- UI command routing, tool state, filesystem, save/load, or render behavior.
- Object descriptor rows.

## Reads

- `CreativeObjectKind` and object descriptor profile/category/capability policy.
- Static mutation metadata rows.
- Payload variant alternatives for validation and storage-policy decisions.

## Writes / Mutates

- Returns mutation descriptors, allowed mutation vectors, categories, predicates, storage policy, and payload packets.
- Does not mutate creative objects or documents.

## Calls Out To / Wires Out To

- Calls `describeObject(...)` to derive allowed mutations from object profile.
- Feeds `ObjectDescriptor.*`, `MutationApply.*`, `DocumentMutation.*`, facade commands, and UI command frames.

## Called By / Entry Points

- `describeMutation(...)`, `allowedMutations(...)`, `canMutate(...)`.
- `requiresPayload(...)`, `payloadMatchesMutation(...)`.
- `mutationStoragePolicy(...)`, `mutationPayloadStoragePolicy(...)`.
- Payload builders such as `makeMovePayload(...)`, `makeBoundsPayload(...)`, and `makePathPointsPayload(...)`.
- Grep proof: `rg -n "CreativeMutationKind|describeMutation|allowedMutations|canMutate|payloadMatchesMutation|mutationPayloadStoragePolicy|make.*Payload" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Mutation metadata is the verb source of truth; do not duplicate per-kind mutation tables in UI or document code.
- Payload validation must match each mutation kind’s declared payload family.
- Future-storage mutations may be valid commands without implying stored object changes.
- `SetPatrolRoute` is payload-dependent: path-points payload stores on objects, legacy text/string payloads remain future storage.
- Allowed mutations must be derived from descriptor profile/capabilities, not hard-coded per caller.

## Tests / Proof Commands

- `creative_document_mutation_tests`.
- `creative_object_descriptor_tests`.
- `creative_document_path_tests`.
- `rg -n "creative_document_mutation_tests|creative_object_descriptor_tests|creative_document_path_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/mutation/MutationApply.*` unless execution changes.
- `src/app/iggy3d/creative/document/ObjectDescriptor.*` unless descriptor policy changes.
- `src/app/iggy3d/creative/document/DocumentMutation.*` unless document-level mutation flow changes.
- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless command-to-mutation routing changes.

## Update When

- Mutation kinds, payloads, categories, storage policies, metadata, allowed-mutation derivation, or payload builders/validation change.

## Do Not Update When

- Only object execution, document revision, UI routing, or save behavior changes without changing the mutation vocabulary contract.
