# E33: Parent Restore Save-Section Compatibility

## Objective

Make the new parent-relationship restore validation compatible with existing
creative save-section tests and intended authored parent semantics.

## Problem

After E28, `creative_document_save_section_tests` segfaults because its authored
restore setup is rejected. The test fixture restores a Crate with `parentId = 2`;
new restore validation rejects parent payloads when the child descriptor does
not support parents.

This may mean the fixture was invalid all along, or the descriptor parent
capability policy is now too strict for existing saved creative data. Either way,
restore compatibility needs an explicit decision and test update.

## Evidence

Command run on 2026-07-06:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target save_creative_document_section_tests creative_document_save_section_tests save_load_tests -j10 &&
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(save_creative_document_section_tests|creative_document_save_section_tests|save_load_tests)$' --output-on-failure
```

Result:

- `save_creative_document_section_tests`: passed
- `save_load_tests`: passed
- `creative_document_save_section_tests`: segfault after repeated `FAIL:
  authored document restore setup`

## Required Reads

- `src/app/iggy3d/creative/document/Document.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_document_save_section_tests.cpp`
- `tests/unit/creative_document_persistence_state_tests.cpp`
- `tests/unit/creative_room_shell_tests.cpp`

## Scope

- Decide whether saved/restored Crate/Floor/Wall/etc. may have parents.
- If parented authored children are valid, update descriptor capability and
  tests coherently.
- If the old fixture is invalid, update the fixture to use a parent-capable
  object and add an explicit rejection test for unsupported parent payloads.
- Prevent setup failures from cascading into segfaults in the test.

## Acceptance

- `creative_document_save_section_tests` no longer segfaults.
- Save-section restore tests clearly distinguish valid parent payloads from
  invalid ones.
- E28 missing-parent and parent-delete tests still pass.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_save_section_tests creative_document_persistence_state_tests creative_room_shell_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_save_section_tests|creative_document_persistence_state_tests|creative_room_shell_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not weaken restore validation back to accepting dangling parent ids.
- Do not hide setup failures by skipping assertions.
- Do not change RoomBake behavior.

## Completion Brief

- Files changed:
  - `tests/unit/creative_document_persistence_state_tests.cpp`
  - `tests/unit/creative_document_save_section_tests.cpp`
  - `tests/unit/product_save_bridge_tests.cpp`
- Decision:
  - The parented `Crate` save/restore fixtures were invalid under the current
    descriptor policy because `Crate.canHaveParent == false`.
  - Restore validation remains strict. It still rejects unsupported parent
    payloads with `parent_unsupported` and still rejects dangling parent ids.
  - Valid parent round-trip fixtures now use `Wall`, which is parent-capable and
    still exercises transform, bounds, visibility, lock, tags, parent id,
    exact-double save conversion, and restore state.
- Test coverage added/updated:
  - `creative_document_persistence_state_tests` now restores a parented Wall and
    explicitly rejects a parented Crate restore payload without mutating the
    existing document.
  - `creative_document_save_section_tests` now round-trips a parented Wall and
    explicitly rejects a save-section parented Crate payload with
    `parent_unsupported`.
  - `creative_document_save_section_tests` now guards setup-derived section
    mutation tests before indexing `section.objects[1]`, so future setup
    failures become assertion failures instead of segfaults.
  - `product_save_bridge_tests` now uses the same parent-capable Wall fixture;
    this focused test reproduced the same invalid parent fixture failure.
- Verification:
  - Reproduced pre-fix failures:
    - `creative_document_persistence_state_tests` failed.
    - `creative_document_save_section_tests` segfaulted after authored restore
      setup failure.
    - `product_save_bridge_tests` failed after creative document fixture restore
      failure.
  - Passed:
    - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_save_section_tests creative_document_persistence_state_tests creative_room_shell_tests save_creative_document_section_tests save_load_tests product_save_bridge_tests -j10`
    - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_save_section_tests|creative_document_persistence_state_tests|creative_room_shell_tests|save_creative_document_section_tests|save_load_tests|product_save_bridge_tests)$' --output-on-failure`
    - `git -C /Users/kogaryu/iggy3d diff --check`
    - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/tests/unit/creative_document_persistence_state_tests.cpp /Users/kogaryu/iggy3d/tests/unit/creative_document_save_section_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_save_bridge_tests.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E33-parent-restore-save-compatibility.md`
- Concerns/deferred:
  - Parent validation currently checks child capability and parent existence. It
    does not yet validate that the parent descriptor can own children; that is a
    possible future integrity slice if needed.
