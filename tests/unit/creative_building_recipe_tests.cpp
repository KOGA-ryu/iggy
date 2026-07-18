#include "app/iggy3d/creative/recipes/BuildingRecipe.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-9;
}

bool sameBounds(const cr::CreativeBounds& lhs,
                const cr::CreativeBounds& rhs) {
  return near(lhs.min.x, rhs.min.x) && near(lhs.min.y, rhs.min.y) &&
         near(lhs.min.z, rhs.min.z) && near(lhs.max.x, rhs.max.x) &&
         near(lhs.max.y, rhs.max.y) && near(lhs.max.z, rhs.max.z);
}

const cr::CreativeRecipeObjectPlan* findPlanObject(
    const cr::CreativeRecipePlan& plan,
    std::string_view stableKey) {
  const auto found = std::find_if(
      plan.objects.begin(), plan.objects.end(),
      [stableKey](const cr::CreativeRecipeObjectPlan& object) {
        return object.stableKey == stableKey;
      });
  return found == plan.objects.end() ? nullptr : &*found;
}

bool hasTag(const cr::CreativeRecipeObjectPlan* object,
            std::string_view tag) {
  return object != nullptr &&
         std::find(object->createRequest.tags.begin(),
                   object->createRequest.tags.end(), tag) !=
             object->createRequest.tags.end();
}

cr::CreativeBuildingRecipeRequest representativeRoom() {
  cr::CreativeBuildingRecipeRequest request;
  request.name = "Opening Room";
  request.rootMode = cr::CreativeBuildingRootMode::CreateRoom;
  request.rootBounds = {{0.0, 0.0, 0.0}, {8.0, 3.0, 6.0}};
  request.boxes.push_back({cr::CreativeObjectKind::Floor,
                           "floor.main",
                           "Main Floor",
                           {{0.0, 0.0, 0.0}, {8.0, 0.25, 6.0}}});

  cr::CreativeBuildingWallSpec wall;
  wall.stableKey = "wall.north";
  wall.name = "North Wall";
  wall.start = {0.0, 0.0, 0.0};
  wall.end = {8.0, 0.0, 0.0};
  wall.openings = {
      cr::makeCreativeBuildingWindowOpening("window.right", "Right Window",
                                             6.5, 1.0, 1.0, 1.0),
      cr::makeCreativeBuildingDoorOpening("door.main", "Main Door", 1.5,
                                           1.0, 2.1),
      cr::makeCreativeBuildingWindowOpening("window.left", "Left Window",
                                             4.0, 1.2, 1.0, 1.0),
  };
  request.walls.push_back(std::move(wall));
  return request;
}

bool roomRecipeProducesDeterministicRealOpenings() {
  const cr::CreativeBuildingRecipeRequest request = representativeRoom();
  const cr::CreativeBuildingRecipeResult result =
      cr::buildCreativeBuildingRecipe(request);
  const cr::CreativeRecipeMaterializeResult materialized =
      cr::materializeCreativeRecipe(result.plan, 100U);
  const cr::CreativeRecipeObjectPlan* door =
      findPlanObject(result.plan, "door.main.insert");
  const cr::CreativeRecipeObjectPlan* doorLintel =
      findPlanObject(result.plan, "door.main.lintel");
  const cr::CreativeRecipeObjectPlan* firstSpan =
      findPlanObject(result.plan, "wall.north.segment.1");
  const cr::CreativeRecipeObjectPlan* leftWindow =
      findPlanObject(result.plan, "window.left.insert");

  bool allChildrenUseRoot = materialized.receipt.accepted;
  for (std::size_t index = 1U; index < materialized.createRequests.size();
       ++index) {
    allChildrenUseRoot &= materialized.createRequests[index].parentId == 100U;
  }

  const cr::CreativeWallGeometryDefaults wallDefaults =
      cr::defaultCreativeWallGeometry();
  return expect(request.walls[0].heightMeters == wallDefaults.heightMeters &&
                    request.walls[0].thicknessMeters ==
                        wallDefaults.thicknessMeters,
                "building wall defaults inherit descriptor geometry") &&
         expect(result.receipt.accepted, "building recipe accepted") &&
         expect(result.receipt.status == cr::CreativeBuildingRecipeStatus::Ready,
                "building recipe ready") &&
         expect(result.receipt.rootObjectCount == 1U &&
                    result.receipt.boxObjectCount == 1U,
                "building root and floor counts") &&
         expect(result.receipt.wallObjectCount == 9U,
                "building segmented wall count") &&
         expect(result.receipt.doorObjectCount == 1U &&
                    result.receipt.windowObjectCount == 2U,
                "building opening insert counts") &&
         expect(result.plan.objects.size() == 14U,
                "building complete object plan count") &&
         expect(materialized.createRequests.size() == 14U &&
                    allChildrenUseRoot,
                "building materialized hierarchy") &&
         expect(firstSpan != nullptr &&
                    sameBounds(firstSpan->createRequest.bounds,
                               {{0.0, 0.0, -0.125},
                                {1.0, 3.0, 0.125}}),
                "building first wall span ends at door") &&
         expect(door != nullptr &&
                    sameBounds(door->createRequest.bounds,
                               {{1.0, 0.0, -0.125},
                                {2.0, 2.1, 0.125}}),
                "building door occupies cutout without wall overlap") &&
         expect(doorLintel != nullptr &&
                    sameBounds(doorLintel->createRequest.bounds,
                               {{1.0, 2.1, -0.125},
                                {2.0, 3.0, 0.125}}),
                "building door lintel fills only above cutout") &&
         expect(leftWindow != nullptr &&
                    sameBounds(leftWindow->createRequest.bounds,
                               {{3.4, 1.0, -0.125},
                                {4.6, 2.0, 0.125}}),
                "building window occupies exact cutout") &&
         expect(cr::creativeRecipeRequestHasProvenance(
                    materialized.createRequests.back(),
                    cr::CreativeRecipeKind::Building,
                    cr::CreativeRecipeObjectRole::Generated,
                    "wall.north.segment.4"),
                "building final span carries recipe provenance");
}

bool inputOpeningOrderDoesNotChangeOutput() {
  cr::CreativeBuildingRecipeRequest first = representativeRoom();
  cr::CreativeBuildingRecipeRequest second = representativeRoom();
  std::reverse(second.walls[0].openings.begin(),
               second.walls[0].openings.end());
  const cr::CreativeBuildingRecipeResult firstResult =
      cr::buildCreativeBuildingRecipe(first);
  const cr::CreativeBuildingRecipeResult secondResult =
      cr::buildCreativeBuildingRecipe(second);
  if (!firstResult.receipt.accepted || !secondResult.receipt.accepted ||
      firstResult.plan.objects.size() != secondResult.plan.objects.size()) {
    return expect(false, "building order comparison prerequisites");
  }
  for (std::size_t index = 0; index < firstResult.plan.objects.size(); ++index) {
    const cr::CreativeRecipeObjectPlan& lhs = firstResult.plan.objects[index];
    const cr::CreativeRecipeObjectPlan& rhs = secondResult.plan.objects[index];
    if (lhs.stableKey != rhs.stableKey ||
        lhs.createRequest.kind != rhs.createRequest.kind ||
        lhs.createRequest.name != rhs.createRequest.name ||
        !sameBounds(lhs.createRequest.bounds, rhs.createRequest.bounds)) {
      return expect(false, "building opening order changes output");
    }
  }
  return true;
}

bool stairBoxesPreserveAuthoredRotation() {
  cr::CreativeBuildingRecipeRequest request;
  request.stableKey = "vertical";
  request.name = "Vertical circulation";
  request.rootMode = cr::CreativeBuildingRootMode::None;
  request.boxes.push_back({cr::CreativeObjectKind::Stair,
                           "stair.main",
                           "Main Stair",
                           {{0.0, 0.0, 0.0}, {2.0, 3.0, 4.0}},
                           {1.0, 1.0, 1.0},
                           {"vertical_connector"},
                           {0.0, 1.5707963267948966, 0.0}});

  const cr::CreativeBuildingRecipeResult result =
      cr::buildCreativeBuildingRecipe(request);
  const cr::CreativeRecipeObjectPlan* stair =
      findPlanObject(result.plan, "stair.main");
  return expect(result.receipt.accepted && stair != nullptr,
                "stair is an accepted building box") &&
         expect(stair->createRequest.hasTransformOverride &&
                    near(stair->createRequest.transform.rotationEulerRadians.y,
                         1.5707963267948966),
                "stair rotation reaches the document request") &&
         expect(near(stair->createRequest.transform.position.x, 1.0) &&
                    near(stair->createRequest.transform.position.y, 1.5) &&
                    near(stair->createRequest.transform.position.z, 2.0),
                "stair pivot remains the authored-bounds center");
}

bool concernTagsReachOnlyTheirGeneratedObjects() {
  cr::CreativeBuildingRecipeRequest request = representativeRoom();
  request.tags = {"building-common"};
  request.boxes[0].tags = {"floor-source"};
  request.walls[0].tags = {"wall-source"};
  const auto door = std::find_if(
      request.walls[0].openings.begin(), request.walls[0].openings.end(),
      [](const cr::CreativeBuildingOpeningSpec& opening) {
        return opening.stableKey == "door.main";
      });
  if (door == request.walls[0].openings.end()) {
    return expect(false, "tag test door prerequisite");
  }
  door->tags = {"opening-source"};

  const cr::CreativeBuildingRecipeResult result =
      cr::buildCreativeBuildingRecipe(request);
  const cr::CreativeRecipeObjectPlan* floor =
      findPlanObject(result.plan, "floor.main");
  const cr::CreativeRecipeObjectPlan* wall =
      findPlanObject(result.plan, "wall.north.segment.1");
  const cr::CreativeRecipeObjectPlan* insert =
      findPlanObject(result.plan, "door.main.insert");
  const cr::CreativeRecipeObjectPlan* lintel =
      findPlanObject(result.plan, "door.main.lintel");
  return expect(result.receipt.accepted,
                "per-concern tag recipe accepted") &&
         expect(hasTag(floor, "building-common") &&
                    hasTag(floor, "floor-source") &&
                    !hasTag(floor, "wall-source") &&
                    !hasTag(floor, "opening-source"),
                "box tags remain on box output") &&
         expect(hasTag(wall, "building-common") &&
                    hasTag(wall, "wall-source") &&
                    !hasTag(wall, "opening-source"),
                "wall tags remain on wall spans") &&
         expect(hasTag(insert, "building-common") &&
                    hasTag(insert, "opening-source") &&
                    !hasTag(insert, "wall-source") &&
                    hasTag(lintel, "opening-source"),
                "opening tags cover insert and cutout wall pieces");
}

bool openDoorPoseMatchesReferenceGeometry() {
  cr::CreativeBuildingRecipeRequest request;
  request.name = "Open Door";
  request.rootMode = cr::CreativeBuildingRootMode::None;
  cr::CreativeBuildingWallSpec wall;
  wall.stableKey = "wall.south";
  wall.name = "South Wall";
  wall.start = {6.0, 3.25, 10.0};
  wall.end = {26.0, 3.25, 10.0};
  cr::CreativeBuildingOpeningSpec door =
      cr::makeCreativeBuildingDoorOpening("door.front", "Front Door Open",
                                           5.0, 2.0, 3.0);
  door.pose =
      cr::CreativeBuildingOpeningPose::OpenFromStartNegativeNormal;
  door.insertHeightMeters = 2.25;
  door.insertWidthMeters = 1.8;
  door.insertThicknessMeters = 0.2;
  wall.openings.push_back(door);
  request.walls.push_back(wall);
  const cr::CreativeBuildingRecipeResult result =
      cr::buildCreativeBuildingRecipe(request);
  const cr::CreativeRecipeObjectPlan* insert =
      findPlanObject(result.plan, "door.front.insert");

  return expect(result.receipt.accepted, "open door recipe accepted") &&
         expect(insert != nullptr, "open door insert present") &&
         expect(insert != nullptr &&
                    sameBounds(insert->createRequest.bounds,
                               {{10.0, 3.25, 8.2},
                                {10.2, 5.5, 10.0}}),
                "open door pose preserves reference bounds");
}

bool assetBackedOpeningsFitTheStructuralInsertVolume() {
  const cr::CreativeBounds sourceBounds{{-0.2, -0.1, -0.05},
                                         {1.3, 2.3, 0.15}};
  const auto buildDoor = [&](cr::CreativeBuildingOpeningPose pose,
                             bool includeInsert = true) {
    cr::CreativeBuildingRecipeRequest request;
    request.stableKey = "asset_opening";
    request.name = "Asset Opening";
    request.rootMode = cr::CreativeBuildingRootMode::None;
    cr::CreativeBuildingWallSpec wall;
    wall.stableKey = "wall.reversed";
    wall.name = "Reversed Wall";
    wall.start = {8.0, 0.0, 0.0};
    wall.end = {0.0, 0.0, 0.0};
    cr::CreativeBuildingOpeningSpec door =
        cr::makeCreativeBuildingDoorOpening("door.asset", "Catalog Door",
                                             2.0, 1.5, 2.4);
    door.pose = pose;
    door.insertHeightMeters = 2.4;
    door.insertWidthMeters = 1.5;
    door.insertThicknessMeters = 0.2;
    door.includeInsert = includeInsert;
    door.insertAssetId = "homestead/modular/door_leaf_1p1x2p2";
    door.insertAssetSourceBoundsMeters = sourceBounds;
    door.hasInsertAssetSourceBounds = true;
    wall.openings.push_back(std::move(door));
    request.walls.push_back(std::move(wall));
    return cr::buildCreativeBuildingRecipe(request);
  };

  const cr::CreativeBuildingRecipeResult closed =
      buildDoor(cr::CreativeBuildingOpeningPose::Closed);
  const cr::CreativeBuildingRecipeResult open = buildDoor(
      cr::CreativeBuildingOpeningPose::OpenFromStartPositiveNormal);
  const cr::CreativeBuildingRecipeResult cutoutOnly =
      buildDoor(cr::CreativeBuildingOpeningPose::Closed, false);
  const cr::CreativeRecipeObjectPlan* closedInsert =
      findPlanObject(closed.plan, "door.asset.insert");
  const cr::CreativeRecipeObjectPlan* openInsert =
      findPlanObject(open.plan, "door.asset.insert");
  const cr::CreativeTransformedBounds closedResolved =
      closedInsert == nullptr
          ? cr::CreativeTransformedBounds{}
          : cr::resolveCreativeTransformedBounds(
                closedInsert->createRequest.bounds,
                closedInsert->createRequest.transform);
  const cr::CreativeTransformedBounds openResolved =
      openInsert == nullptr
          ? cr::CreativeTransformedBounds{}
          : cr::resolveCreativeTransformedBounds(
                openInsert->createRequest.bounds,
                openInsert->createRequest.transform);

  cr::CreativeBuildingOpeningAssetFitRequest invalidFit;
  invalidFit.sourceBoundsMeters = sourceBounds;
  invalidFit.sourceBoundsMeters.max.x =
      std::numeric_limits<double>::quiet_NaN();
  invalidFit.targetBoundsMeters = {{0.0, 0.0, 0.0}, {1.0, 2.0, 0.2}};
  invalidFit.wallFrame.axis = cr::CreativeStructuralWallAxis::X;
  invalidFit.wallFrame.tangent = {1.0, 0.0, 0.0};
  invalidFit.wallFrame.normal = {0.0, 0.0, 1.0};
  const cr::CreativeBuildingOpeningAssetFitPlan invalid =
      cr::planCreativeBuildingOpeningAssetFit(invalidFit);

  return expect(closed.receipt.accepted && closedInsert != nullptr &&
                    closedResolved.valid,
                "asset-backed closed opening compiles") &&
         expect(closedInsert->createRequest.assetId ==
                        "homestead/modular/door_leaf_1p1x2p2" &&
                    closedInsert->createRequest.hasBoundsOverride &&
                    closedInsert->createRequest.hasTransformOverride,
                "asset identity and explicit pivot reach the document request") &&
         expect(sameBounds(closedResolved.worldBounds,
                           {{5.25, 0.0, -0.1}, {6.75, 2.4, 0.1}}),
                "reversed closed wall fits the exact insert volume") &&
         expect(open.receipt.accepted && openInsert != nullptr &&
                    openResolved.valid &&
                    sameBounds(openResolved.worldBounds,
                               {{6.55, 0.0, 0.0},
                                {6.75, 2.4, 1.5}}),
                "open pose rotates the asset into the exact swept insert volume") &&
         expect(cutoutOnly.receipt.accepted &&
                    findPlanObject(cutoutOnly.plan, "door.asset.insert") ==
                        nullptr,
                "disabling an asset insert preserves a valid cutout-only opening") &&
         expect(!invalid.accepted &&
                    invalid.status == cr::CreativeBuildingOpeningAssetFitStatus::
                                          InvalidSourceBounds,
                "asset fit rejects non-finite source geometry");
}

bool invalidGeometryFailsWithSpecificStatuses() {
  cr::CreativeBuildingRecipeRequest diagonal = representativeRoom();
  diagonal.walls[0].end = {8.0, 0.0, 2.0};
  const cr::CreativeBuildingRecipeResult diagonalResult =
      cr::buildCreativeBuildingRecipe(diagonal);

  cr::CreativeBuildingRecipeRequest overlap = representativeRoom();
  overlap.walls[0].openings[1].centerOffsetMeters = 6.4;
  const cr::CreativeBuildingRecipeResult overlapResult =
      cr::buildCreativeBuildingRecipe(overlap);

  cr::CreativeBuildingRecipeRequest nonFinite = representativeRoom();
  nonFinite.walls[0].openings[0].widthMeters =
      std::numeric_limits<double>::infinity();
  const cr::CreativeBuildingRecipeResult nonFiniteResult =
      cr::buildCreativeBuildingRecipe(nonFinite);

  cr::CreativeBuildingRecipeRequest negativeInsert = representativeRoom();
  negativeInsert.walls[0].openings[0].insertWidthMeters = -1.0;
  const cr::CreativeBuildingRecipeResult negativeInsertResult =
      cr::buildCreativeBuildingRecipe(negativeInsert);

  return expect(!diagonalResult.receipt.accepted &&
                    diagonalResult.receipt.status ==
                        cr::CreativeBuildingRecipeStatus::
                            UnsupportedWallOrientation,
                "diagonal wall explicitly unsupported") &&
         expect(!overlapResult.receipt.accepted &&
                    overlapResult.receipt.status ==
                        cr::CreativeBuildingRecipeStatus::OverlappingOpenings,
                "overlapping openings rejected") &&
         expect(!nonFiniteResult.receipt.accepted &&
                    nonFiniteResult.receipt.status ==
                        cr::CreativeBuildingRecipeStatus::InvalidOpening,
                "non-finite opening rejected") &&
         expect(!negativeInsertResult.receipt.accepted &&
                    negativeInsertResult.receipt.status ==
                        cr::CreativeBuildingRecipeStatus::InvalidOpening,
                "negative insert dimensions reject instead of inheriting");
}

bool rectangularRoomGeometryKeepsFloorAndWallsOnOneSeam() {
  cr::CreativeRectangularRoomGeometryRequest request;
  request.firstFloorCorner = {6.0, 2.0, 5.0};
  request.oppositeFloorCorner = {1.0, 2.0, -3.0};
  request.wallHeightMeters = 4.0;
  request.wallThicknessMeters = 0.5;
  request.floorThicknessMeters = 0.25;
  const cr::CreativeRectangularRoomGeometryPlan plan =
      cr::planCreativeRectangularRoomGeometry(request);

  return expect(plan.accepted &&
                    plan.status ==
                        cr::CreativeRectangularRoomGeometryStatus::Ready,
                "rectangular room geometry accepted") &&
         expect(sameBounds(plan.floorBounds,
                           {{1.0, 1.75, -3.0}, {6.0, 2.0, 5.0}}),
                "rectangular floor extends below finished plane") &&
         expect(sameBounds(plan.wallBounds[0],
                           {{1.0, 2.0, -3.25}, {6.0, 6.0, -2.75}}) &&
                    sameBounds(plan.wallBounds[1],
                               {{5.75, 2.0, -3.0}, {6.25, 6.0, 5.0}}) &&
                    sameBounds(plan.wallBounds[2],
                               {{1.0, 2.0, 4.75}, {6.0, 6.0, 5.25}}) &&
                    sameBounds(plan.wallBounds[3],
                               {{0.75, 2.0, -3.0}, {1.25, 6.0, 5.0}}),
                "rectangular walls begin on finished floor plane") &&
         expect(sameBounds(plan.rootBounds,
                           {{0.75, 1.75, -3.25}, {6.25, 6.0, 5.25}}),
                "rectangular aggregate bounds include floor and wall shell");
}

bool rectangularRoomCompilesToStableShellRecipe() {
  cr::CreativeRectangularRoomRecipeRequest request;
  request.stableKey = "viewport-room-12";
  request.name = "Room 12";
  request.geometry.firstFloorCorner = {0.0, 0.0, 0.0};
  request.geometry.oppositeFloorCorner = {4.0, 0.0, 3.0};
  const cr::CreativeBuildingRecipeResult result =
      cr::buildCreativeRectangularRoomRecipe(request);

  constexpr std::array<std::string_view, 5U> kExpectedKeys{
      "floor", "wall.north.segment.1", "wall.east.segment.1",
      "wall.south.segment.1", "wall.west.segment.1"};
  bool stableOrder = result.plan.objects.size() == kExpectedKeys.size();
  for (std::size_t index = 0U;
       stableOrder && index < kExpectedKeys.size(); ++index) {
    stableOrder = result.plan.objects[index].stableKey == kExpectedKeys[index];
  }
  return expect(result.receipt.accepted &&
                    result.receipt.rootObjectCount == 0U &&
                    result.receipt.boxObjectCount == 1U &&
                    result.receipt.wallObjectCount == 4U,
                "rectangular room compiles to one floor and four walls") &&
         expect(result.plan.instanceKey == "viewport-room-12" && stableOrder,
                "rectangular room recipe order and identity are stable") &&
         expect(result.plan.objects[0].createRequest.kind ==
                        cr::CreativeObjectKind::Floor &&
                    sameBounds(result.plan.objects[0].createRequest.bounds,
                               {{0.0, -0.25, 0.0}, {4.0, 0.0, 3.0}}),
                "rectangular room recipe preserves floor seam");
}

bool rectangularRoomRejectsAmbiguousGeometry() {
  cr::CreativeRectangularRoomGeometryRequest uneven;
  uneven.firstFloorCorner = {0.0, 0.0, 0.0};
  uneven.oppositeFloorCorner = {4.0, 1.0, 4.0};
  const cr::CreativeRectangularRoomGeometryPlan unevenPlan =
      cr::planCreativeRectangularRoomGeometry(uneven);

  cr::CreativeRectangularRoomGeometryRequest consumed;
  consumed.firstFloorCorner = {0.0, 0.0, 0.0};
  consumed.oppositeFloorCorner = {0.4, 0.0, 2.0};
  consumed.wallThicknessMeters = 0.25;
  const cr::CreativeRectangularRoomGeometryPlan consumedPlan =
      cr::planCreativeRectangularRoomGeometry(consumed);

  cr::CreativeRectangularRoomRecipeRequest invalidRecipe;
  invalidRecipe.geometry = consumed;
  const cr::CreativeBuildingRecipeResult result =
      cr::buildCreativeRectangularRoomRecipe(invalidRecipe);
  return expect(!unevenPlan.accepted &&
                    unevenPlan.status ==
                        cr::CreativeRectangularRoomGeometryStatus::
                            UnevenFloorPlane,
                "rectangular room rejects uneven floor corners") &&
         expect(!consumedPlan.accepted &&
                    consumedPlan.status ==
                        cr::CreativeRectangularRoomGeometryStatus::
                            WallConsumesFootprint,
                "rectangular room rejects walls consuming the interior") &&
         expect(!result.receipt.accepted &&
                    result.receipt.status ==
                        cr::CreativeBuildingRecipeStatus::InvalidWall &&
                    result.plan.objects.empty(),
                "invalid rectangular geometry cannot leak a partial recipe");
}

}  // namespace

int main() {
  const bool ok = roomRecipeProducesDeterministicRealOpenings() &&
                  inputOpeningOrderDoesNotChangeOutput() &&
                  stairBoxesPreserveAuthoredRotation() &&
                  concernTagsReachOnlyTheirGeneratedObjects() &&
                  openDoorPoseMatchesReferenceGeometry() &&
                  assetBackedOpeningsFitTheStructuralInsertVolume() &&
                  invalidGeometryFailsWithSpecificStatuses() &&
                  rectangularRoomGeometryKeepsFloorAndWallsOnOneSeam() &&
                  rectangularRoomCompilesToStableShellRecipe() &&
                  rectangularRoomRejectsAmbiguousGeometry();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
