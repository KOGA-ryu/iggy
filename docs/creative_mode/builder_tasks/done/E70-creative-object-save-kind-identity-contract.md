# E70: Creative Object Save Kind Identity Contract

## Objective

Make the persisted `CreativeObjectKind` identity an explicit compatibility
contract instead of an implicit match between `toString(kind)`, descriptor
`name`, and current tests.

## Problem

Creative save files write object kind as a string key:

- `SaveCodec.cpp` writes `creativeDocument.object.N.kind`.
- `DocumentSection.cpp` fills that field with `creative::toString(object.kind)`.
- Restore parses the string by scanning `allObjectDescriptors()` and matching
  `descriptor.name`.
- Descriptor tests assert `descriptor.name == toString(descriptor.kind)`, but
  no separate serialized id or alias/compatibility seam exists.

That works today, but it makes descriptor/name cleanup risky: renaming a
descriptor `name` or changing `toString(CreativeObjectKind)` can break old saves
with `invalid_object_kind`. Current tests mostly prove today's strings
round-trip, not that the serialized kind identity is intentionally stable.

## Required Reads

- `src/app/iggy3d/creative/document/Object.hpp`
- `src/app/iggy3d/creative/document/Object.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/creative/world/DocumentSection.cpp`
- `src/runtime/save/SaveCodec.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `tests/unit/creative_document_save_section_tests.cpp`
- `tests/unit/save_creative_document_section_tests.cpp`

## Scope

- Add a named serialized-kind seam for Creative objects, for example
  `serializedKindId` on `CreativeObjectDescriptor` or a dedicated
  `serializedObjectKindId(CreativeObjectKind)` helper.
- Keep the existing save key name `creativeDocument.object.N.kind`.
- Keep currently emitted string values compatible unless there is a deliberate
  migration decision.
- Route save conversion and restore parsing through the new explicit seam.
- If aliases are added, keep them local to the parse seam and test at least one
  legacy alias path.

## Acceptance

- The save/restore path no longer relies on descriptor display/name cleanup
  accidentally preserving old save compatibility.
- Tests distinguish descriptor display labels from persisted kind identity.
- Every non-Unknown `CreativeObjectKind` has exactly one serialized id and no two
  kinds share one id.
- Existing saves using current ids still restore.
- Invalid kind strings still reject with the existing `invalid_object_kind`
  reason.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_save_section_tests save_creative_document_section_tests product_save_bridge_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_save_section_tests|save_creative_document_section_tests|product_save_bridge_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change the save key name or broad save schema for this slice.
- Do not make unknown kind strings lenient.
- Do not use display names as save ids.
- Do not fold this into the object-field codec extraction; this is the kind
  identity compatibility contract, not the field-list extraction.

## Completion Brief

- Status: done.
- Files modified:
  - `src/app/iggy3d/creative/document/Object.hpp`
  - `src/app/iggy3d/creative/document/Object.cpp`
  - `src/app/iggy3d/creative/world/DocumentSection.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
  - `tests/unit/creative_document_save_section_tests.cpp`
- Implementation:
  - Added the explicit serialized-kind seam:
    - `serializedObjectKindId(CreativeObjectKind)`
    - `parseSerializedObjectKindId(std::string_view, CreativeObjectKind&)`
  - Backed the seam with a dedicated stable ID table rather than descriptor
    display/name parsing.
  - Kept existing emitted save values unchanged, including `PatrolRoute`,
    `MovingPlatform`, etc.
  - Routed creative save-section build through `serializedObjectKindId(...)`.
  - Routed creative save-section restore through `parseSerializedObjectKindId(...)`.
  - Invalid kind strings still reject with `invalid_object_kind`.
- Tests:
  - Descriptor tests now assert every authored kind has one non-empty unique
    serialized id, current ids preserve legacy strings, ids parse back to their
    kind, and display labels such as `Moving Platform` are not accepted as save
    kind ids.
  - Save-section tests now compare against the serialized id helper and reject a
    display-label kind with the existing invalid-kind status/reason.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_save_section_tests save_creative_document_section_tests product_save_bridge_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_save_section_tests|save_creative_document_section_tests|product_save_bridge_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Result: all passed.
