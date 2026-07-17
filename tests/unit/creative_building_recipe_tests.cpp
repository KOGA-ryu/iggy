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
                "non-finite opening rejected");
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
                  openDoorPoseMatchesReferenceGeometry() &&
                  invalidGeometryFailsWithSpecificStatuses() &&
                  rectangularRoomGeometryKeepsFloorAndWallsOnOneSeam() &&
                  rectangularRoomCompilesToStableShellRecipe() &&
                  rectangularRoomRejectsAmbiguousGeometry();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
