# E59: CreativeObjectKind Descriptor Coverage Sentinel

## Objective

Make it impossible to add a `CreativeObjectKind` enum value without also adding
an explicit descriptor row and string mapping.

## Problem

`CreativeObjectKind` has no `Count`/sentinel value or generated inventory.
Descriptor tests currently pin `allObjectDescriptors().size() == 108`, but that
is only a snapshot. If a future object kind is appended to the enum and the test
count is not updated, `describeObject(kind)` can silently fall back to the
Unknown descriptor.

Evidence:

- `src/app/iggy3d/creative/document/Object.hpp:39` defines
  `enum class CreativeObjectKind` with no terminal count/sentinel.
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp:396` loops descriptors
  and returns `kDescriptors.front()` on no match.
- `tests/unit/creative_object_descriptor_tests.cpp:56` pins a hardcoded row
  count, but does not derive expected coverage from the enum.
- `src/app/iggy3d/creative/document/Object.cpp:32` has a separate `toString`
  switch that must also stay in sync with descriptors.

That means the central noun table can still drift when adding object kinds,
which is exactly the feature-add path this registry is supposed to simplify.

## Required Reads

- `src/app/iggy3d/creative/document/Object.hpp`
- `src/app/iggy3d/creative/document/Object.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `tests/unit/creative_room_tests.cpp`

## Scope

- Add a durable enum coverage mechanism, such as:
  - a terminal `Count`/`Last` enum value plus an iteration helper, or
  - a single authoritative array of all known `CreativeObjectKind` values.
- Update descriptor tests to prove every non-sentinel kind has exactly one
  descriptor row.
- Prove every descriptor row's `name` still matches `toString(kind)`.
- Keep `Unknown` behavior for invalid/unrecognized values if callers rely on it,
  but make missing in-range enum coverage fail tests.

## Acceptance

- Adding a new `CreativeObjectKind` without adding a descriptor row fails a
  focused test.
- Adding a descriptor row for a non-enum or duplicate kind fails a focused test.
- `describeObject(...)` behavior for `Unknown` and invalid cast values remains
  deterministic.
- Existing descriptor row count tests no longer rely only on a magic number.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_room_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_room_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not reorder existing enum values in a way that breaks save compatibility.
- Do not remove the Unknown descriptor.
- Do not change descriptor facts or object behavior in this card.
- Do not combine this with E40 row-builder readability work.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/document/Object.hpp`
  - `src/app/iggy3d/creative/document/Object.cpp`
  - `src/app/iggy3d/creative/mutation/Mutation.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
- Added terminal `CreativeObjectKind::Count` without reordering existing enum
  values.
- Added `allCreativeObjectKinds()` generated from the contiguous enum range
  before `Count`.
- Added `Count` handling to `toString(CreativeObjectKind)` and
  `allowedMutations(...)` so sentinel/invalid values remain deterministic.
- Replaced the descriptor test's magic row-count reliance with:
  - descriptor count equals `allCreativeObjectKinds().size()`;
  - every known kind has exactly one descriptor row;
  - every descriptor row uses a kind from the known inventory;
  - descriptor names match `toString(kind)`;
  - `Unknown`, `Count`, and invalid casts describe/stringify deterministically.
- Descriptor facts and existing object behavior were not changed.
- Verification:
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_room_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_room_tests)$' --output-on-failure`
- Result: all checks passed.
