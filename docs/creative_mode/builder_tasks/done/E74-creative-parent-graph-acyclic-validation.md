# E74: Creative Parent Graph Acyclic Validation

## Objective

Make CreativeDocument parent relationships reject cycles through the same
document-truth path used by create, restore, and relationship mutations.

## Problem

E28/E34 made parent relationships stricter for missing parents, unsupported
children, unsupported owners, and parent deletion. The remaining hole is graph
shape: restore validation checks only one object at a time, and relationship
mutations write `parentId` directly on the object.

That means a save/restore payload can encode a cycle such as Group A parented
to Group B and Group B parented to Group A, because both descriptors can have a
parent and can own children. Relationship mutations also need a shared guard
before they are exposed more broadly, otherwise `SetParent` / `AttachTo` can
make the same invalid graph through the mutation gateway.

## Required Reads

- `src/app/iggy3d/creative/document/Document.cpp`
- `src/app/iggy3d/creative/document/Document.hpp`
- `src/app/iggy3d/creative/document/DocumentMutation.cpp`
- `src/app/iggy3d/creative/mutation/MutationApply.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_document_persistence_state_tests.cpp`
- `tests/unit/creative_document_save_section_tests.cpp`
- `tests/unit/creative_document_mutation_tests.cpp`

## Scope

- Add a shared parent-graph validation helper for CreativeDocument object
  collections.
- Restore must reject direct and indirect parent cycles with a deterministic
  reason such as `parent_cycle`.
- Relationship mutations that set/attach a parent must reject:
  - missing parent id;
  - parent id equal to the object id;
  - parent descriptor that cannot own children;
  - any parent assignment that would create a cycle.
- Keep valid Room -> generated Floor/Wall shell relationships working.
- Keep parent delete rejection from E28 unchanged.

## Acceptance

- Restore rejects a two-object Group cycle.
- Restore rejects a longer parent chain that loops back to an ancestor.
- `SetParent` / `AttachTo` cannot create a cycle in an existing document.
- `SetParent` / `AttachTo` cannot target a missing parent or a parent whose
  descriptor cannot own children.
- No-change parent mutations still stay no-change when the existing parent
  already matches the requested valid parent.
- Existing generated Room shell parent tests still pass.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_persistence_state_tests creative_document_save_section_tests creative_document_mutation_tests creative_room_shell_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_persistence_state_tests|creative_document_save_section_tests|creative_document_mutation_tests|creative_room_shell_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not add cascade delete in this slice.
- Do not weaken missing-parent or parent-owner validation.
- Do not move relationship truth into UI or RoomShell code.
- Do not make Group/Prefab descriptors less capable just to avoid cycles.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/document/Document.hpp`
  - `src/app/iggy3d/creative/document/Document.cpp`
  - `src/app/iggy3d/creative/document/DocumentMutation.cpp`
  - `tests/unit/creative_document_persistence_state_tests.cpp`
  - `tests/unit/creative_document_save_section_tests.cpp`
  - `tests/unit/creative_document_mutation_tests.cpp`
- Added shared parent graph validation:
  - `validateCreativeObjectParentGraph(std::span<const CreativeObject>)`
  - Preserves existing parent payload reasons first: `parent_unsupported`,
    `invalid_parent`, `missing_parent`, `parent_owner_unsupported`
  - Adds deterministic cycle rejection: `parent_cycle`
- Restore behavior:
  - `CreativeDocument::restoreForLoad(...)` validates the full parent graph
    before installing restored objects.
  - Direct two-object Group cycles and longer cycles reject with
    `InvalidObject` / `parent_cycle`.
- Relationship mutation behavior:
  - `DocumentMutation` simulates `SetParent` and `AttachTo` assignments against
    a document object snapshot before `MutationApply` writes `parentId`.
  - Missing parent, self-parent, unsupported owner, and cycle assignments reject
    without revision changes.
  - Valid no-change parent assignment remains `NoChange`.
- Tests added/updated:
  - Restore direct/indirect parent cycle tests in
    `creative_document_persistence_state_tests`.
  - Save-section parent cycle restore test in
    `creative_document_save_section_tests`.
  - Relationship mutation missing/self/owner/cycle/no-change tests in
    `creative_document_mutation_tests`.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_persistence_state_tests creative_document_save_section_tests creative_document_mutation_tests creative_room_shell_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_persistence_state_tests|creative_document_save_section_tests|creative_document_mutation_tests|creative_room_shell_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing whitespace scan over touched files
- Result: all passed.
- Concerns:
  - The mutation guard deliberately leaves descriptor-unsupported child kinds
    to the existing object-mutation descriptor validation, so unsupported
    mutation receipts remain stable while parentable objects get graph-specific
    rejection reasons.
