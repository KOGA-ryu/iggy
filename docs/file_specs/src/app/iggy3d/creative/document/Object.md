# File Spec

Files: `src/app/iggy3d/creative/document/Object.hpp`, `src/app/iggy3d/creative/document/Object.cpp`

Verified at: `1ffca70b`

## Owns

- Creative object identity and raw object record types.
- `CreativeObjectKind` enum, ids, layer ids, transform/bounds/path-point structs, and `CreativeObject`.
- Serialized object-kind id table and parsing.
- Category convenience predicates that delegate to descriptor policy.
- `makeRoomObject(...)` helper for room container objects.

## Does Not Own

- Descriptor policy, palette visibility, runtime meaning, or dirty flags.
- Creative document storage, id allocation, parent graph validation, or revision policy.
- Object mutation execution.
- Persistence section encoding beyond serialized kind ids.
- UI, renderer, or physics behavior.

## Reads

- Static `CreativeObjectKind` list and serialized id table.
- Object descriptor category data through `describeObject(...)`.

## Writes / Mutates

- Constructs `CreativeObject` values for callers.
- Parses serialized kind ids into output references.
- Does not mutate any document-owned object collection.

## Calls Out To / Wires Out To

- Calls `describeObject(...)` and `objectUsesCategory(...)` for display/category helpers.
- Used by document, mutation, bake, projection, persistence, facade, and creative UI surfaces as the shared object packet definition.

## Called By / Entry Points

- `makeRoomObject(...)`.
- `toString(CreativeObjectKind)`.
- `serializedObjectKindId(...)`.
- `parseSerializedObjectKindId(...)`.
- `allCreativeObjectKinds()`.
- Grep proof: `rg -n "makeRoomObject|serializedObjectKindId|parseSerializedObjectKindId|allCreativeObjectKinds" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- `kInvalidObjectId` remains zero.
- `kDefaultLayerId` remains zero.
- Serialized ids must preserve save/load compatibility.
- `allCreativeObjectKinds()` must cover the enum range used by `CreativeObjectKind::Count`.
- `Unknown` is not accepted by serialized kind parser.
- Category helpers must reflect descriptor truth, not duplicate separate category tables.

## Tests / Proof Commands

- `creative_object_descriptor_tests`.
- `creative_document_save_section_tests`.
- `creative_document_create_tests`.
- `rg -n "creative_object_descriptor_tests|creative_document_save_section_tests|creative_document_create_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/document/ObjectDescriptor.*` unless kind policy changes with the enum.
- `src/app/iggy3d/creative/document/Document.*` unless document storage behavior changes.
- `src/app/iggy3d/creative/world/DocumentSection.*` unless save/load kind ids change.
- `src/app/iggy3d/creative/mutation/Mutation.*` unless object mutation verbs change.

## Update When

- Object record fields, object kind enum, serialized object ids, room object helper, or category delegation changes.

## Do Not Update When

- Only descriptor metadata, mutation behavior, document storage, or rendering policy changes without changing the object packet contract.
