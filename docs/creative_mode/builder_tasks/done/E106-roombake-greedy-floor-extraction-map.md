# E106: RoomBake Greedy Floor Extraction Map

## Objective

Produce a concrete extraction map for the greedy-floor RoomBake block before
moving code. This is a read-only/design card unless a trivially safe mechanical
extraction falls out during inspection; default to no source edits.

After E104/E105, `RoomBake.cpp` is down to about 1,024 LOC. The next cohesive
block is greedy structural floor mesh collapse, but it currently depends on
RoomBake-local internals (`BakeBounds`, `BakedRoomRole`, `BakeStaticMeshEntry`,
stable mesh ids, walkable surface source ids, and sidecars). Extracting it
blindly risks creating another semi-public internal API or duplicating policy.

## Scope

Read only unless the extraction is mechanically obvious and stays very small.

Inspect:

- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `src/app/iggy3d/creative/adapters/RoomBake.hpp`
- `src/core/grid/GreedyMesh.hpp`
- `src/core/grid/GridFootprint.hpp`
- `tests/unit/creative_document_room_bake_tests.cpp`

## Questions To Answer

1. Which exact structs/functions form the greedy-floor block?
2. Which dependencies are pure data and which are RoomBake policy?
3. What is the smallest safe helper API if this becomes
   `RoomBakeGreedyFloors.hpp/.cpp`?
4. Should the helper return a pure plan, or should it append meshes/surfaces
   directly?
5. What tests must exist before extraction to prove no ordering/source-sidecar
   drift?

## Required Output

Append a completion brief to this card with:

- line ranges for the greedy-floor block;
- proposed destination files;
- proposed internal structs/API;
- exact source-policy dependencies that should stay in `RoomBake.cpp`;
- minimum test guard list;
- recommendation: implement extraction now, add guard tests first, or defer.

## Do Not

- Do not change runtime behavior.
- Do not change RoomBake policy, descriptors, renderer, save/load, input, or
  standalone app.
- Do not stage, commit, push, launch a window, or run broad CTest.
- Do not create a broad `RoomBakeInternal.hpp` dumping ground unless the
  completion brief proves it is the least-bad option.

## Suggested Verification

If no source edits:

```sh
git -C /Users/kogaryu/iggy3d status --short
git -C /Users/kogaryu/iggy3d diff --check
```

If source edits occur, run the focused RoomBake tests and explain why edits were
safe.

## Completed

- Files changed:
  - `docs/creative_mode/builder_tasks/claimed/E106-roombake-greedy-floor-extraction-map.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Line ranges for the greedy-floor block:
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp:88-106`: greedy floor local data structs:
    `GreedyFloorCandidate`, `GreedyFloorGroup`, and `GreedyFloorMesh`.
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp:580-595`: aligned grid footprint conversion for floor bounds.
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp:597-605`: greedy candidate predicate.
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp:607-655`: source dedupe, merged mesh id policy, and per-object fallback mesh helpers.
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp:657-779`: bounded-grid construction, `greedyMeshGrid(...)` call, quad-to-source mapping, and merged mesh construction.
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp:781-846`: grouping by Y layer and floor role/material/mesh identity, fallback collection, and stable output ordering.
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp:848-871`: appending merged meshes plus source sidecars and per-object walkable surfaces.
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp:975-990`: final bake consumption seam that substitutes greedy floor meshes for per-object floor meshes.
- Proposed destination files:
  - `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.hpp`
  - `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.cpp`
  - Keep the helper private to the adapter layer; do not add it to `RoomBake.hpp` unless another caller appears.
- Proposed helper shape:
  - Prefer a pure plan helper, not a helper that mutates `RoomAsset` directly.
  - RoomBake should continue to own object classification, receipt counting, anchor handling, non-floor mesh append, and final room/source/surface append policy.
  - Suggested narrow inputs:
    - `RoomBakeGreedyFloorInput { CreativeObjectId objectId; std::size_t documentIndex; Vec3 min; Vec3 max; Vec3 center; Vec3 size; std::string singleMeshId; }`
    - `RoomBakeGreedyFloorPolicy { float cellSizeMeters = 1.0F; std::int64_t maxCells = 1'000'000; std::string meshId = "creative_floor_rect"; std::string materialId = "creative_floor"; std::string role = "floor"; }`
  - Suggested outputs:
    - `RoomBakeGreedyFloorSource { CreativeObjectId objectId; std::size_t documentIndex; Vec3 min; Vec3 max; Vec3 center; Vec3 size; }`
    - `RoomBakeGreedyFloorMeshPlan { RoomStaticMeshAsset mesh; std::vector<RoomBakeGreedyFloorSource> sources; std::size_t firstDocumentIndex; }`
    - `buildRoomBakeGreedyFloorPlan(const std::vector<RoomBakeGreedyFloorInput>&, const RoomBakeGreedyFloorPolicy&)`
  - The returned plan should include fallback per-object floor meshes as plan items so the caller can append one ordered stream.
- Source-policy dependencies that should stay in `RoomBake.cpp`:
  - `RoomBakeObjectClassification`, `RoomBakeObjectDecision`, `BakeStaticMeshEntry`, and `BakedRoomRole` unless a later card deliberately extracts all static-mesh classification.
  - Descriptor-driven eligibility: `CreativeObjectKind::Floor`, shape `Surface`, occupancy `Structural`, and role `Floor`.
  - Stable object id policy for non-floor room objects and anchor ids.
  - Receipt counters and skip counters.
  - `RoomAsset` assembly order across anchors, floor plans, non-floor meshes, and reachability.
  - Spatial surface policy: walkable surfaces remain per authored floor object, with `sourceStaticMeshId` pointing to the merged greedy mesh.
  - Source sidecar append policy for `CreativeRoomBakeStaticMeshSource` and `CreativeRoomBakeSpatialSurfaceSource`.
- Pure data dependencies that can move:
  - Grid footprint alignment for floor bounds.
  - Group bounding region and `kMaxGreedyFloorCells` guard.
  - Dense grid allocation and duplicate-cell fallback.
  - `greedyMeshGrid(...)` invocation.
  - Quad bounds calculation.
  - Source dedupe and source sorting inside one quad.
  - Greedy merged mesh id construction, if supplied with source object ids and stable single-object mesh ids.
- Minimum test guard list before extraction:
  - Existing `adjacentStructuralFloorsCollapseToSingleGreedyMesh()` must stay green: 25 floors to 1 mesh, all 25 mesh sources, all 25 per-object walkable surface sources.
  - Existing `nearAlignedGreedyFloorDoesNotExpandAcrossNeighborCell()` must stay green.
  - Add an ordering guard for multiple floor groups interleaved with non-floor meshes so `RoomAsset.staticMeshes` order and source sidecar order do not drift.
  - Add a fallback-order guard for a non-grid-aligned floor mixed with a greedy group.
  - Add a multi-quad guard such as an L-shaped floor layout where one group produces more than one greedy quad, with first-document-index ordering and per-quad source ids pinned.
  - Add a different-Y-layer guard proving coplanar grouping does not merge floors from different minY/maxY layers.
  - Keep the surface-source policy pinned: per-object walkable surfaces survive even when static meshes merge, and every surface points at the correct merged mesh id.
- Recommendation:
  - Do not extract the source yet. E107 should add the ordering/source-sidecar guards first.
  - After E107, extract to `RoomBakeGreedyFloors.hpp/.cpp` as a pure plan builder and leave RoomBake classification, receipt, and room-append policy in `RoomBake.cpp`.
  - Avoid a broad `RoomBakeInternal.hpp`; the current dependency graph does not justify a dumping-ground header.
- Verification:
  - Read-only/source-neutral card. No source edits.
  - Run `git -C /Users/kogaryu/iggy3d status --short`.
  - Run `git -C /Users/kogaryu/iggy3d diff --check`.
