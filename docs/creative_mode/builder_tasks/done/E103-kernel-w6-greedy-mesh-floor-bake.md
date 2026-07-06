# E103: Kernel W6 - GreedyMesh Structural Floor Bake Collapse

## Objective

Use the shipped `core/grid/GreedyMesh` kernel to collapse adjacent same-material
structural Floor bake output, starting with floor planes only.

## Source Brief

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`, section W6.

## Scope

- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- focused RoomBake tests only.

## Required Behavior

- Start with Structural Floor objects only.
- Group coplanar same-role/same-material floor objects.
- Build a bounded grid only around the group, call `greedyMeshGrid(...)`, and
  emit one `RoomStaticMeshAsset` per greedy quad.
- Preserve collision/walkable spatial surface coverage.
- Preserve source attribution by extending sidecars or adding grouped source
  records. Do not lose object ownership diagnostics.

## Hazards

- Merging many objects into one mesh breaks the old one-mesh-per-object identity
  assumption. The implementation must preserve enough source attribution for
  receipts/tests.
- Spatial surface policy must be explicit: either merge the walkable surfaces or
  keep per-object surfaces while explaining the choice.
- Only grid the group's occupied bounding region; do not allocate a huge sparse
  document-wide grid.
- Walls, props, openings, stairs, and line geometry are out of scope for this
  first greedy-mesh slice.

## Do Not

- Do not change RoomBake object eligibility.
- Do not change renderer/Vulkan, product input, save/load, descriptors, or
  creative mutation policy.
- Do not remove source sidecars.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`
- `src/core/grid/GreedyMesh.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- existing RoomBake tests.

## Acceptance

- A 5x5 same-material floor layout bakes to fewer static meshes; target proof is
  25 floor objects -> 1 floor mesh when they form one rectangle.
- Rendered/baked floor extents are identical.
- Collision/walkable surface coverage still covers the full area.
- Source sidecar evidence still identifies all contributing CreativeObject ids.
- Existing Floor/Wall/Crate/Beam/Point/Path RoomBake tests remain green.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target creative_document_room_bake_tests render_room_mesh_geometry_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_room_bake_tests|render_room_mesh_geometry_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Build `iggy3d` too if public headers or compile coverage require it.

## Completion Brief

Append:

- Files changed:
- Greedy grouping policy:
- Source attribution policy:
- Spatial surface policy:
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `src/app/iggy3d/creative/adapters/RoomBake.hpp`
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `tests/unit/creative_document_room_bake_tests.cpp`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/done/E103-kernel-w6-greedy-mesh-floor-bake.md`
- Greedy grouping policy:
  - Structural `Floor` objects only.
  - Candidates must be bounds-backed, coplanar by floor `minY/maxY`, and same
    floor role/material/mesh policy.
  - A bounded 1m XZ grid is built only around each floor group.
  - Non-aligned, overlapping, invalid, or oversized groups conservatively fall
    back to the prior per-object floor mesh path.
- Source attribution policy:
  - `RoomAsset.staticMeshes` emits one floor mesh per greedy quad.
  - `CreativeRoomBakeStaticMeshSource` remains the sidecar. Multiple records may
    share one `staticMeshId` when several CreativeObjects contribute to one
    merged mesh.
  - The 5x5 proof records all 25 contributing object ids against the single
    merged floor mesh.
- Spatial surface policy:
  - Walkable spatial surfaces remain per authored Floor object to preserve exact
    existing collision/walkable coverage.
  - Per-object walkable surfaces now point `sourceStaticMeshId` at the merged
    greedy mesh id when their floor object was merged.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_document_room_bake_tests render_room_mesh_geometry_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_room_bake_tests|render_room_mesh_geometry_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Concerns/deferred:
  - This slice intentionally does not merge walkable surfaces, walls, props,
    line geometry, openings, stairs, or non-1m-aligned floor regions.
