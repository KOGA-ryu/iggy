# E93: Descriptor Kind Totality Guard

## Objective

Add a totality guard so every valid `CreativeObjectKind` descriptor reports
`describeObject(kind).kind == kind`.

## Problem

The descriptor queue already relies on descriptor rows as object truth. A new
enum value can silently degrade to `Unknown` if the descriptor table misses it,
which can make that object vanish from palette, bake, graph, or shape-driven
routes.

## Scope

- Add a focused descriptor totality test or compile-time guard where it fits the
  existing descriptor test style.
- Verify every valid object kind round-trips through `describeObject(kind)`.
- Keep this as descriptor registry/test protection only.

## Do Not

- Do not introduce per-kind editor files or a parallel role enum.
- Do not change descriptor policy unless the test exposes an actual missing row.
- Do not touch SaveFileStore twins.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`

## Acceptance

- The guard fails if any valid `CreativeObjectKind` maps to a descriptor whose
  `kind` differs from the requested kind.
- Existing descriptor shape/projection tests still pass.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target creative_object_descriptor_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_object_descriptor_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files modified:
- Guard added:
- Verification:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files modified:
  - `tests/unit/creative_object_descriptor_tests.cpp`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/claimed/E93-descriptor-kind-totality-guard.md`
- Guard added:
  - Added `descriptorLookupIsTotalForEveryEnumKind()`.
  - The guard checks `allCreativeObjectKinds().size() ==
    static_cast<std::size_t>(CreativeObjectKind::Count)`.
  - It walks every ordinal below `Count`, verifies the inventory follows enum
    order, and verifies `describeObject(kind).kind == kind`.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_object_descriptor_tests -j10`
    passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_object_descriptor_tests$' --output-on-failure`
    passed.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing whitespace scan over touched files passed.
- Concerns/deferred:
  - No descriptor policy changed.
  - No SaveFileStore files touched.
  - No commit/stage/push performed.
