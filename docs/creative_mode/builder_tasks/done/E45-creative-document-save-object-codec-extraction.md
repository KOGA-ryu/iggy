# E45: CreativeDocument Save Object Codec Extraction

## Objective

Reduce the cost of adding new `CreativeObject` persisted fields by extracting
the repeated creative-object save/read field list into smaller codec helpers.

## Problem

CreativeDocument persistence currently repeats object-field knowledge across
multiple places:

- `SaveCodec.cpp` writes every `creativeDocument.object.N.*` field inline.
- `SaveCodec.cpp` reads the same field list inline.
- `DocumentSection.cpp` converts `CreativeObject` to/from
  `SaveCreativeDocumentObjectRecord`.

The current path works, but every new object payload field requires carefully
editing long inline loops and then writing tests that can accidentally prove only
that the duplicated plumbing matches itself.

## Required Reads

- `src/runtime/save/SaveEnvelope.hpp`
- `src/runtime/save/SaveCodec.cpp`
- `src/app/iggy3d/creative/world/DocumentSection.cpp`
- `src/app/iggy3d/creative/document/Object.hpp`
- `tests/unit/save_creative_document_section_tests.cpp`
- `tests/unit/creative_document_save_section_tests.cpp`
- `tests/unit/product_save_bridge_tests.cpp`

## Scope

- Extract private helpers in `SaveCodec.cpp` for one creative object record,
  e.g. write/read object scalar fields, tags, and optional path points under a
  shared `creativeDocument.object.N.` prefix.
- Keep the line format, schema version, key names, diagnostics, and strict
  ordered reads unchanged.
- If useful, add a tiny local helper in `DocumentSection.cpp` for path-point
  conversion, but do not widen into a serialization redesign.

## Acceptance

- Creative object write/read code is shorter and has one named object-record
  field seam.
- Existing save files still round-trip exactly under the current schema.
- Tests continue to prove:
  - old saves without path points restore;
  - path points round-trip in order;
  - malformed path count/position diagnostics still point at the same key;
  - parent id and tags survive the bridge.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d save_creative_document_section_tests creative_document_save_section_tests product_save_bridge_tests creative_document_persistence_state_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(save_creative_document_section_tests|creative_document_save_section_tests|product_save_bridge_tests|creative_document_persistence_state_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not bump schema version.
- Do not change key names or key order.
- Do not make unknown keys lenient.
- Do not add new persisted fields.

## Completion Brief

- Status: done.
- Files modified:
  - `src/runtime/save/SaveCodec.cpp`
- Implementation:
  - Extracted writer helpers for one creative document object record:
    `writeCreativeDocumentObject(...)`,
    `writeCreativeDocumentObjectTags(...)`, and
    `writeCreativeDocumentObjectPathPoints(...)`.
  - Extracted reader helper `readCreativeDocumentObject(...)`.
  - Kept schema version, key names, key order, optional path point behavior, and
    diagnostics unchanged.
  - Left `DocumentSection.cpp` unchanged because it already has focused
    conversion helpers.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d save_creative_document_section_tests creative_document_save_section_tests product_save_bridge_tests creative_document_persistence_state_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(save_creative_document_section_tests|creative_document_save_section_tests|product_save_bridge_tests|creative_document_persistence_state_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over `src/runtime/save/SaveCodec.cpp`
- Result: all passed.
