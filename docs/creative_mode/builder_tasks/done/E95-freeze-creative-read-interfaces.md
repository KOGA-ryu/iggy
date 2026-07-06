# E95: Freeze Creative Descriptor And Document Read Interfaces

## Objective

Confirm and freeze the creative descriptor/document read interfaces needed by
pure validation/export kernels.

## Problem

Downstream kernels need stable read-only access to creative descriptor truth and
document object payloads:

- `validateDocumentPreBake`
- `patrolRouteWaypointsFromDocument`
- future traversal/export readers.

The E-queue has been actively changing these files, so this card exists to
finish the contract review and state what is stable.

## Scope

- Audit and pin the read interfaces:
  - `describeObject(CreativeObjectKind)`
  - `CreativeObjectDescriptor` fields: `shapeKind`, `occupancyKind`, bounds
    defaults, `runtimeAnchorSemantic`, projection/profile/category facts.
  - `CreativeDocument::objects()` read access.
  - `CreativeObject` read fields: `id`, `kind`, `transform`, `bounds`,
    `parentId`, `pathPoints`.
- Add or tighten focused tests only if a read contract lacks protection.
- Update the card completion brief with exact stable interface statements.

## Do Not

- Do not refactor descriptor shape policy.
- Do not add object kinds.
- Do not change save/load or mutation behavior unless a missing guard exposes a
  direct correctness hole.
- Do not touch SaveFileStore twins.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/creative/document/Object.hpp`
- `src/app/iggy3d/creative/document/Document.hpp`
- `src/app/iggy3d/creative/document/Document.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- relevant document path/save/create tests.

## Acceptance

- Completion brief states the stable read contract downstream kernels can rely
  on.
- Focused tests prove descriptor totality, shape/projection separation, and
  document object payload preservation where needed.
- Any unstable seam is explicitly called out as blocked/deferred instead of
  silently changed.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target creative_object_descriptor_tests creative_document_path_tests creative_document_create_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_path_tests|creative_document_create_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files inspected:
- Files modified:
- Stable descriptor read contract:
- Stable document/object read contract:
- Tests/guards:
- Verification:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files inspected:
  - `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
  - `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
  - `src/app/iggy3d/creative/document/Object.hpp`
  - `src/app/iggy3d/creative/document/Document.hpp`
  - `src/app/iggy3d/creative/document/Document.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
  - `tests/unit/creative_document_path_tests.cpp`
  - `tests/unit/creative_document_save_section_tests.cpp`
  - `tests/unit/creative_document_create_tests.cpp`
- Files modified:
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/claimed/E95-freeze-creative-read-interfaces.md`
- Stable descriptor read contract:
  - `describeObject(CreativeObjectKind)` is stable as the descriptor lookup for
    every valid object kind.
  - `CreativeObjectDescriptor::kind`, `category`, `profile`, `shapeKind`,
    `projectionProfile`, `occupancyKind`, `runtimeAnchorSemantic`,
    `authoringPaletteVisibility`, `creationDirtyFlags`, `defaults`,
    `hasTransform`, `hasBounds`, `canHaveParent`, `canOwnChildren`,
    `isRuntimeMeaningful`, and `isEditorOnly` are stable read fields for pure
    kernels.
  - Shape and projection remain separate columns. Examples now pinned include
    `PatrolRoute` as `Path + PathProjection`, traversal links as
    `Line + LinkProjection`, and bounds-backed Line objects such as camera rail
    as `Line + LineProjection`.
  - `runtimeAnchorSemantic` is stable for point-anchor export readers.
- Stable document/object read contract:
  - `CreativeDocument::objects()` is the stable ordered read view for pure
    readers.
  - `CreativeObject::id`, `kind`, `transform`, `bounds`, `parentId`, and
    `pathPoints` are stable read fields.
  - `PatrolRoute` stores ordered waypoint payload in `pathPoints` with at least
    two finite points.
  - `NavLink`, `JumpLink`, and `ClimbLink` store ordered endpoint payload in
    `pathPoints` with exactly two finite points; index `0` is start and index
    `1` is end.
  - Bounds-backed Line objects continue to expose authored geometry through
    `bounds`, not `pathPoints`.
- Tests/guards:
  - `creative_object_descriptor_tests` guards descriptor totality and
    descriptor helper round-trips.
  - `creative_object_descriptor_tests` also pins shape/projection divergence and
    representative runtime anchor semantics.
  - `creative_document_path_tests` guards Path and endpoint-backed Line
    create/restore/mutation payload preservation.
  - `creative_document_save_section_tests` guards save-section and codec
    round-trip preservation for both `PatrolRoute` path points and `NavLink`
    endpoints.
  - `creative_document_create_tests` remains green against the create contract.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_object_descriptor_tests creative_document_path_tests creative_document_create_tests -j10`
    passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_path_tests|creative_document_create_tests)$' --output-on-failure`
    passed.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing whitespace scan over E95-touched files passed.
- Concerns/deferred:
  - No production code changed for E95; it is a freeze/audit card after E94.
  - Future descriptor expansion should continue adding descriptor rows and tests
    rather than per-object reader branches.
  - `src/app/iggy3d/creative/tools/RoomShell.cpp` remains a pre-existing dirty
    file outside this card.
