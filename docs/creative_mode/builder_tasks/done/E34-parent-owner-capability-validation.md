# E34: Parent Owner Capability Validation

## Objective

Close the remaining parent relationship integrity hole: a child with
`canHaveParent=true` must not be creatable/restorable under a parent descriptor
that cannot own children.

## Problem

E28/E33 added strict child-side parent validation and missing-parent rejection,
but the parent side is still not validated. `CreativeDocument::createObject(...)`
checks only the child descriptor and parent existence, while
`restoreForLoad(...)` calls `validateRestoredParentPayload(...)`, which checks
child support, self-parent, and missing parent, but not parent ownership.

This can still encode invalid relationships such as a parent-capable Wall under
a non-owner object. That undermines Room shell provenance, save/load integrity,
and future grouped/cascade operations.

## Required Reads

- `src/app/iggy3d/creative/document/Document.cpp`
- `src/app/iggy3d/creative/document/Document.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_document_persistence_state_tests.cpp`
- `tests/unit/creative_document_save_section_tests.cpp`
- `tests/unit/creative_room_shell_tests.cpp`

## Scope

- Add parent-owner validation for create and restore.
- Keep missing-parent and child `canHaveParent` validation strict.
- Preserve valid Room -> generated Floor/Wall shell parent relationships.
- Add deterministic status/reason codes for unsupported parent owner payloads.

## Acceptance

- Creating a parent-capable child under a parent whose descriptor has
  `canOwnChildren=false` rejects without mutating the document.
- Restoring a document with such a relationship rejects and preserves the
  existing document.
- Room -> generated shell Floor/Wall remains valid.
- Save-section restore tests distinguish child unsupported, parent missing, and
  parent cannot own children.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_persistence_state_tests creative_document_save_section_tests creative_room_shell_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_persistence_state_tests|creative_document_save_section_tests|creative_room_shell_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not weaken dangling-parent restore rejection.
- Do not make Crate parent-capable just to satisfy tests.
- Do not add cascade delete in this slice.
- Do not touch RoomBake policy.

## Completion Brief

- Files changed:
  - `src/app/iggy3d/creative/document/Document.cpp`
  - `tests/unit/creative_document_create_tests.cpp`
  - `tests/unit/creative_document_persistence_state_tests.cpp`
  - `tests/unit/creative_document_save_section_tests.cpp`
- Parent-owner validation policy:
  - `CreativeDocument::createObject(...)` now rejects parented create requests
    when the child can have a parent and the parent id exists, but the parent
    descriptor has `canOwnChildren == false`.
  - `CreativeDocument::restoreForLoad(...)` now applies the same owner-side
    validation after duplicate/missing object indexing.
  - Existing `parent_unsupported`, `invalid_parent`, and `missing_parent`
    validation remains strict and distinct.
  - New deterministic reason code: `parent_owner_unsupported`.
- Tests added/updated:
  - `creative_document_create_tests` proves parent-capable `Wall` under
    non-owner `Crate` rejects without revision/object-count mutation.
  - `creative_document_persistence_state_tests` proves restore rejects the same
    non-owner relationship and preserves the existing document.
  - `creative_document_save_section_tests` now distinguishes:
    - child cannot have parent: `parent_unsupported`
    - parent id missing: `missing_parent`
    - parent exists but cannot own children: `parent_owner_unsupported`
  - Existing Room -> generated Floor/Wall shell parent tests remain valid.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_create_tests creative_document_persistence_state_tests creative_document_save_section_tests creative_room_shell_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_create_tests|creative_document_persistence_state_tests|creative_document_save_section_tests|creative_room_shell_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/src/app/iggy3d/creative/document/Document.cpp /Users/kogaryu/iggy3d/tests/unit/creative_document_create_tests.cpp /Users/kogaryu/iggy3d/tests/unit/creative_document_persistence_state_tests.cpp /Users/kogaryu/iggy3d/tests/unit/creative_document_save_section_tests.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E34-parent-owner-capability-validation.md`
- Concerns/deferred:
  - No cascade delete was added. Parent delete rejection remains the current
    lifecycle policy for objects that own children.
