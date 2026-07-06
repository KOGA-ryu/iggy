# E50: Future-Storage Mutation Policy

## Objective

Stop “allowed but no stored field yet” mutations from looking like usable edit
verbs without an explicit policy signal.

## Problem

Several descriptor-allowed mutations route to
`makeFutureStorageNoChangeReceipt(...)`, returning NoChange with message
`mutation has no stored object field yet`.

Tests currently pin this as green behavior for examples like Note text and
NavLink target payloads. That is honest as a compatibility placeholder, but it
also means an edit verb can be advertised as allowed while doing nothing
persistently. That is a test-quality risk: a command can be wired and tests can
pass while no authored state changes.

## Required Reads

- `src/app/iggy3d/creative/mutation/Mutation.hpp`
- `src/app/iggy3d/creative/mutation/Mutation.cpp`
- `src/app/iggy3d/creative/mutation/MutationApply.cpp`
- `src/app/iggy3d/creative/document/DocumentMutation.cpp`
- `tests/unit/creative_document_mutation_tests.cpp`

## Scope

- Add an explicit way to distinguish real stored mutations from future-storage
  placeholder mutations.
- Keep current compatibility behavior unless the focused design decides a
  placeholder should now reject instead of NoChange.
- Ensure tests assert the policy signal, not merely the NoChange message.
- Make future command/UI exposure check the policy before treating a mutation as
  user-editable, or leave a hard failing test if that surface is not present yet.

## Acceptance

- A reviewer can tell from one helper/field whether a mutation has durable
  stored-object effects today.
- Existing sleeper/future-storage tests are updated to prove the placeholder
  policy explicitly.
- Real stored mutations still advance revision/dirty exactly as before.
- Path-point `SetPatrolRoute` remains classified as stored when using
  `PathPointsMutation`; legacy text/string payloads remain compatibility
  placeholders if kept.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_mutation_tests creative_document_path_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_mutation_tests|creative_document_path_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not fake persistence by storing placeholder text in tags.
- Do not silently remove legacy payload acceptance without tests and a reasoned
  compatibility decision.
- Do not expose future-storage verbs as UI commands.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/mutation/Mutation.hpp`
  - `src/app/iggy3d/creative/mutation/Mutation.cpp`
  - `tests/unit/creative_document_mutation_tests.cpp`
  - `tests/unit/creative_document_path_tests.cpp`
- Added `CreativeMutationStoragePolicy`:
  - `Unknown`
  - `StoredObject`
  - `FutureStoragePlaceholder`
  - `PayloadDependent`
- Added public policy helpers:
  - `toString(CreativeMutationStoragePolicy)`
  - `mutationStoragePolicy(...)`
  - `mutationPayloadStoragePolicy(...)`
  - `mutationHasStoredObjectEffect(...)`
  - `mutationPayloadHasStoredObjectEffect(...)`
- Added `storagePolicy` to `CreativeMutationDescriptor`.
- Classified current stored mutations, future-storage placeholder mutations, and
  payload-dependent `SetPatrolRoute` in the local mutation metadata registry.
- Preserved compatibility behavior:
  - future-storage payloads still return `NoChange`
  - legacy text/string `SetPatrolRoute` payloads still stay future-storage
  - path-point `SetPatrolRoute` remains stored/durable
- Updated sleeper/future-storage tests to assert policy signals directly instead
  of only pinning the no-storage message.

## Verification

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_mutation_tests creative_document_path_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_mutation_tests|creative_document_path_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

All verification passed.
