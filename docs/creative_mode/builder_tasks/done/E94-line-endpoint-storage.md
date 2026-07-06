# E94: Line Endpoint Storage For Traversal Links

## Objective

Add real stored endpoint data for Line-shaped traversal link objects so pure
reader kernels can consume authored `NavLink`, `JumpLink`, and `ClimbLink`
geometry.

## Problem

`NavLink`, `JumpLink`, and `ClimbLink` are Line-shaped descriptors with no
bounds-backed storage. Today they have no persisted start/end truth on
`CreativeObject`, so they bake/export to nothing and cannot feed
`traversalLinksFromDocument`.

This is not an editor handle task. It is document truth.

## Scope

- Decide and implement the smallest durable representation for two-point Line
  objects.
- Prefer reusing existing `CreativeObject::pathPoints` if it fits the document,
  save, projection, mutation, and reader contract cleanly.
- If a dedicated start/end payload is more correct, add it intentionally and
  update create/restore/save/mutation tests.
- Support at least `NavLink`, `JumpLink`, and `ClimbLink`.
- Keep Path (`PatrolRoute`) semantics intact.
- Provide public read behavior that a pure kernel can consume without app-local
  state.

## Do Not

- Do not add authoring handles or gizmo UI.
- Do not fake endpoints from bounds.
- Do not add app-local endpoint state.
- Do not touch SaveFileStore twins.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `src/app/iggy3d/creative/document/Object.hpp`
- `src/app/iggy3d/creative/document/Document.hpp`
- `src/app/iggy3d/creative/document/Document.cpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/creative/mutation/Mutation.hpp`
- `src/app/iggy3d/creative/mutation/Mutation.cpp`
- `src/app/iggy3d/creative/mutation/MutationApply.cpp`
- `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
- `src/app/iggy3d/creative/document/DocumentWireframe.cpp`
- `src/app/iggy3d/creative/world/DocumentSection.cpp`
- `src/runtime/save/SaveEnvelope.hpp`
- `src/runtime/save/SaveCodec.cpp`
- focused creative document/path/line/save/projection/wireframe tests.

## Acceptance

- `NavLink`, `JumpLink`, and `ClimbLink` can be created/restored with exactly
  two finite ordered endpoints.
- Invalid endpoint count or non-finite endpoints reject deterministically and do
  not mutate document state.
- Save/load preserves endpoint order and values.
- Mutation support can update endpoints through a public mutation seam.
- Projection/wireframe can consume stored endpoints without bounds fallback.
- Existing `PatrolRoute` path-point behavior remains unchanged.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_path_tests|creative_document_mutation_tests|creative_document_save_section_tests|save_creative_document_section_tests|creative_spatial_projection_tests|creative_document_wireframe_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files modified:
- Endpoint storage decision:
- Create/restore/save/mutation policy:
- Reader contract exposed:
- Verification:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files modified:
  - `src/app/iggy3d/creative/document/Document.cpp`
  - `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
  - `src/app/iggy3d/creative/mutation/MutationApply.cpp`
  - `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
  - `src/app/iggy3d/creative/document/DocumentWireframe.cpp`
  - `tests/unit/creative_document_path_tests.cpp`
  - `tests/unit/creative_document_save_section_tests.cpp`
  - `tests/unit/creative_spatial_projection_tests.cpp`
  - `tests/unit/creative_document_wireframe_tests.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/claimed/E94-line-endpoint-storage.md`
- Endpoint storage decision:
  - Reused `CreativeObject::pathPoints` for endpoint-backed Line objects.
  - `NavLink`, `JumpLink`, and `ClimbLink` now use
    `CreativeSpatialProjectionProfile::LinkProjection`.
  - Reader contract: for endpoint-backed traversal links, `object.pathPoints`
    contains exactly two ordered points; index `0` is start, index `1` is end.
  - Bounds-backed Line objects remain bounds-backed and do not use this payload.
- Create/restore/save/mutation policy:
  - Create requires `hasPathOverride=true` and exactly two finite points for
    endpoint-backed links.
  - Missing endpoint override rejects with `line_endpoint_override_required`.
  - Wrong count or non-finite endpoints reject with `invalid_line_endpoints`.
  - Restore validates the same exact-two endpoint policy.
  - Save-section build and codec round-trip preserve ordered endpoint points.
  - Existing `SetPatrolRoute + PathPointsMutation` public mutation seam now
    updates endpoint-backed links as stored line endpoints.
  - `PatrolRoute` Path semantics remain at-least-two ordered points.
- Reader contract exposed:
  - `projectLinkObjectToGrid(...)` reads stored endpoints and projects sampled
    navigation cells without bounds fallback.
  - Wireframe generation reads stored endpoints for `LinkProjection` items and
    emits one Navigation-styled line segment.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d -j10` passed.
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_document_path_tests creative_document_mutation_tests creative_document_save_section_tests save_creative_document_section_tests creative_spatial_projection_tests creative_document_wireframe_tests creative_object_descriptor_tests -j10`
    passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_path_tests|creative_document_mutation_tests|creative_document_save_section_tests|save_creative_document_section_tests|creative_spatial_projection_tests|creative_document_wireframe_tests|creative_object_descriptor_tests)$' --output-on-failure`
    passed.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing whitespace scan over touched E94 files passed.
- Concerns/deferred:
  - No authoring handles or editor UI were added.
  - `SetPatrolRoute` remains the shared public mutation kind for stored
    path-points payloads; a future naming cleanup could add an alias, but this
    slice avoided enum churn.
  - `src/app/iggy3d/creative/tools/RoomShell.cpp` was dirty before this task and
    was not modified for E94.
