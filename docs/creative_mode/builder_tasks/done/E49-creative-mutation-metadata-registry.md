# E49: Creative Mutation Metadata Registry

## Objective

Make mutation metadata easier to extend by consolidating repeated
`CreativeMutationKind` facts into one local registry/table or equivalent
single-source helper.

## Problem

`Mutation.cpp` is one of the highest branch-density files in the Creative stack.
Adding a mutation currently requires checking several switch/branch surfaces:

- `toString(CreativeMutationKind)`
- category predicates and `categoryOf(...)`
- `mutationChangesGeometry(...)`
- `mutationChangesRelationships(...)`
- `mutationChangesRuntimeMeaning(...)`
- `requiresPayload(...)`
- `payloadMatchesMutation(...)`

The behavior is mostly straightforward, but it is easy to add a new mutation and
miss one metadata surface. That is exactly the kind of feature-add friction the
review is trying to remove.

## Required Reads

- `src/app/iggy3d/creative/mutation/Mutation.hpp`
- `src/app/iggy3d/creative/mutation/Mutation.cpp`
- `src/app/iggy3d/creative/mutation/MutationApply.hpp/.cpp`
- `tests/unit/creative_document_mutation_tests.cpp`
- `tests/unit/creative_document_path_tests.cpp`

## Scope

- Introduce a small metadata table/helper for mutation kind facts where it
  reduces duplicated switch logic.
- Preserve all public enum values, names, categories, payload matching, dirty
  flag behavior, and receipts.
- Add focused coverage that every non-Unknown mutation has a non-Unknown name
  and category/facts are internally consistent.
- Keep object-kind allow-list policy out of this slice unless it is a direct,
  mechanical consumer of the metadata helper.

## Acceptance

- Adding a simple new mutation should have one obvious metadata location instead
  of several independent switches.
- Existing mutation tests pass unchanged in behavior.
- Path-point mutation remains real stored state, and legacy text/string patrol
  payload compatibility remains as currently pinned.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_mutation_tests creative_document_path_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_mutation_tests|creative_document_path_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change mutation semantics.
- Do not remove future-storage compatibility behavior in this slice; E50 owns
  that policy.
- Do not widen into descriptor object-kind capability rows.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/mutation/Mutation.cpp`
  - `tests/unit/creative_document_mutation_tests.cpp`
- Added a local mutation metadata registry in `Mutation.cpp` that owns each
  `CreativeMutationKind` name, category, geometry/relationship/runtime flags,
  and payload family.
- Replaced duplicated mutation-kind switch surfaces for:
  - `toString(CreativeMutationKind)`
  - `categoryOf(...)`
  - category predicates
  - `mutationChangesGeometry(...)`
  - `mutationChangesRelationships(...)`
  - `mutationChangesRuntimeMeaning(...)`
  - `requiresPayload(...)`
  - `payloadMatchesMutation(...)`
- Preserved object-kind allow-list policy and mutation apply semantics.
- Preserved `SetPatrolRoute` stored `PathPointsMutation` support and legacy
  text/string-id future-storage payload compatibility.
- Added focused mutation tests proving every authored mutation has non-Unknown
  name/category, exactly one category predicate, descriptor/fact consistency,
  no-payload behavior, representative payload families, and PatrolRoute legacy
  payload compatibility.

## Verification

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_mutation_tests creative_document_path_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_mutation_tests|creative_document_path_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

All verification passed.
