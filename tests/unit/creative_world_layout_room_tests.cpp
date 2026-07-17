#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
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
                    near(lowerCeilingBounds.worldBounds.min.y, 3.0) &&
                    near(upperFloorBounds.worldBounds.max.y, 3.0),
                "lower ceiling and upper floor share the story boundary") &&
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
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  std::vector<const cr::CreativeObject*> slopes;
  const cr::CreativeObject* base = nullptr;
  for (const cr::CreativeObject& object : preview.document.objects()) {
    if (object.kind == cr::CreativeObjectKind::GableRoof) {
      slopes.push_back(&object);
    } else if (object.kind == cr::CreativeObjectKind::Roof) {
      base = &object;
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

  return expect(roofPlan.accepted && roofPlan.geometry.partCount == 3U &&
                    roofPlan.footprint.minimum ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    roofPlan.footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 4} &&
                    near(roofPlan.geometry.riseMeters, 3.0),
                "adjacent rooms resolve one pitched roof footprint") &&
         expect(compiled.receipt.accepted && preview.accepted &&
                    base != nullptr && slopes.size() == 2U,
                "gable layout emits one base and two semantic slope objects") &&
         expect(baked.receipt.accepted && firstMesh != nullptr &&
                    secondMesh != nullptr &&
                    firstMesh->meshId == "creative_ramp_wedge" &&
                    secondMesh->meshId == "creative_ramp_wedge",
                "gable slopes reuse the proven generated wedge mesh") &&
         expect(north.status == iggy3d::CollisionQueryStatus::Hit &&
                    ridge.status == iggy3d::CollisionQueryStatus::Hit &&
                    south.status == iggy3d::CollisionQueryStatus::Hit &&
                    nearFloat(north.heightMeters, 5.5F) &&
                    nearFloat(ridge.heightMeters, 7.0F) &&
                    nearFloat(south.heightMeters, 5.5F),
                "rendered gable slopes expose matching pitched collision") &&
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

}  // namespace

int main() {
  const bool ok = roomEdgesHaveStableCardinalIdentity() &&
                  adjacentRoomsShareOneCanonicalWall() &&
                  sharedEdgeInspectionMatchesCanonicalTopology() &&
                  invalidTopologyFailsClosed() &&
                  roomTopologyCompilesThroughExistingBuildingRecipe() &&
                  generatedRoomObjectsResolveToSemanticSources() &&
                  horizontalStructuralLayersUseDescriptorThickness() &&
                  occupiedLevelsGenerateCeilingsAndOneTopRoof() &&
                  authoredGableRoofCompilesThroughSharedRenderCollisionGeometry() &&
                  flatRoofOverhangUsesTheSharedLevelFootprint();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
