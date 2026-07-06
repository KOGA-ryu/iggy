# E71: Descriptor Table Row Layout

## Objective

Make the `CreativeObjectDescriptor` table reviewable at row level, not just free
of trailing raw booleans.

## Problem

E40 replaced ambiguous raw boolean tuples with named capability flags, which was
the right first repair. But the descriptor table still stores all object facts
as one long call per row:

- all 108 descriptor rows are over 260 characters;
- the longest row is currently over 530 characters;
- a single descriptor row mixes kind/category/profile/shape/projection/occupancy,
  serialized name, display name, purpose, dirty flags, defaults, and capability
  flags on one physical line.

That shape still makes reviews brittle. A future feature can change projection,
occupancy, defaults, or palette/runtime semantics in the middle of a dense line,
and tests can still mostly prove only the edited row's current outcome.

## Required Reads

- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `tests/unit/creative_object_descriptor_tests.cpp`

## Scope

- Reformat or restructure descriptor rows so each row is inspectable.
- Prefer a named builder/record style or multi-line rows with grouped fields:
  taxonomy, projection/occupancy, labels, dirty/defaults, and capabilities.
- Preserve descriptor facts and order.
- Keep compile-time construction if practical.

## Acceptance

- A reviewer can inspect one descriptor row without horizontally scanning a
  300-500 character call.
- Descriptor tests still prove representative taxonomy, projection, capability,
  palette visibility, and dirty-flag facts.
- No descriptor semantics change in this slice.
- Follow-up descriptor cards such as E59/E67/E70 become easier to review.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_create_tests creative_document_room_bake_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_create_tests|creative_document_room_bake_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change object descriptor semantics.
- Do not add or remove object kinds.
- Do not combine this with save-kind identity, anchor semantics, or descriptor
  coverage work.

## Completion Brief

- Status: done.
- Files modified:
  - `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- Implementation:
  - Mechanically reformatted every `kDescriptors` table row from a single dense
    `descriptor(...)` line into a multi-line call with one top-level argument per
    line.
  - Preserved descriptor row order, helper calls, labels, defaults, dirty flags,
    capabilities, projection/occupancy facts, and runtime anchor semantics.
  - No object kinds were added or removed.
  - No descriptor semantics changed.
- Reviewability result:
  - Before this slice, all 108 descriptor rows were over 260 characters.
  - After this slice, `awk` reports `over260=0` for
    `ObjectDescriptor.cpp`.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_create_tests creative_document_room_bake_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_create_tests|creative_document_room_bake_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
  - descriptor row length scan over `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- Result: all passed.
