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
  const cr::CreativeRecipeObjectPlan* leftWindowFrame =
      findPlanObject(result.plan, "window.left.frame.minimum_jamb");
  const cr::CreativeRecipeObjectPlan* doorFrame =
      findPlanObject(result.plan, "door.main.frame.minimum_jamb");
  const cr::CreativeRecipeObjectPlan* doorHandle =
      findPlanObject(result.plan, "door.main.hardware.primary.handle");
  const std::size_t doorIndex = door == nullptr
                                    ? result.plan.objects.size()
                                    : static_cast<std::size_t>(
                                          door - result.plan.objects.data());
  const std::size_t handleIndex =
      doorHandle == nullptr
          ? result.plan.objects.size()
          : static_cast<std::size_t>(doorHandle - result.plan.objects.data());

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
                    result.receipt.doorPartObjectCount == 7U &&
                    result.receipt.windowObjectCount == 2U &&
                    result.receipt.windowPartObjectCount == 10U,
                "building opening assembly counts") &&
         expect(result.plan.objects.size() == 28U,
                "building complete object plan count") &&
         expect(materialized.createRequests.size() == 28U &&
                    doorIndex < materialized.createRequests.size() &&
                    handleIndex < materialized.createRequests.size() &&
                    materialized.createRequests[doorIndex].parentId == 100U &&
                    materialized.createRequests[handleIndex].parentId ==
                        100U + doorIndex,
                "building materializes door assembly hierarchy") &&
         expect(firstSpan != nullptr &&
                    sameBounds(firstSpan->createRequest.bounds,
                               {{0.0, 0.0, -0.125},
                                {1.0, 3.0, 0.125}}),
                "building first wall span ends at door") &&
         expect(door != nullptr &&
                    sameBounds(door->createRequest.bounds,
                               {{1.075, 0.015, -0.025},
                                {1.925, 2.025, 0.025}}) &&
                    door->createRequest.hasDoorSettingsOverride,
                "building door leaf fits inside its real frame") &&
         expect(doorFrame != nullptr &&
                    sameBounds(doorFrame->createRequest.bounds,
                               {{1.0, 0.0, -0.125},
                                {1.07, 2.1, 0.125}}),
                "building door emits physical frame geometry") &&
         expect(doorLintel != nullptr &&
                    sameBounds(doorLintel->createRequest.bounds,
                               {{1.0, 2.1, -0.125},
                                {2.0, 3.0, 0.125}}),
                "building door lintel fills only above cutout") &&
         expect(leftWindow != nullptr &&
                    sameBounds(leftWindow->createRequest.bounds,
                               {{3.46, 1.06, -0.01},
                                {4.54, 1.94, 0.01}}) &&
                    leftWindow->createRequest.hasWindowSettingsOverride &&
                    leftWindowFrame != nullptr &&
                    sameBounds(leftWindowFrame->createRequest.bounds,
                               {{3.4, 1.0, -0.125},
                                {3.45, 2.0, 0.125}}),
                "building window owns inset glazing and physical frame") &&
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

bool openDoorStatePreservesClosedEditableSource() {
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
  door.door.hingeSide = cr::CreativeDoorHingeSide::MinimumEdge;
  door.door.swingSide = cr::CreativeDoorSwingSide::NegativeNormal;
  door.door.initialState = cr::CreativeDoorInitialState::Open;
  wall.openings.push_back(door);
  request.walls.push_back(wall);
  const cr::CreativeBuildingRecipeResult result =
      cr::buildCreativeBuildingRecipe(request);
  const cr::CreativeRecipeObjectPlan* insert =
      findPlanObject(result.plan, "door.front.insert");

  return expect(result.receipt.accepted, "initially-open door recipe accepted") &&
         expect(insert != nullptr, "initially-open door leaf present") &&
         expect(insert != nullptr &&
                    sameBounds(insert->createRequest.bounds,
                               {{10.075, 3.265, 9.975},
                                {11.925, 6.175, 10.025}}) &&
                    insert->createRequest.door.initialState ==
                        cr::CreativeDoorInitialState::Open &&
                    insert->createRequest.door.swingSide ==
                        cr::CreativeDoorSwingSide::NegativeNormal,
                "initial state is data while source geometry stays closed");
}

bool pairedShutterWindowOwnsItsSecondaryInsert() {
  cr::CreativeBuildingRecipeRequest request;
  request.stableKey = "paired-shutter-window";
  request.name = "Paired Shutter Window";
  request.rootMode = cr::CreativeBuildingRootMode::None;

  cr::CreativeBuildingWallSpec wall;
  wall.stableKey = "wall.front";
  wall.name = "Front Wall";
  wall.start = {0.0, 0.0, 0.0};
  wall.end = {4.0, 0.0, 0.0};
  cr::CreativeBuildingOpeningSpec window =
      cr::makeCreativeBuildingWindowOpening(
          "window.shuttered", "Shuttered Window", 2.0, 1.2, 1.0, 1.0);
  window.window.insertKind = cr::CreativeWindowInsertKind::PairedShutters;
  wall.openings.push_back(window);
  request.walls.push_back(wall);

  const cr::CreativeBuildingRecipeResult result =
      cr::buildCreativeBuildingRecipe(request);
  const cr::CreativeRecipeObjectPlan* primary =
      findPlanObject(result.plan, "window.shuttered.insert");
  const cr::CreativeRecipeObjectPlan* secondary =
      findPlanObject(result.plan, "window.shuttered.shutter.secondary");
  const std::size_t primaryIndex =
      primary == nullptr
          ? result.plan.objects.size()
          : static_cast<std::size_t>(primary - result.plan.objects.data());
  const std::size_t secondaryIndex =
      secondary == nullptr
          ? result.plan.objects.size()
          : static_cast<std::size_t>(secondary - result.plan.objects.data());
  const cr::CreativeRecipeMaterializeResult materialized =
      cr::materializeCreativeRecipe(result.plan, 500U);

  return expect(result.receipt.accepted && primary != nullptr &&
                    secondary != nullptr,
                "paired shutter window compiles both inserts") &&
         expect(primary->createRequest.kind == cr::CreativeObjectKind::Window &&
                    primary->createRequest.hasWindowSettingsOverride &&
                    primary->createRequest.window.insertKind ==
                        cr::CreativeWindowInsertKind::PairedShutters &&
                    cr::describeObject(cr::CreativeObjectKind::Window)
                        .canOwnChildren,
                "primary window owns its paired-shutter contract") &&
         expect(secondary->createRequest.kind == cr::CreativeObjectKind::Prop &&
                    secondary->parentObjectIndex == primaryIndex &&
                    secondary->createRequest.attachmentSocket ==
                        "window_secondary_shutter",
                "secondary shutter targets the primary window socket") &&
         expect(materialized.receipt.accepted &&
                    secondaryIndex < materialized.createRequests.size() &&
                    materialized.createRequests[secondaryIndex].parentId ==
                        500U + primaryIndex,
                "paired shutter materialization resolves the window parent");
}

bool assetBackedOpeningsFitTheStructuralInsertVolume() {
  const cr::CreativeBounds sourceBounds{{-0.2, -0.1, -0.05},
                                         {1.3, 2.3, 0.15}};
  const auto buildDoor = [&](cr::CreativeBuildingOpeningPose pose,
                             bool includeInsert = true,
                             cr::CreativeBuildingOpeningFacing facing =
                                 cr::CreativeBuildingOpeningFacing::
                                     PositiveNormal) {
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
    door.door.initialState =
        pose == cr::CreativeBuildingOpeningPose::Closed
            ? cr::CreativeDoorInitialState::Closed
            : cr::CreativeDoorInitialState::Open;
    door.facing = facing;
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
  const cr::CreativeBuildingRecipeResult oppositeFacing = buildDoor(
      cr::CreativeBuildingOpeningPose::Closed, true,
      cr::CreativeBuildingOpeningFacing::NegativeNormal);
  const cr::CreativeBuildingRecipeResult cutoutOnly =
      buildDoor(cr::CreativeBuildingOpeningPose::Closed, false);
  const cr::CreativeRecipeObjectPlan* closedInsert =
      findPlanObject(closed.plan, "door.asset.insert");
  const cr::CreativeRecipeObjectPlan* openInsert =
      findPlanObject(open.plan, "door.asset.insert");
  const cr::CreativeRecipeObjectPlan* oppositeFacingInsert =
      findPlanObject(oppositeFacing.plan, "door.asset.insert");
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
  const cr::CreativeTransformedBounds oppositeFacingResolved =
      oppositeFacingInsert == nullptr
          ? cr::CreativeTransformedBounds{}
          : cr::resolveCreativeTransformedBounds(
                oppositeFacingInsert->createRequest.bounds,
                oppositeFacingInsert->createRequest.transform);

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
  cr::CreativeBuildingOpeningAssetFitRequest invalidFacingFit;
  invalidFacingFit.sourceBoundsMeters = sourceBounds;
  invalidFacingFit.targetBoundsMeters =
      {{5.25, 0.0, -0.1}, {6.75, 2.4, 0.1}};
  invalidFacingFit.wallFrame.axis = cr::CreativeStructuralWallAxis::X;
  invalidFacingFit.wallFrame.tangent = {-1.0, 0.0, 0.0};
  invalidFacingFit.wallFrame.normal = {0.0, 0.0, -1.0};
  invalidFacingFit.facing = cr::CreativeBuildingOpeningFacing::Count;
  const cr::CreativeBuildingOpeningAssetFitPlan invalidFacing =
      cr::planCreativeBuildingOpeningAssetFit(invalidFacingFit);

  return expect(closed.receipt.accepted && closedInsert != nullptr &&
                    closedResolved.valid,
                "asset-backed closed opening compiles") &&
         expect(closedInsert->createRequest.assetId ==
                        "homestead/modular/door_leaf_1p1x2p2" &&
                    closedInsert->createRequest.hasBoundsOverride &&
                    closedInsert->createRequest.hasTransformOverride,
                "asset identity and explicit pivot reach the document request") &&
         expect(sameBounds(closedResolved.worldBounds,
                           {{5.325, 0.015, -0.025},
                            {6.675, 2.325, 0.025}}),
                "reversed closed wall fits the framed leaf volume") &&
         expect(open.receipt.accepted && openInsert != nullptr &&
                    openResolved.valid &&
                    sameBounds(openResolved.worldBounds,
                               closedResolved.worldBounds) &&
                    openInsert->createRequest.door.initialState ==
                        cr::CreativeDoorInitialState::Open,
                "initially-open asset preserves closed source for runtime motion") &&
         expect(oppositeFacing.receipt.accepted &&
                    oppositeFacingInsert != nullptr &&
                    oppositeFacingResolved.valid &&
                    sameBounds(oppositeFacingResolved.worldBounds,
                               closedResolved.worldBounds) &&
                    near(std::abs(
                             oppositeFacingInsert->createRequest.transform
                                     .rotationEulerRadians.y -
                             closedInsert->createRequest.transform
                                 .rotationEulerRadians.y),
                         3.14159265358979323846),
                "opening facing rotates an asymmetric asset by half a turn without moving its target volume") &&
         expect(cutoutOnly.receipt.accepted &&
                    findPlanObject(cutoutOnly.plan, "door.asset.insert") ==
                        nullptr,
                "disabling an asset insert preserves a valid cutout-only opening") &&
         expect(!invalid.accepted &&
                    invalid.status == cr::CreativeBuildingOpeningAssetFitStatus::
                                          InvalidSourceBounds,
                "asset fit rejects non-finite source geometry") &&
         expect(!invalidFacing.accepted &&
                    invalidFacing.status ==
                        cr::CreativeBuildingOpeningAssetFitStatus::InvalidFacing,
                "asset fit rejects invalid facing explicitly");
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
                  openDoorStatePreservesClosedEditableSource() &&
                  pairedShutterWindowOwnsItsSecondaryInsert() &&
                  assetBackedOpeningsFitTheStructuralInsertVolume() &&
                  invalidGeometryFailsWithSpecificStatuses() &&
                  rectangularRoomGeometryKeepsFloorAndWallsOnOneSeam() &&
                  rectangularRoomCompilesToStableShellRecipe() &&
                  rectangularRoomRejectsAmbiguousGeometry();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
