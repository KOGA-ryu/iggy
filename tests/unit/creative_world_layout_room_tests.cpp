#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-9;
}

bool nearFloat(float lhs, float rhs) {
  return std::abs(lhs - rhs) <= 1.0e-5F;
}

const cr::CreativeObject* findKind(const cr::CreativeDocument& document,
                                   cr::CreativeObjectKind kind) {
  const auto found = std::find_if(
      document.objects().begin(), document.objects().end(),
      [kind](const cr::CreativeObject& object) { return object.kind == kind; });
  return found == document.objects().end() ? nullptr : &*found;
}

std::string stableObjectId(cr::CreativeObjectId objectId) {
  return "creative_object_" + std::to_string(objectId);
}

const iggy3d::RoomStaticMeshAsset* findMesh(
    const iggy3d::RoomAsset& room, cr::CreativeObjectId objectId) {
  const std::string id = stableObjectId(objectId);
  const auto found = std::find_if(
      room.staticMeshes.begin(), room.staticMeshes.end(),
      [&id](const iggy3d::RoomStaticMeshAsset& mesh) { return mesh.id == id; });
  return found == room.staticMeshes.end() ? nullptr : &*found;
}

const iggy3d::PhysicsAabbCollider* findCollider(
    const iggy3d::PhysicsSpatialSurfaceColliderBakeResult& physics,
    std::string_view surfaceId) {
  const auto found = std::find(physics.sourceSurfaceIds.begin(),
                               physics.sourceSurfaceIds.end(), surfaceId);
  if (found == physics.sourceSurfaceIds.end()) {
    return nullptr;
  }
  const std::size_t index = static_cast<std::size_t>(
      std::distance(physics.sourceSurfaceIds.begin(), found));
  return index < physics.colliders.size() ? &physics.colliders[index] : nullptr;
}

bool colliderMatchesBounds(const iggy3d::PhysicsAabbCollider& collider,
                           cr::CreativeBounds bounds) {
  return nearFloat(collider.bounds.min.x, static_cast<float>(bounds.min.x)) &&
         nearFloat(collider.bounds.min.y, static_cast<float>(bounds.min.y)) &&
         nearFloat(collider.bounds.min.z, static_cast<float>(bounds.min.z)) &&
         nearFloat(collider.bounds.max.x, static_cast<float>(bounds.max.x)) &&
         nearFloat(collider.bounds.max.y, static_cast<float>(bounds.max.y)) &&
         nearFloat(collider.bounds.max.z, static_cast<float>(bounds.max.z));
}

cr::CreativeWorldLayout adjacentRooms() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "room_topology";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_1";
  building.name = "House";
  layout.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "level_ground";
  level.name = "Ground Level";
  layout.levels.push_back(level);

  cr::CreativeWorldLayoutRoom left;
  left.buildingIndex = 0U;
  left.levelIndex = 0U;
  left.stableKey = "room_left";
  left.name = "Left Room";
  left.footprint = {{0, 0}, {4, 4}};
  layout.rooms.push_back(left);

  cr::CreativeWorldLayoutRoom right = left;
  right.stableKey = "room_right";
  right.name = "Right Room";
  right.footprint = {{4, 0}, {8, 4}};
  layout.rooms.push_back(right);

  cr::CreativeWorldLayoutOpening door;
  door.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  door.roomIndex = 1U;
  door.roomEdge = cr::CreativeWorldLayoutRoomEdge::West;
  door.kind = cr::CreativeBuildingOpeningKind::Door;
  door.stableKey = "shared_door";
  door.name = "Shared Door";
  door.centerOffsetCells = 2.0;
  layout.openings.push_back(door);
  return layout;
}

cr::CreativeWorldLayout orthogonalLRoom() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "orthogonal_l_room";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_l";
  building.name = "L Building";
  building.rootFootprint = {{0, 0}, {6, 6}};
  layout.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground_l";
  level.name = "Ground";
  layout.levels.push_back(level);
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room_l";
  room.name = "L Room";
  room.footprint = {{0, 0}, {6, 6}};
  layout.rooms.push_back(room);

  const cr::CreativeTerrainCoord2 points[] = {
      {0, 0}, {6, 0}, {6, 2}, {2, 2}, {2, 6}, {0, 6},
  };
  for (std::size_t index = 0U; index < std::size(points); ++index) {
    layout.topologyVertices.push_back(
        {0U, "l_vertex_" + std::to_string(index), points[index]});
  }
  const std::pair<std::size_t, std::size_t> edges[] = {
      {0U, 1U}, {1U, 2U}, {3U, 2U},
      {3U, 4U}, {5U, 4U}, {0U, 5U},
  };
  const bool reversed[] = {false, false, true, false, true, true};
  for (std::size_t index = 0U; index < std::size(edges); ++index) {
    layout.topologyEdges.push_back(
        {0U, "l_edge_" + std::to_string(index), edges[index].first,
         edges[index].second, 0.25});
    layout.roomBoundaries.push_back({0U, index, index, reversed[index]});
  }
  return layout;
}

bool roomEdgesHaveStableCardinalIdentity() {
  return expect(static_cast<std::uint8_t>(
                    cr::CreativeWorldLayoutRoomEdge::North) == 0U &&
                    static_cast<std::uint8_t>(
                        cr::CreativeWorldLayoutRoomEdge::East) == 1U &&
                    static_cast<std::uint8_t>(
                        cr::CreativeWorldLayoutRoomEdge::South) == 2U &&
                    static_cast<std::uint8_t>(
                        cr::CreativeWorldLayoutRoomEdge::West) == 3U,
                "cardinal room edge values remain serialization-stable") &&
         expect(cr::toString(cr::CreativeWorldLayoutRoomEdge::North) ==
                        "North" &&
                    cr::creativeWorldLayoutRoomEdgeKey(
                        cr::CreativeWorldLayoutRoomEdge::North) == "north" &&
                    cr::creativeWorldLayoutRoomEdgeKey(
                        cr::CreativeWorldLayoutRoomEdge::East) == "east" &&
                    cr::creativeWorldLayoutRoomEdgeKey(
                        cr::CreativeWorldLayoutRoomEdge::South) == "south" &&
                    cr::creativeWorldLayoutRoomEdgeKey(
                        cr::CreativeWorldLayoutRoomEdge::West) == "west",
                "room edge labels provide stable authored side identities");
}

bool adjacentRoomsShareOneCanonicalWall() {
  const cr::CreativeWorldLayoutRoomCompileResult first =
      cr::expandCreativeWorldLayoutRooms(adjacentRooms());
  const cr::CreativeWorldLayoutRoomCompileResult second =
      cr::expandCreativeWorldLayoutRooms(adjacentRooms());
  if (!expect(first.accepted && second.accepted,
              "adjacent room expansion is accepted")) {
    return false;
  }
  const cr::CreativeWorldLayoutOpening& opening = first.expanded.openings[0];
  const cr::CreativeWorldLayoutWall& host =
      first.expanded.walls[opening.wallIndex];
  return expect(first.expanded.boxes.empty(),
                "room floors bypass generic layout boxes") &&
         expect(near(first.expanded.walls[0].baseLayer, 0.0),
                "room walls begin on the authored floor-top plane") &&
         expect(first.expanded.walls.size() == 5U,
                "shared boundary is emitted exactly once") &&
         expect(
             opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
                 host.start == cr::CreativeTerrainCoord2{4, 0} &&
                 host.end == cr::CreativeTerrainCoord2{4, 4} &&
                 opening.centerOffsetCells == 2.0,
             "room-edge opening resolves onto the canonical shared wall") &&
         expect(first.expanded.walls[0].stableKey ==
                        second.expanded.walls[0].stableKey &&
                    first.expanded.walls.back().start ==
                        second.expanded.walls.back().start,
                "room expansion ordering is deterministic");
}

bool sharedEdgeInspectionMatchesCanonicalTopology() {
  const cr::CreativeWorldLayout adjacent = adjacentRooms();
  const auto adjacentSpans =
      cr::inspectCreativeWorldLayoutSharedRoomEdges(adjacent);

  cr::CreativeWorldLayout partial = adjacent;
  partial.openings.clear();
  partial.rooms[1].footprint = {{4, 1}, {8, 3}};
  const auto partialSpans =
      cr::inspectCreativeWorldLayoutSharedRoomEdges(partial);
  const bool interiorInterval =
      cr::creativeWorldLayoutRoomEdgeIntervalIsShared(
          partial, 0U, cr::CreativeWorldLayoutRoomEdge::East, 2.0, 1.0);
  const bool exteriorInterval =
      cr::creativeWorldLayoutRoomEdgeIntervalIsShared(
          partial, 0U, cr::CreativeWorldLayoutRoomEdge::East, 0.25, 0.25);

  cr::CreativeWorldLayout differentHeight = adjacent;
  differentHeight.openings.clear();
  cr::CreativeWorldLayoutLevel upper = differentHeight.levels[0];
  upper.stableKey = "level_upper";
  upper.name = "Upper Level";
  upper.floorTopLayer = 3.0;
  upper.wallHeightCells += 1U;
  differentHeight.levels.push_back(upper);
  differentHeight.rooms[1].levelIndex = 1U;
  const auto differentHeightSpans =
      cr::inspectCreativeWorldLayoutSharedRoomEdges(differentHeight);

  return expect(adjacentSpans.size() == 1U &&
                    adjacentSpans[0].firstRoomIndex == 0U &&
                    adjacentSpans[0].firstRoomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    adjacentSpans[0].secondRoomIndex == 1U &&
                    adjacentSpans[0].secondRoomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::West &&
                    adjacentSpans[0].start ==
                        cr::CreativeTerrainCoord2{4, 0} &&
                    adjacentSpans[0].end ==
                        cr::CreativeTerrainCoord2{4, 4},
                "shared edge inspection emits one stable contributor pair") &&
         expect(partialSpans.size() == 1U &&
                    partialSpans[0].start ==
                        cr::CreativeTerrainCoord2{4, 1} &&
                    partialSpans[0].end ==
                        cr::CreativeTerrainCoord2{4, 3} &&
                    interiorInterval && !exteriorInterval,
                "partial shared edges classify only their interior interval") &&
         expect(differentHeightSpans.empty(),
                "edges with incompatible wall geometry remain exterior");
}

bool stackedRoomsUseBuildingFacadesAndLevelPartitions() {
  cr::CreativeWorldLayout layout = adjacentRooms();
  cr::CreativeWorldLayoutLevel upperLevel = layout.levels[0];
  upperLevel.stableKey = "level_upper";
  upperLevel.name = "Upper Level";
  upperLevel.floorTopLayer = 3.0;
  layout.levels.push_back(upperLevel);

  cr::CreativeWorldLayoutRoom upperLeft = layout.rooms[0];
  upperLeft.levelIndex = 1U;
  upperLeft.stableKey = "room_upper_left";
  upperLeft.name = "Upper Left Room";
  layout.rooms.push_back(upperLeft);
  cr::CreativeWorldLayoutRoom upperRight = layout.rooms[1];
  upperRight.levelIndex = 1U;
  upperRight.stableKey = "room_upper_right";
  upperRight.name = "Upper Right Room";
  layout.rooms.push_back(upperRight);

  cr::CreativeWorldLayoutOpening upperWindow;
  upperWindow.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  upperWindow.roomIndex = 2U;
  upperWindow.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  upperWindow.kind = cr::CreativeBuildingOpeningKind::Window;
  upperWindow.stableKey = "upper_window";
  upperWindow.name = "Upper Window";
  upperWindow.centerOffsetCells = 2.0;
  upperWindow.widthCells = 1.5;
  upperWindow.cutoutBottomCells = 1.0;
  upperWindow.cutoutHeightCells = 1.2;
  upperWindow.insertHeightCells = 1.2;
  layout.openings.push_back(upperWindow);

  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(layout);
  if (!expect(expanded.accepted,
              "stacked room expansion is accepted")) {
    return false;
  }

  std::size_t facadeCount = 0U;
  std::size_t partitionCount = 0U;
  const cr::CreativeWorldLayoutWall* northFacade = nullptr;
  for (const cr::CreativeWorldLayoutWall& wall : expanded.expanded.walls) {
    if (near(wall.baseLayer, 0.0) && wall.heightCells == 6U) {
      ++facadeCount;
      if (wall.start == cr::CreativeTerrainCoord2{0, 0} &&
          wall.end == cr::CreativeTerrainCoord2{8, 0}) {
        northFacade = &wall;
      }
    }
    if (wall.start == cr::CreativeTerrainCoord2{4, 0} &&
        wall.end == cr::CreativeTerrainCoord2{4, 4} &&
        wall.heightCells == 3U) {
      ++partitionCount;
    }
  }

  const cr::CreativeWorldLayoutOpening& lowerDoor =
      expanded.expanded.openings[0];
  const cr::CreativeWorldLayoutOpening& resolvedUpperWindow =
      expanded.expanded.openings[1];
  const cr::CreativeWorldLayoutWall& lowerDoorWall =
      expanded.expanded.walls[lowerDoor.wallIndex];
  const cr::CreativeWorldLayoutWall& upperWindowWall =
      expanded.expanded.walls[resolvedUpperWindow.wallIndex];
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Stacked Facade Windows");
  static_cast<void>(document.assignId(9207U));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);

  cr::CreativeWorldLayout independentWallHeight = layout;
  independentWallHeight.levels[0].wallHeightCells = 4U;
  const cr::CreativeWorldLayoutRoomCompileResult independentExpanded =
      cr::expandCreativeWorldLayoutRooms(independentWallHeight);
  std::size_t independentFacadeCount = 0U;
  if (independentExpanded.accepted) {
    independentFacadeCount = static_cast<std::size_t>(std::count_if(
        independentExpanded.expanded.walls.begin(),
        independentExpanded.expanded.walls.end(),
        [](const cr::CreativeWorldLayoutWall& wall) {
          return near(wall.baseLayer, 0.0) && wall.heightCells == 6U &&
                 wall.profile ==
                     cr::CreativeWorldLayoutWallProfile::Exterior;
        }));
  }
  const cr::CreativeWorldLayoutLevelDimensions lowerDimensions =
      cr::measureCreativeWorldLayoutLevelDimensions(
          document.gridSettings(), independentWallHeight, 0U);
  const cr::CreativeWorldLayoutCompileResult independentCompiled =
      cr::buildCreativeWorldLayoutPlan(document, independentWallHeight);
  const cr::CreativeWorldLayoutPreviewResult independentPreview =
      independentCompiled.receipt.accepted
          ? cr::previewCreativeWorldLayoutPlan(document,
                                               independentCompiled.plan)
          : cr::CreativeWorldLayoutPreviewResult{};
  std::size_t lowerPartitionPartCount = 0U;
  bool lowerPartitionEnvelopeMatches = independentPreview.accepted;
  for (const cr::CreativeObject& object :
       independentPreview.document.objects()) {
    if (object.kind != cr::CreativeObjectKind::Wall) {
      continue;
    }
    const cr::CreativeTransformedBounds geometry =
        cr::resolveCreativeObjectBounds(object);
    const double centerX =
        (geometry.worldBounds.min.x + geometry.worldBounds.max.x) * 0.5;
    if (!geometry.valid || !near(centerX, 4.0) ||
        geometry.size.x >= 0.5 ||
        geometry.worldBounds.min.y >=
            lowerDimensions.nextFloorTopMeters - 1.0e-9) {
      continue;
    }
    ++lowerPartitionPartCount;
    lowerPartitionEnvelopeMatches =
        lowerPartitionEnvelopeMatches &&
        near(geometry.worldBounds.max.y,
             lowerDimensions.interiorPartitionTopMeters);
  }

  cr::CreativeWorldLayout oversizedInteriorDoor = independentWallHeight;
  oversizedInteriorDoor.openings[0].cutoutHeightCells = 3.0;
  oversizedInteriorDoor.openings[0].insertHeightCells = 3.0;
  const cr::CreativeWorldLayoutCompileResult oversizedDoorCompiled =
      cr::buildCreativeWorldLayoutPlan(document, oversizedInteriorDoor);

  return expect(expanded.expanded.walls.size() == 6U && facadeCount == 4U,
                "four exterior runs span both storeys exactly once") &&
         expect(partitionCount == 2U &&
                    lowerDoorWall.heightCells == 3U &&
                    near(lowerDoorWall.baseLayer, 0.0),
                "interior partitions remain owned by their level") &&
         expect(northFacade != nullptr &&
                    &upperWindowWall == northFacade &&
                    near(resolvedUpperWindow.cutoutBottomCells, 4.0) &&
                    near(resolvedUpperWindow.insertBottomCells, 4.0),
                "default upper window inserts align to their rebased cutouts") &&
         expect(compiled.receipt.accepted,
                "stacked facades with default window inserts compile") &&
         expect(independentExpanded.accepted &&
                    independentFacadeCount == 4U,
                "exterior facades follow floor datums instead of authored partition height") &&
         expect(lowerDimensions.accepted &&
                    independentCompiled.receipt.accepted &&
                    independentPreview.accepted &&
                    lowerPartitionPartCount > 0U &&
                    lowerPartitionEnvelopeMatches,
                "generated lower partitions stop at the exact ceiling support plane") &&
         expect(!oversizedDoorCompiled.receipt.accepted &&
                    oversizedDoorCompiled.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::Opening &&
                    oversizedDoorCompiled.receipt.reasonCode ==
                        "creative_world_layout_opening_exceeds_wall_envelope" &&
                    oversizedDoorCompiled.receipt.kernelReasonCode ==
                        "creative_world_layout_opening_height_invalid",
                "interior openings cannot exceed the compiled clear-height envelope");
}

bool invalidTopologyFailsClosed() {
  cr::CreativeWorldLayout overlap = adjacentRooms();
  overlap.rooms[1].footprint = {{3, 1}, {7, 5}};
  const auto overlapResult = cr::expandCreativeWorldLayoutRooms(overlap);

  cr::CreativeWorldLayout noisyElevation = overlap;
  cr::CreativeWorldLayoutLevel noisyLevel = noisyElevation.levels[0];
  noisyLevel.stableKey = "level_noise";
  noisyLevel.name = "Noisy Level";
  noisyLevel.floorTopLayer = 5.0e-10;
  noisyElevation.levels.push_back(noisyLevel);
  noisyElevation.rooms[1].levelIndex = 1U;
  const auto noisyElevationResult =
      cr::expandCreativeWorldLayoutRooms(noisyElevation);

  cr::CreativeWorldLayout noisyAdjacent = adjacentRooms();
  noisyLevel = noisyAdjacent.levels[0];
  noisyLevel.stableKey = "level_noise";
  noisyLevel.name = "Noisy Level";
  noisyLevel.floorTopLayer = 5.0e-10;
  noisyAdjacent.levels.push_back(noisyLevel);
  noisyAdjacent.rooms[1].levelIndex = 1U;
  const auto noisyAdjacentResult =
      cr::expandCreativeWorldLayoutRooms(noisyAdjacent);

  cr::CreativeWorldLayout stacked = overlap;
  cr::CreativeWorldLayoutLevel stackedLevel = stacked.levels[0];
  stackedLevel.stableKey = "level_upper";
  stackedLevel.name = "Upper Level";
  stackedLevel.floorTopLayer = 3.0;
  stacked.levels.push_back(stackedLevel);
  stacked.rooms[1].levelIndex = 1U;
  const auto stackedResult = cr::expandCreativeWorldLayoutRooms(stacked);

  cr::CreativeWorldLayout badOpening = adjacentRooms();
  badOpening.openings[0].centerOffsetCells = 5.0;
  const auto openingResult = cr::expandCreativeWorldLayoutRooms(badOpening);

  cr::CreativeWorldLayout straddledOpening = adjacentRooms();
  straddledOpening.openings[0].centerOffsetCells = 3.75;
  const auto straddledResult =
      cr::expandCreativeWorldLayoutRooms(straddledOpening);

  cr::CreativeWorldLayout thickWalls = adjacentRooms();
  thickWalls.rooms[0].wallThicknessCells = 2.0;
  const auto thickWallResult =
      cr::expandCreativeWorldLayoutRooms(thickWalls);

  return expect(
             !overlapResult.accepted &&
                 overlapResult.status ==
                     cr::CreativeWorldLayoutRoomCompileStatus::OverlappingRooms,
             "interior-overlapping rooms are rejected") &&
         expect(!noisyElevationResult.accepted && stackedResult.accepted,
                "near-duplicate levels fail closed while distinct stacked rooms remain valid") &&
         expect(!noisyAdjacentResult.accepted,
                "near-duplicate story elevations cannot create ambiguous topology") &&
         expect(!openingResult.accepted &&
                    openingResult.status ==
                        cr::CreativeWorldLayoutRoomCompileStatus::
                            InvalidOpeningHost,
                "opening outside its semantic room edge is rejected") &&
         expect(!straddledResult.accepted &&
                    straddledResult.status ==
                        cr::CreativeWorldLayoutRoomCompileStatus::
                            InvalidOpeningHost,
                "opening width cannot straddle beyond its semantic edge") &&
         expect(!thickWallResult.accepted &&
                    thickWallResult.status ==
                        cr::CreativeWorldLayoutRoomCompileStatus::InvalidRoom,
                "room shell walls cannot consume the complete interior");
}

bool roomTopologyCompilesThroughExistingBuildingRecipe() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Room Layout");
  static_cast<void>(document.assignId(9201U));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, adjacentRooms());
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  const cr::CreativeObject* floor =
      findKind(preview.document, cr::CreativeObjectKind::Floor);
  const cr::CreativeObject* wall =
      findKind(preview.document, cr::CreativeObjectKind::Wall);
  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &preview.document;
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const auto bakedFloor = std::find_if(
      baked.room.spatialSurfaces.begin(), baked.room.spatialSurfaces.end(),
      [](const iggy3d::RoomSpatialSurface& surface) {
        return surface.role == iggy3d::RoomSpatialSurfaceRole::Walkable;
      });
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  const cr::CreativeTransformedBounds wallGeometry =
      wall == nullptr ? cr::CreativeTransformedBounds{}
                      : cr::resolveCreativeObjectBounds(*wall);
  const iggy3d::PhysicsAabbCollider* wallCollider =
      wall == nullptr
          ? nullptr
          : findCollider(physics, stableObjectId(wall->id) + "_actor_blocker");
  std::size_t floorCollider = physics.sourceSurfaceIds.size();
  if (bakedFloor != baked.room.spatialSurfaces.end()) {
    const auto source =
        std::find(physics.sourceSurfaceIds.begin(),
                  physics.sourceSurfaceIds.end(), bakedFloor->id);
    floorCollider = static_cast<std::size_t>(
        std::distance(physics.sourceSurfaceIds.begin(), source));
  }
  return expect(compiled.receipt.accepted &&
                    compiled.receipt.objectRecipeCount == 1U,
                "semantic rooms compile through one building recipe") &&
         expect(preview.accepted && preview.document.objectCount() ==
                                        compiled.receipt.objectCount,
                "exact preview materializes the compiled room shell") &&
         expect(floor != nullptr &&
                    near(cr::resolveCreativeObjectBounds(*floor).size.y, 0.05),
                "materialized room floor uses exact descriptor thickness") &&
         expect(wall != nullptr && near(wall->transform.position.y, 1.5),
                "three-cell room wall is centered above floor top") &&
         expect(wallGeometry.valid &&
                    near(wallGeometry.size.y,
                         cr::kDefaultCreativeWorldLayoutWallHeightCells) &&
                    near(std::min(wallGeometry.size.x, wallGeometry.size.z),
                         cr::kDefaultCreativeWorldLayoutWallThicknessCells),
                "room wall resolves from the cell-space wall policy") &&
         expect(baked.receipt.accepted &&
                    bakedFloor != baked.room.spatialSurfaces.end() &&
                    near(bakedFloor->collisionThicknessMeters, 0.05),
                "room bake preserves resolved floor thickness") &&
         expect(physics.ok && floorCollider < physics.colliders.size() &&
                    nearFloat(
                        physics.colliders[floorCollider].bounds.max.y -
                            physics.colliders[floorCollider].bounds.min.y,
                        0.05F),
                "physics collider consumes room-bake floor thickness") &&
         expect(wallCollider != nullptr &&
                    colliderMatchesBounds(*wallCollider,
                                          wallGeometry.worldBounds),
                "wall render and collision consume identical resolved bounds");
}

bool generatedRoomObjectsResolveToSemanticSources() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Room Provenance");
  static_cast<void>(document.assignId(9203U));
  const cr::CreativeWorldLayout source = adjacentRooms();
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, source);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  const cr::CreativeObject* floor = nullptr;
  const cr::CreativeObject* sharedWall = nullptr;
  const cr::CreativeObject* contiguousWall = nullptr;
  const cr::CreativeObject* door = nullptr;
  cr::CreativeWorldLayoutObjectProvenance shared;
  for (const cr::CreativeObject& object : preview.document.objects()) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(source, object);
    if (floor == nullptr && object.kind == cr::CreativeObjectKind::Floor) {
      floor = &object;
    }
    if (object.kind == cr::CreativeObjectKind::Wall &&
        provenance.contributorCount == 2U) {
      sharedWall = &object;
      shared = provenance;
      if (provenance.roomEdge == cr::CreativeWorldLayoutRoomEdge::North) {
        contiguousWall = &object;
      }
    }
    if (object.kind == cr::CreativeObjectKind::Door) {
      door = &object;
    }
  }
  const cr::CreativeWorldLayoutObjectProvenance floorSource =
      floor == nullptr
          ? cr::CreativeWorldLayoutObjectProvenance{}
          : cr::resolveCreativeWorldLayoutObjectProvenance(source, *floor);
  const cr::CreativeWorldLayoutObjectProvenance doorSource =
      door == nullptr
          ? cr::CreativeWorldLayoutObjectProvenance{}
          : cr::resolveCreativeWorldLayoutObjectProvenance(source, *door);
  const cr::CreativeWorldLayoutObjectProvenance leftNorthSource =
      contiguousWall == nullptr
          ? cr::CreativeWorldLayoutObjectProvenance{}
          : cr::resolveCreativeWorldLayoutObjectProvenance(
                source, *contiguousWall, {2.0, 0.0, 0.0});
  const cr::CreativeWorldLayoutObjectProvenance rightNorthSource =
      contiguousWall == nullptr
          ? cr::CreativeWorldLayoutObjectProvenance{}
          : cr::resolveCreativeWorldLayoutObjectProvenance(
                source, *contiguousWall, {6.0, 0.0, 0.0});
  return expect(compiled.receipt.accepted && preview.accepted,
                "room provenance preview accepted") &&
         expect(floor != nullptr && floorSource.owned &&
                    floorSource.table == cr::CreativeWorldLayoutTable::Room &&
                    floorSource.index == 0U &&
                    floorSource.contributorCount == 1U,
                "generated floor resolves to its authored room") &&
         expect(sharedWall != nullptr && shared.owned &&
                    shared.table == cr::CreativeWorldLayoutTable::Room &&
                    shared.index == 0U &&
                    shared.roomEdge == cr::CreativeWorldLayoutRoomEdge::East &&
                    shared.contributorCount == 2U,
                "shared wall retains both room-edge contributors") &&
         expect(contiguousWall != nullptr && leftNorthSource.owned &&
                    leftNorthSource.index == 0U &&
                    rightNorthSource.owned && rightNorthSource.index == 1U &&
                    rightNorthSource.roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::North &&
                    rightNorthSource.contributorCount == 2U,
                "point-aware provenance distinguishes condensed adjacent edges") &&
         expect(door != nullptr && doorSource.owned &&
                    doorSource.table ==
                        cr::CreativeWorldLayoutTable::Opening &&
                    doorSource.index == 0U,
                "generated door resolves to its authored opening");
}

bool generatedWallHitResolvesExactCanonicalEdge() {
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(adjacentRooms());
  if (!materialized.accepted) {
    return expect(false, "explicit provenance fixture materializes");
  }
  const cr::CreativeWorldLayout& source = materialized.edited;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Exact Wall Provenance");
  static_cast<void>(document.assignId(9213U));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, source);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);

  const cr::CreativeObject* condensed = nullptr;
  std::vector<std::size_t> contributors;
  for (const cr::CreativeObject& object : preview.document.objects()) {
    if (object.kind != cr::CreativeObjectKind::Wall) {
      continue;
    }
    std::vector<std::size_t> objectContributors;
    for (std::size_t edgeIndex = 0U;
         edgeIndex < source.topologyEdges.size(); ++edgeIndex) {
      const std::string tag = cr::creativeWorldLayoutProvenanceTag(
          source, cr::CreativeWorldLayoutTable::TopologyEdge, edgeIndex);
      if (!tag.empty() &&
          std::find(object.tags.begin(), object.tags.end(), tag) !=
              object.tags.end()) {
        objectContributors.push_back(edgeIndex);
      }
    }
    if (objectContributors.size() >= 2U) {
      condensed = &object;
      contributors = std::move(objectContributors);
      break;
    }
  }
  if (condensed == nullptr) {
    return expect(false, "explicit adjacent walls condense for provenance");
  }

  const auto midpoint = [&](std::size_t edgeIndex) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        source.topologyEdges[edgeIndex];
    const cr::CreativeTerrainCoord2 start =
        source.topologyVertices[edge.startVertexIndex].position;
    const cr::CreativeTerrainCoord2 end =
        source.topologyVertices[edge.endVertexIndex].position;
    const cr::CreativeWorldLayoutLevel& level = source.levels[edge.levelIndex];
    return cr::CreativeVec3{
        (static_cast<double>(start.x) + end.x) * 0.5,
        level.floorTopLayer + 0.5,
        (static_cast<double>(start.z) + end.z) * 0.5};
  };
  const auto first = cr::resolveCreativeWorldLayoutObjectProvenance(
      source, *condensed, midpoint(contributors[0]));
  const auto second = cr::resolveCreativeWorldLayoutObjectProvenance(
      source, *condensed, midpoint(contributors[1]));
  const auto fallback =
      cr::resolveCreativeWorldLayoutObjectProvenance(source, *condensed);
  return expect(compiled.receipt.accepted && preview.accepted,
                "explicit topology provenance preview accepted") &&
         expect(first.owned && second.owned &&
                    first.table ==
                        cr::CreativeWorldLayoutTable::TopologyEdge &&
                    second.table ==
                        cr::CreativeWorldLayoutTable::TopologyEdge &&
                    first.index == contributors[0] &&
                    second.index == contributors[1] &&
                    first.contributorCount == contributors.size() &&
                    second.contributorCount == contributors.size(),
                "3D hit position selects the exact condensed wall segment") &&
         expect(fallback.owned && fallback.index == contributors.front() &&
                    fallback.contributorCount == contributors.size(),
                "point-free wall provenance remains deterministic");
}

bool horizontalStructuralLayersUseDescriptorThickness() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Layer Layout");
  static_cast<void>(document.assignId(9202U));
  cr::CreativeWorldLayout layout;
  layout.stableKey = "structural_layers";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "layers";
  building.name = "Structural Layers";
  layout.buildings.push_back(building);
  layout.boxes = {
      {0U, cr::CreativeObjectKind::Floor, "floor", "Floor",
       {{0, 0}, {2, 2}}, 0, 1U},
      {0U, cr::CreativeObjectKind::Ceiling, "ceiling", "Ceiling",
       {{3, 0}, {5, 2}}, 3, 1U},
      {0U, cr::CreativeObjectKind::Roof, "roof", "Roof",
       {{6, 0}, {8, 2}}, 4, 1U},
  };
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  const cr::CreativeObject* floor =
      findKind(preview.document, cr::CreativeObjectKind::Floor);
  const cr::CreativeObject* ceiling =
      findKind(preview.document, cr::CreativeObjectKind::Ceiling);
  const cr::CreativeObject* roof =
      findKind(preview.document, cr::CreativeObjectKind::Roof);

  cr::CreativeRoomBakeRequest request;
  request.document = &preview.document;
  request.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(request);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});

  bool parity = compiled.receipt.accepted && preview.accepted &&
                baked.receipt.accepted && physics.ok;
  for (const cr::CreativeObject* object : {floor, ceiling, roof}) {
    if (object == nullptr) {
      parity = false;
      continue;
    }
    const cr::CreativeTransformedBounds geometry =
        cr::resolveCreativeObjectBounds(*object);
    const iggy3d::RoomStaticMeshAsset* mesh = findMesh(baked.room, object->id);
    const iggy3d::PhysicsAabbCollider* collider = findCollider(
        physics, stableObjectId(object->id) + "_walkable");
    parity = parity && geometry.valid && mesh != nullptr && collider != nullptr &&
             near(geometry.size.y,
                  cr::defaultCreativeStructuralLayerThicknessMeters(
                      object->kind)) &&
             nearFloat(mesh->sizeMeters.x,
                       static_cast<float>(geometry.size.x)) &&
             nearFloat(mesh->sizeMeters.y,
                       static_cast<float>(geometry.size.y)) &&
             nearFloat(mesh->sizeMeters.z,
                       static_cast<float>(geometry.size.z)) &&
             colliderMatchesBounds(*collider, geometry.worldBounds);
  }
  return expect(parity,
                "floor ceiling and roof share descriptor-to-render-to-collision geometry");
}

bool architecturalDimensionsOwnCompilerAndOpeningScale() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Architectural Dimensions");
  static_cast<void>(document.assignId(9210U));
  static_cast<void>(document.setGridSettings(
      {{10.0, 2.0, -4.0}, 0.5, {64, 64, 64}}));

  cr::CreativeWorldLayout layout;
  layout.stableKey = "architectural_dimensions";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "Measured House";
  layout.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel ground;
  ground.buildingIndex = 0U;
  ground.stableKey = "ground";
  ground.name = "Ground";
  ground.floorTopLayer = 1.0;
  ground.wallHeightCells = 6U;
  ground.floorThicknessLayers = 2U;
  ground.ceilingThicknessLayers = 2U;
  ground.roofThicknessLayers = 2U;
  layout.levels.push_back(ground);
  cr::CreativeWorldLayoutLevel upper = ground;
  upper.stableKey = "upper";
  upper.name = "Upper";
  upper.floorTopLayer = 7.0;
  layout.levels.push_back(upper);

  cr::CreativeWorldLayoutRoom lowerRoom;
  lowerRoom.buildingIndex = 0U;
  lowerRoom.levelIndex = 0U;
  lowerRoom.stableKey = "lower_room";
  lowerRoom.name = "Lower Room";
  lowerRoom.footprint = {{0, 0}, {8, 6}};
  layout.rooms.push_back(lowerRoom);
  cr::CreativeWorldLayoutRoom upperRoom = lowerRoom;
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "upper_room";
  upperRoom.name = "Upper Room";
  layout.rooms.push_back(upperRoom);

  cr::CreativeWorldLayoutOpening window;
  window.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  window.roomIndex = 1U;
  window.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  window.kind = cr::CreativeBuildingOpeningKind::Window;
  window.stableKey = "upper_window";
  window.name = "Upper Window";
  window.centerOffsetCells = 4.0;
  window.widthCells = 2.0;
  window.cutoutBottomCells = 1.0;
  window.cutoutHeightCells = 2.2;
  window.insertBottomCells = 0.0;
  window.insertHeightCells = 2.0;
  window.insertWidthCells = 1.5;
  window.insertThicknessCells = 0.25;
  layout.openings.push_back(window);

  const cr::CreativeWorldLayoutLevelDimensions groundDimensions =
      cr::measureCreativeWorldLayoutLevelDimensions(
          document.gridSettings(), layout, 0U);
  const cr::CreativeWorldLayoutBuildingDimensions buildingDimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          document.gridSettings(), layout, 0U);
  const cr::CreativeWorldLayoutOpeningDimensions sourceOpeningDimensions =
      cr::measureCreativeWorldLayoutOpeningDimensions(
          document.gridSettings(), layout, 0U);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(layout);
  const cr::CreativeWorldLayoutOpeningDimensions expandedOpeningDimensions =
      cr::measureCreativeWorldLayoutOpeningDimensions(
          document.gridSettings(), expanded.expanded, 0U);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);

  const cr::CreativeObject* roof =
      findKind(preview.document, cr::CreativeObjectKind::Roof);
  const cr::CreativeObject* generatedWindow =
      findKind(preview.document, cr::CreativeObjectKind::Window);
  const cr::CreativeTransformedBounds roofBounds =
      roof == nullptr ? cr::CreativeTransformedBounds{}
                      : cr::resolveCreativeObjectBounds(*roof);
  const cr::CreativeTransformedBounds windowBounds =
      generatedWindow == nullptr
          ? cr::CreativeTransformedBounds{}
          : cr::resolveCreativeObjectBounds(*generatedWindow);

  cr::CreativeGridSettings invalidGrid = document.gridSettings();
  invalidGrid.cellSizeMeters = 0.0;
  const cr::CreativeWorldLayoutBuildingDimensions invalid =
      cr::measureCreativeWorldLayoutBuildingDimensions(invalidGrid, layout,
                                                       0U);
  cr::CreativeWorldLayout nonuniform = layout;
  nonuniform.levels[1].wallHeightCells = 8U;
  const cr::CreativeWorldLayoutBuildingDimensions varied =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          document.gridSettings(), nonuniform, 0U);

  return expect(groundDimensions.accepted &&
                    near(groundDimensions.floorBottomMeters, 2.4) &&
                    near(groundDimensions.floorTopMeters, 2.5) &&
                    near(groundDimensions.wallTopMeters, 5.5) &&
                    groundDimensions.hasUpperLevel &&
                    groundDimensions.upperLevelIndex == 1U &&
                    near(groundDimensions.nextFloorBottomMeters, 5.4) &&
                    near(groundDimensions.nextFloorTopMeters, 5.5) &&
                    near(groundDimensions.floorToFloorMeters, 3.0) &&
                    near(groundDimensions.clearHeightMeters, 2.4) &&
                    near(groundDimensions.interiorPartitionTopMeters, 4.9) &&
                    near(groundDimensions.exteriorFacadeTopMeters, 5.5) &&
                    near(groundDimensions.exteriorFacadeHeightMeters, 3.0) &&
                    near(groundDimensions.upperSurfaceSupportMeters, 4.9) &&
                    near(groundDimensions.upperSurfaceTopMeters, 5.4),
                "level dimensions separate partitions, facades, datum spacing, and clear height") &&
         expect(buildingDimensions.accepted &&
                    buildingDimensions.occupiedLevelCount == 2U &&
                    near(buildingDimensions.footprintMinimumXMeters, 10.0) &&
                    near(buildingDimensions.footprintMaximumXMeters, 14.0) &&
                    near(buildingDimensions.footprintMinimumZMeters, -4.0) &&
                    near(buildingDimensions.footprintMaximumZMeters, -1.0) &&
                    near(buildingDimensions.minimumFloorToFloorMeters, 3.0) &&
                    near(buildingDimensions.exteriorFacadeHeightMeters, 6.0) &&
                    near(buildingDimensions.roofBaseMeters, 8.5) &&
                    near(buildingDimensions.roofTopMeters, 9.0) &&
                    near(buildingDimensions.totalHeightMeters, 6.6) &&
                    buildingDimensions.uniformFloorToFloor &&
                    buildingDimensions.uniformWallHeight &&
                    buildingDimensions.uniformFloorThickness,
                "building dimensions report one exact architectural scale") &&
         expect(sourceOpeningDimensions.accepted &&
                    expandedOpeningDimensions.accepted &&
                    near(sourceOpeningDimensions.cutoutBottomMeters, 6.0) &&
                    near(sourceOpeningDimensions.insertBottomMeters, 6.0) &&
                    near(sourceOpeningDimensions.cutoutTopMeters, 7.1) &&
                    near(expandedOpeningDimensions.cutoutBottomMeters,
                         sourceOpeningDimensions.cutoutBottomMeters) &&
                    near(expandedOpeningDimensions.insertBottomMeters,
                         sourceOpeningDimensions.insertBottomMeters),
                "source and facade-normalized openings retain world elevation") &&
         expect(compiled.receipt.accepted && preview.accepted &&
                    roofBounds.valid && windowBounds.valid &&
                    near(roofBounds.worldBounds.min.y,
                         buildingDimensions.roofBaseMeters) &&
                    near(roofBounds.worldBounds.max.y,
                         buildingDimensions.roofTopMeters) &&
                    near(windowBounds.worldBounds.min.y, 6.06),
                "compiler geometry consumes measured roof dimensions and the inset window recipe") &&
         expect(!invalid.accepted &&
                    invalid.status ==
                        cr::CreativeWorldLayoutDimensionStatus::InvalidGrid,
                "invalid grid dimensions fail closed") &&
         expect(varied.accepted && !varied.uniformWallHeight &&
                    near(varied.minimumWallHeightMeters, 3.0) &&
                    near(varied.maximumWallHeightMeters, 4.0),
                "nonuniform storeys remain measurable and explicit");
}

bool occupiedLevelsGenerateCeilingsAndOneTopRoof() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Level Shell");
  static_cast<void>(document.assignId(9204U));
  cr::CreativeWorldLayout layout = adjacentRooms();
  layout.openings.clear();
  layout.rooms.resize(1U);
  layout.levels[0].ceilingThicknessLayers = 2U;

  cr::CreativeWorldLayoutLevel upperLevel = layout.levels[0];
  upperLevel.stableKey = "level_upper";
  upperLevel.name = "Upper Level";
  upperLevel.floorTopLayer = 3.0;
  layout.levels.push_back(upperLevel);
  const cr::CreativeWorldLayoutCompileResult emptyUpperCompiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult emptyUpperPreview =
      cr::previewCreativeWorldLayoutPlan(document, emptyUpperCompiled.plan);
  const auto emptyUpperCount =
      [&emptyUpperPreview](cr::CreativeObjectKind kind) {
    return static_cast<std::size_t>(std::count_if(
        emptyUpperPreview.document.objects().begin(),
        emptyUpperPreview.document.objects().end(),
        [kind](const cr::CreativeObject& object) {
          return object.kind == kind;
        }));
  };
  cr::CreativeWorldLayoutRoom upperRoom = layout.rooms[0];
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "room_upper";
  upperRoom.name = "Upper Room";
  layout.rooms.push_back(upperRoom);

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  const auto countKind = [&preview](cr::CreativeObjectKind kind) {
    return static_cast<std::size_t>(std::count_if(
        preview.document.objects().begin(), preview.document.objects().end(),
        [kind](const cr::CreativeObject& object) {
          return object.kind == kind;
        }));
  };
  const auto findName = [&preview](std::string_view name) {
    const auto found = std::find_if(
        preview.document.objects().begin(), preview.document.objects().end(),
        [name](const cr::CreativeObject& object) {
          return object.name == name;
        });
    return found == preview.document.objects().end() ? nullptr : &*found;
  };
  const cr::CreativeObject* lowerCeiling = findName("Left Room Ceiling");
  const cr::CreativeObject* upperFloor = findName("Upper Room Floor");
  const cr::CreativeObject* upperRoof = findName("Upper Room Roof");
  const cr::CreativeTransformedBounds lowerCeilingBounds =
      lowerCeiling == nullptr ? cr::CreativeTransformedBounds{}
                              : cr::resolveCreativeObjectBounds(*lowerCeiling);
  const cr::CreativeTransformedBounds upperFloorBounds =
      upperFloor == nullptr ? cr::CreativeTransformedBounds{}
                            : cr::resolveCreativeObjectBounds(*upperFloor);
  const cr::CreativeTransformedBounds upperRoofBounds =
      upperRoof == nullptr ? cr::CreativeTransformedBounds{}
                           : cr::resolveCreativeObjectBounds(*upperRoof);

  return expect(compiled.receipt.accepted && preview.accepted,
                "two-level room shell compiles") &&
         expect(emptyUpperCompiled.receipt.accepted &&
                    emptyUpperPreview.accepted &&
                    emptyUpperCount(cr::CreativeObjectKind::Roof) == 1U &&
                    emptyUpperCount(cr::CreativeObjectKind::Ceiling) == 0U,
                "an empty upper level does not replace the occupied roof") &&
         expect(countKind(cr::CreativeObjectKind::Floor) == 2U &&
                    countKind(cr::CreativeObjectKind::Ceiling) == 1U &&
                    countKind(cr::CreativeObjectKind::Roof) == 1U,
                "lower occupied level gets a ceiling and top level gets one roof") &&
         expect(lowerCeilingBounds.valid && upperFloorBounds.valid &&
                    near(lowerCeilingBounds.worldBounds.max.y,
                         upperFloorBounds.worldBounds.min.y),
                "lower ceiling finishes against the underside of the upper floor") &&
         expect(upperRoofBounds.valid &&
                    near(upperRoofBounds.worldBounds.min.y, 6.0),
                "roof begins at the upper wall support plane");
}

bool authoredGableRoofCompilesThroughSharedRenderCollisionGeometry() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Gable Roof");
  static_cast<void>(document.assignId(9205U));
  cr::CreativeWorldLayout layout = adjacentRooms();
  layout.openings.clear();
  cr::CreativeWorldLayoutLevel& level = layout.levels[0];
  level.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  level.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  level.roofPitchDegrees = 45.0;
  level.roofOverhangCells = 1.0;

  const cr::CreativeWorldLayoutRoofPlan roofPlan =
      cr::planCreativeWorldLayoutRoof(document.gridSettings(), layout, 0U);
  const cr::CreativeWorldLayoutBuildingDimensions dimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          document.gridSettings(), layout, 0U);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  std::vector<const cr::CreativeObject*> slopes;
  for (const cr::CreativeObject& object : preview.document.objects()) {
    if (object.kind == cr::CreativeObjectKind::RoofSlope) {
      slopes.push_back(&object);
    }
  }

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &preview.document;
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::CollisionQueryResult north =
      iggy3d::sampleSurfaceHeight(surfaces, {2.0F, 0.0F, 0.5F});
  const iggy3d::CollisionQueryResult ridge =
      iggy3d::sampleSurfaceHeight(surfaces, {2.0F, 0.0F, 2.0F});
  const iggy3d::CollisionQueryResult south =
      iggy3d::sampleSurfaceHeight(surfaces, {2.0F, 0.0F, 3.5F});
  const iggy3d::RoomStaticMeshAsset* firstMesh =
      slopes.size() > 0U ? findMesh(baked.room, slopes[0]->id) : nullptr;
  const iggy3d::RoomStaticMeshAsset* secondMesh =
      slopes.size() > 1U ? findMesh(baked.room, slopes[1]->id) : nullptr;

  const cr::CreativeWorldLayoutBuildingTransformResult rotated =
      cr::transformCreativeWorldLayoutBuilding(
          layout,
          {0U, cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});

  cr::CreativeWorldLayout irregular = layout;
  irregular.rooms[1].footprint.maximum.z = 2;
  const cr::CreativeWorldLayoutCompileResult rejected =
      cr::buildCreativeWorldLayoutPlan(document, irregular);

  return expect(roofPlan.accepted && roofPlan.geometry.partCount == 2U &&
                    roofPlan.footprint.minimum ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    roofPlan.footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 4} &&
                    near(roofPlan.geometry.riseMeters, 3.0),
                "adjacent rooms resolve one pitched roof footprint") &&
         expect(dimensions.accepted &&
                    near(dimensions.roofBaseMeters, 3.0) &&
                    near(dimensions.roofTopMeters, 6.0) &&
                    near(dimensions.totalHeightMeters, 6.05),
                "building dimensions include the pitched roof envelope") &&
         expect(compiled.receipt.accepted && preview.accepted &&
                    slopes.size() == 2U,
                "gable layout emits exactly two semantic slope panels") &&
         expect(baked.receipt.accepted && firstMesh != nullptr &&
                    secondMesh != nullptr &&
                    firstMesh->meshId == "creative_solid_prism" &&
                    secondMesh->meshId == "creative_solid_prism",
                "gable slopes render as thin oriented panels") &&
         expect(north.status == iggy3d::CollisionQueryStatus::Hit &&
                    ridge.status == iggy3d::CollisionQueryStatus::Hit &&
                    south.status == iggy3d::CollisionQueryStatus::Hit &&
                    nearFloat(north.heightMeters, 4.5F) &&
                    nearFloat(ridge.heightMeters, 6.0F) &&
                    nearFloat(south.heightMeters, 4.5F),
                "visible gable weather faces expose continuous pitched collision") &&
         expect(rotated.accepted &&
                    rotated.transformed.levels[0].roofRidgeAxis ==
                        cr::CreativeStructuralRoofRidgeAxis::Z,
                "building rotation swaps the authored roof ridge axis") &&
         expect(!rejected.receipt.accepted &&
                    rejected.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::Level &&
                    rejected.receipt.failedIndex == 0U,
                "non-rectangular gable footprint fails closed at its level");
}

bool authoredRoofStylesKeepStableGeneratedOwnership() {
  struct RoofCase {
    cr::CreativeStructuralRoofStyle style;
    cr::CreativeStructuralRoofRidgeAxis ridgeAxis;
    cr::CreativeStructuralRoofSlopeDirection slopeDirection;
    std::array<cr::CreativeObjectKind, 4U> kinds;
    std::array<std::string_view, 4U> stableKeys;
    std::size_t count;
  };
  const std::array<RoofCase, 4U> cases{{
      {cr::CreativeStructuralRoofStyle::Flat,
       cr::CreativeStructuralRoofRidgeAxis::X,
       cr::CreativeStructuralRoofSlopeDirection::PositiveZ,
       {cr::CreativeObjectKind::Roof, cr::CreativeObjectKind::Unknown,
        cr::CreativeObjectKind::Unknown, cr::CreativeObjectKind::Unknown},
       {"building_1.level_ground.roof.flat", {}, {}, {}}, 1U},
      {cr::CreativeStructuralRoofStyle::Shed,
       cr::CreativeStructuralRoofRidgeAxis::X,
       cr::CreativeStructuralRoofSlopeDirection::PositiveX,
       {cr::CreativeObjectKind::RoofSlope,
        cr::CreativeObjectKind::Unknown, cr::CreativeObjectKind::Unknown,
        cr::CreativeObjectKind::Unknown},
       {"building_1.level_ground.roof.shed", {}, {}, {}}, 1U},
      {cr::CreativeStructuralRoofStyle::Gable,
       cr::CreativeStructuralRoofRidgeAxis::X,
       cr::CreativeStructuralRoofSlopeDirection::PositiveZ,
       {cr::CreativeObjectKind::RoofSlope,
        cr::CreativeObjectKind::RoofSlope,
        cr::CreativeObjectKind::Unknown, cr::CreativeObjectKind::Unknown},
       {"building_1.level_ground.roof.gable.first",
        "building_1.level_ground.roof.gable.second", {}, {}}, 2U},
      {cr::CreativeStructuralRoofStyle::Hip,
       cr::CreativeStructuralRoofRidgeAxis::X,
       cr::CreativeStructuralRoofSlopeDirection::PositiveZ,
       {cr::CreativeObjectKind::HipRoof, cr::CreativeObjectKind::HipRoof,
        cr::CreativeObjectKind::HipRoof, cr::CreativeObjectKind::HipRoof},
       {"building_1.level_ground.roof.hip.north",
        "building_1.level_ground.roof.hip.south",
        "building_1.level_ground.roof.hip.west",
        "building_1.level_ground.roof.hip.east"}, 4U},
  }};

  bool allReady = true;
  for (const RoofCase& roofCase : cases) {
    cr::CreativeDocument document =
        cr::CreativeDocument::create("Roof Ownership");
    static_cast<void>(document.assignId(9220U));
    cr::CreativeWorldLayout layout = adjacentRooms();
    layout.openings.clear();
    cr::CreativeWorldLayoutLevel& level = layout.levels[0];
    level.roofStyle = roofCase.style;
    level.roofRidgeAxis = roofCase.ridgeAxis;
    level.roofSlopeDirection = roofCase.slopeDirection;
    level.roofPitchDegrees = 35.0;
    // A non-zero authored overhang selects the canonical rectangular roof
    // owner for Flat while preserving exact per-room surfaces by default.
    level.roofOverhangCells = 0.5;
    level.roofMaterial = cr::CreativeStructuralMaterial::Stone;

    const cr::CreativeWorldLayoutCompileResult firstCompiled =
        cr::buildCreativeWorldLayoutPlan(document, layout);
    const cr::CreativeWorldLayoutCompileResult secondCompiled =
        cr::buildCreativeWorldLayoutPlan(document, layout);
    const cr::CreativeWorldLayoutPreviewResult firstPreview =
        cr::previewCreativeWorldLayoutPlan(document, firstCompiled.plan);
    const cr::CreativeWorldLayoutPreviewResult secondPreview =
        cr::previewCreativeWorldLayoutPlan(document, secondCompiled.plan);

    std::vector<const cr::CreativeObject*> firstRoofs;
    std::vector<const cr::CreativeObject*> secondRoofs;
    const auto collectRoofs = [](const cr::CreativeDocument& source,
                                 std::vector<const cr::CreativeObject*>& out) {
      for (const cr::CreativeObject& object : source.objects()) {
        if (object.kind == cr::CreativeObjectKind::Roof ||
            object.kind == cr::CreativeObjectKind::RoofSlope ||
            object.kind == cr::CreativeObjectKind::HipRoof) {
          out.push_back(&object);
        }
      }
    };
    collectRoofs(firstPreview.document, firstRoofs);
    collectRoofs(secondPreview.document, secondRoofs);

    bool caseReady = firstCompiled.receipt.accepted &&
                     secondCompiled.receipt.accepted &&
                     firstPreview.accepted && secondPreview.accepted &&
                     firstRoofs.size() == roofCase.count &&
                     secondRoofs.size() == roofCase.count;
    for (std::size_t index = 0U;
         caseReady && index < roofCase.count; ++index) {
      cr::CreativeStructuralMaterial material =
          cr::CreativeStructuralMaterial::Count;
      const cr::CreativeWorldLayoutObjectProvenance provenance =
          cr::resolveCreativeWorldLayoutObjectProvenance(
              layout, *firstRoofs[index]);
      caseReady = firstRoofs[index]->kind == roofCase.kinds[index] &&
                  secondRoofs[index]->kind == roofCase.kinds[index] &&
                  cr::creativeRecipeObjectStableKey(*firstRoofs[index]) ==
                      roofCase.stableKeys[index] &&
                  cr::creativeRecipeObjectStableKey(*secondRoofs[index]) ==
                      roofCase.stableKeys[index] &&
                  firstRoofs[index]->id == secondRoofs[index]->id &&
                  provenance.owned &&
                  provenance.table == cr::CreativeWorldLayoutTable::Level &&
                  provenance.index == 0U &&
                  cr::parseCreativeStructuralMaterialTag(
                      firstRoofs[index]->tags, material) &&
                  material == cr::CreativeStructuralMaterial::Stone;
    }
    if (!caseReady) {
      std::cerr << "Roof ownership case failed: "
                << cr::toString(roofCase.style) << '\n';
    }
    allReady = allReady && caseReady;
  }
  return expect(allReady,
                "all authored roof styles keep stable level-owned children and material tags");
}

bool authoredRoofAperturesCompileWithStableOwnershipAndCollisionHoles() {
  struct RoofCase {
    cr::CreativeStructuralRoofStyle style;
    cr::CreativeStructuralRoofSlopeDirection slopeDirection;
  };
  const std::array<RoofCase, 3U> cases{{
      {cr::CreativeStructuralRoofStyle::Flat,
       cr::CreativeStructuralRoofSlopeDirection::PositiveZ},
      {cr::CreativeStructuralRoofStyle::Shed,
       cr::CreativeStructuralRoofSlopeDirection::PositiveZ},
      {cr::CreativeStructuralRoofStyle::Gable,
       cr::CreativeStructuralRoofSlopeDirection::PositiveZ},
  }};

  bool allReady = true;
  for (const RoofCase& roofCase : cases) {
    cr::CreativeDocument document =
        cr::CreativeDocument::create("Roof Apertures");
    static_cast<void>(document.assignId(9230U));
    cr::CreativeWorldLayout layout = adjacentRooms();
    layout.openings.clear();
    layout.levels[0].roofStyle = roofCase.style;
    layout.levels[0].roofRidgeAxis =
        cr::CreativeStructuralRoofRidgeAxis::X;
    layout.levels[0].roofSlopeDirection = roofCase.slopeDirection;
    layout.levels[0].roofPitchDegrees = 30.0;
    layout.levels[0].roofOverhangCells = 0.5;
    layout.roofApertures.push_back(
        {0U, cr::CreativeStructuralRoofApertureKind::Skylight,
         "skylight.test", "Test Skylight", 1.0, 2.0, 0.5, 1.25});
    layout.roofApertures.push_back(
        {0U, cr::CreativeStructuralRoofApertureKind::ChimneyClearance,
         "chimney.test", "Test Chimney Clearance", 4.5, 5.5, 0.5, 1.25});

    const cr::CreativeWorldLayoutRoofPlan roofPlan =
        cr::planCreativeWorldLayoutRoof(document.gridSettings(), layout, 0U);
    const cr::CreativeWorldLayoutCompileResult firstCompiled =
        cr::buildCreativeWorldLayoutPlan(document, layout);
    const cr::CreativeWorldLayoutCompileResult secondCompiled =
        cr::buildCreativeWorldLayoutPlan(document, layout);
    const cr::CreativeWorldLayoutPreviewResult firstPreview =
        cr::previewCreativeWorldLayoutPlan(document, firstCompiled.plan);
    const cr::CreativeWorldLayoutPreviewResult secondPreview =
        cr::previewCreativeWorldLayoutPlan(document, secondCompiled.plan);

    std::vector<const cr::CreativeObject*> firstRoofPieces;
    std::vector<const cr::CreativeObject*> secondRoofPieces;
    const cr::CreativeObject* skylight = nullptr;
    for (const cr::CreativeObject& object : firstPreview.document.objects()) {
      if (object.kind == cr::CreativeObjectKind::Roof ||
          object.kind == cr::CreativeObjectKind::RoofSlope) {
        firstRoofPieces.push_back(&object);
      } else if (object.kind == cr::CreativeObjectKind::Window) {
        skylight = &object;
      }
    }
    for (const cr::CreativeObject& object : secondPreview.document.objects()) {
      if (object.kind == cr::CreativeObjectKind::Roof ||
          object.kind == cr::CreativeObjectKind::RoofSlope) {
        secondRoofPieces.push_back(&object);
      }
    }

    bool stablePieces = firstRoofPieces.size() == secondRoofPieces.size();
    for (std::size_t index = 0U;
         stablePieces && index < firstRoofPieces.size(); ++index) {
      stablePieces =
          cr::creativeRecipeObjectStableKey(*firstRoofPieces[index]) ==
              cr::creativeRecipeObjectStableKey(*secondRoofPieces[index]) &&
          firstRoofPieces[index]->id == secondRoofPieces[index]->id;
    }
    const cr::CreativeWorldLayoutObjectProvenance skylightProvenance =
        skylight == nullptr
            ? cr::CreativeWorldLayoutObjectProvenance{}
            : cr::resolveCreativeWorldLayoutObjectProvenance(layout,
                                                              *skylight);

    cr::CreativeRoomBakeRequest bakeRequest;
    bakeRequest.document = &firstPreview.document;
    bakeRequest.validateReachability = false;
    const cr::CreativeRoomBakeResult baked =
        cr::buildRoomAssetFromCreativeDocument(bakeRequest);
    const iggy3d::SpatialSurfaceSet surfaces =
        iggy3d::buildSpatialSurfaceSet(baked.room);
    const iggy3d::CollisionQueryResult chimneySample =
        iggy3d::sampleSurfaceHeight(surfaces, {5.0F, 0.0F, 0.875F});

    const bool caseReady =
        roofPlan.accepted && roofPlan.sourceApertureCount == 2U &&
        roofPlan.closure.insertCount == 1U &&
        roofPlan.closure.pieceCount > roofPlan.geometry.partCount &&
        firstCompiled.receipt.accepted && secondCompiled.receipt.accepted &&
        firstPreview.accepted && secondPreview.accepted && stablePieces &&
        firstRoofPieces.size() == roofPlan.closure.pieceCount &&
        skylight != nullptr &&
        cr::creativeRecipeObjectStableKey(*skylight) ==
            "building_1.skylight.test.insert" &&
        skylightProvenance.owned &&
        skylightProvenance.table ==
            cr::CreativeWorldLayoutTable::RoofAperture &&
        skylightProvenance.index == 0U && baked.receipt.accepted &&
        chimneySample.status == iggy3d::CollisionQueryStatus::Hit &&
        chimneySample.heightMeters < 2.9F;
    if (!caseReady) {
      std::cerr << "Roof aperture case failed: "
                << cr::toString(roofCase.style) << '\n';
    }
    allReady = allReady && caseReady;
  }

  cr::CreativeDocument document =
      cr::CreativeDocument::create("Rejected Roof Apertures");
  static_cast<void>(document.assignId(9231U));
  cr::CreativeWorldLayout ridge = adjacentRooms();
  ridge.openings.clear();
  ridge.levels[0].roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  ridge.levels[0].roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  ridge.roofApertures.push_back(
      {0U, cr::CreativeStructuralRoofApertureKind::Skylight,
       "skylight.ridge", "Ridge Skylight", 2.0, 3.0, 1.5, 2.5});
  const cr::CreativeWorldLayoutCompileResult ridgeRejected =
      cr::buildCreativeWorldLayoutPlan(document, ridge);
  cr::CreativeWorldLayout hip = ridge;
  hip.levels[0].roofStyle = cr::CreativeStructuralRoofStyle::Hip;
  hip.roofApertures[0].minimumZCells = 0.5;
  hip.roofApertures[0].maximumZCells = 1.25;
  const cr::CreativeWorldLayoutCompileResult hipRejected =
      cr::buildCreativeWorldLayoutPlan(document, hip);

  if (ridgeRejected.receipt.accepted ||
      ridgeRejected.receipt.failedTable !=
          cr::CreativeWorldLayoutTable::RoofAperture ||
      ridgeRejected.receipt.failedIndex != 0U) {
    std::cerr << "Ridge rejection: accepted="
              << ridgeRejected.receipt.accepted << " table="
              << cr::toString(ridgeRejected.receipt.failedTable)
              << " index=" << ridgeRejected.receipt.failedIndex
              << " status=" << cr::toString(ridgeRejected.receipt.status)
              << " reason=" << ridgeRejected.receipt.reasonCode
              << " kernel=" << ridgeRejected.receipt.kernelReasonCode
              << '\n';
  }

  return expect(allReady,
                "flat shed and gable apertures compile with stable source ownership and collision holes") &&
         expect(!ridgeRejected.receipt.accepted &&
                    ridgeRejected.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::RoofAperture &&
                    ridgeRejected.receipt.failedIndex == 0U,
                "ridge-crossing aperture rejects at its durable source") &&
         expect(!hipRejected.receipt.accepted &&
                    hipRejected.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::RoofAperture &&
                    hipRejected.receipt.failedIndex == 0U,
                "hip aperture deferral rejects at its durable source");
}

bool flatRoofOverhangUsesTheSharedLevelFootprint() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Flat Overhang");
  static_cast<void>(document.assignId(9206U));
  cr::CreativeWorldLayout layout = adjacentRooms();
  layout.openings.clear();
  layout.levels[0].roofOverhangCells = 1.0;

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  const cr::CreativeObject* roof = nullptr;
  std::size_t roofCount = 0U;
  std::size_t slopeCount = 0U;
  for (const cr::CreativeObject& object : preview.document.objects()) {
    if (object.kind == cr::CreativeObjectKind::Roof) {
      roof = &object;
      ++roofCount;
    } else if (object.kind == cr::CreativeObjectKind::GableRoof) {
      ++slopeCount;
    }
  }
  const cr::CreativeTransformedBounds bounds =
      roof == nullptr ? cr::CreativeTransformedBounds{}
                      : cr::resolveCreativeObjectBounds(*roof);

  return expect(compiled.receipt.accepted && preview.accepted,
                "flat roof overhang compiles") &&
         expect(roofCount == 1U && slopeCount == 0U && bounds.valid,
                "flat overhang emits one shared level slab") &&
         expect(near(bounds.worldBounds.min.x, -1.0) &&
                    near(bounds.worldBounds.max.x, 9.0) &&
                    near(bounds.worldBounds.min.z, -1.0) &&
                    near(bounds.worldBounds.max.z, 5.0),
                "flat shared roof applies exact authored overhang");
}

bool orthogonalRoomCompilesExactHorizontalSurfaces() {
  cr::CreativeDocument document = cr::CreativeDocument::create("L Room");
  static_cast<void>(document.assignId(9207U));
  const cr::CreativeWorldLayout layout = orthogonalLRoom();
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);

  std::size_t floorCount = 0U;
  std::size_t roofCount = 0U;
  double floorArea = 0.0;
  bool notchCovered = false;
  for (const cr::CreativeObject& object : preview.document.objects()) {
    if (object.kind == cr::CreativeObjectKind::Floor) {
      ++floorCount;
      const cr::CreativeTransformedBounds bounds =
          cr::resolveCreativeObjectBounds(object);
      if (bounds.valid) {
        floorArea += (bounds.worldBounds.max.x - bounds.worldBounds.min.x) *
                     (bounds.worldBounds.max.z - bounds.worldBounds.min.z);
        notchCovered = notchCovered ||
                       (5.0 > bounds.worldBounds.min.x &&
                        5.0 < bounds.worldBounds.max.x &&
                        5.0 > bounds.worldBounds.min.z &&
                        5.0 < bounds.worldBounds.max.z);
      }
    } else if (object.kind == cr::CreativeObjectKind::Roof) {
      ++roofCount;
    }
  }

  cr::CreativeWorldLayout gable = layout;
  gable.levels[0].roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  const cr::CreativeWorldLayoutRoofPlan rejectedRoof =
      cr::planCreativeWorldLayoutRoof(document.gridSettings(), gable, 0U);

  return expect(compiled.receipt.accepted && preview.accepted,
                "orthogonal room compiles through the building recipe") &&
         expect(floorCount == 2U && roofCount == 2U &&
                    near(floorArea, 20.0) && !notchCovered,
                "floor and flat roof use exact L-room surface pieces") &&
         expect(!rejectedRoof.accepted &&
                    rejectedRoof.status ==
                        cr::CreativeWorldLayoutRoofStatus::NonRectangularFootprint,
                "gable roof rejects an irregular support union instead of filling its bounds");
}

}  // namespace

int main() {
  const bool ok = roomEdgesHaveStableCardinalIdentity() &&
                  adjacentRoomsShareOneCanonicalWall() &&
                  sharedEdgeInspectionMatchesCanonicalTopology() &&
                  stackedRoomsUseBuildingFacadesAndLevelPartitions() &&
                  invalidTopologyFailsClosed() &&
                  roomTopologyCompilesThroughExistingBuildingRecipe() &&
                  generatedRoomObjectsResolveToSemanticSources() &&
                  generatedWallHitResolvesExactCanonicalEdge() &&
                  horizontalStructuralLayersUseDescriptorThickness() &&
                  architecturalDimensionsOwnCompilerAndOpeningScale() &&
                  occupiedLevelsGenerateCeilingsAndOneTopRoof() &&
                  authoredGableRoofCompilesThroughSharedRenderCollisionGeometry() &&
                  authoredRoofStylesKeepStableGeneratedOwnership() &&
                  authoredRoofAperturesCompileWithStableOwnershipAndCollisionHoles() &&
                  flatRoofOverhangUsesTheSharedLevelFootprint() &&
                  orthogonalRoomCompilesExactHorizontalSurfaces();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
