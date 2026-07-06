# E108: RoomBake Greedy Floor Plan Extraction

## Objective

Extract the greedy structural floor mesh-collapse block from `RoomBake.cpp` into
a narrow adapter-private pure plan helper.

E106 mapped the boundary. E107 added ordering and sidecar guard tests. This card
is the implementation step.

## Scope

Expected files:

- Add `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.hpp`
- Add `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.cpp`
- Update `CMakeLists.txt`
- Update `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- Use existing RoomBake tests; add only minimal tests if extraction exposes a
  missing case.

## Required Shape

Prefer a pure plan builder. The helper should not mutate `RoomAsset` directly.

Reasonable shape:

```cpp
struct RoomBakeGreedyFloorInput {
  CreativeObjectId objectId;
  std::size_t documentIndex;
  Vec3 min;
  Vec3 max;
  Vec3 center;
  Vec3 size;
  std::string singleMeshId;
};

struct RoomBakeGreedyFloorPolicy {
  float cellSizeMeters = 1.0F;
  std::int64_t maxCells = 1'000'000;
  std::string meshId = "creative_floor_rect";
  std::string materialId = "creative_floor";
  std::string role = "floor";
};

struct RoomBakeGreedyFloorSource {
  CreativeObjectId objectId;
  std::size_t documentIndex;
  Vec3 min;
  Vec3 max;
  Vec3 center;
  Vec3 size;
};

struct RoomBakeGreedyFloorMeshPlan {
  RoomStaticMeshAsset mesh;
  std::vector<RoomBakeGreedyFloorSource> sources;
  std::size_t firstDocumentIndex;
};

std::vector<RoomBakeGreedyFloorMeshPlan> buildRoomBakeGreedyFloorPlan(...);
```

Follow repo naming/style if a smaller equivalent is cleaner.

## Ownership Rules

Move into the helper:

- aligned grid footprint conversion for floor bounds;
- grouping by Y layer and floor role/material/mesh identity;
- bounded grid allocation and max-cell guard;
- `greedyMeshGrid(...)` invocation;
- overlapping-cell fallback;
- quad bounds calculation;
- source dedupe/sorting inside quads;
- merged greedy floor mesh id construction when multiple sources contribute.

Keep in `RoomBake.cpp`:

- descriptor/object classification;
- decision that only actual `CreativeObjectKind::Floor`, `Surface`,
  `Structural`, `Floor` role entries are eligible;
- receipt counters and skip counters;
- `RoomAsset` assembly;
- `CreativeRoomBakeStaticMeshSource` and
  `CreativeRoomBakeSpatialSurfaceSource` append policy;
- per-authored-floor walkable surface policy and sourceStaticMeshId assignment;
- non-floor static mesh, anchor, and reachability routing.

Do not introduce a broad `RoomBakeInternal.hpp` dumping ground.

## Required Behavior

- All E107 guard tests continue to pass:
  - interleaved source ordering;
  - nonaligned floor fallback;
  - separated aligned islands preserving gaps.
- Existing 25-floor collapse and near-aligned floor tests continue to pass.
- No RoomBake output counts or source-sidecar order changes.

## Do Not

- Do not change RoomBake eligibility or policy.
- Do not change `GreedyMesh`, `GridFootprint`, descriptors, renderer, save/load,
  input, standalone app, or reachability behavior.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Extraction shape:
- Behavior preserved:
- Tests/checks run:
- Concerns/deferred:

