#include "app/iggy3d/creative/recipes/DoorRecipe.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

constexpr double kHalfPi = 1.57079632679489661923;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-9;
}

cr::CreativeStructuralWallFrame xWall() {
  cr::CreativeStructuralWallFrame frame;
  frame.axis = cr::CreativeStructuralWallAxis::X;
  frame.start = {0.0, 0.0, 0.0};
  frame.end = {4.0, 0.0, 0.0};
  frame.tangent = {1.0, 0.0, 0.0};
  frame.normal = {0.0, 0.0, 1.0};
  frame.bounds = {{0.0, 0.0, -0.125}, {4.0, 3.0, 0.125}};
  frame.baseYMeters = 0.0;
  frame.lengthMeters = 4.0;
  frame.heightMeters = 3.0;
  frame.thicknessMeters = 0.25;
  return frame;
}

cr::CreativeDoorRecipeRequest singleDoor() {
  cr::CreativeDoorRecipeRequest request;
  request.wallFrame = xWall();
  request.cutoutBounds = {{1.0, 0.0, -0.125}, {2.0, 2.1, 0.125}};
  return request;
}

bool singleDoorOwnsFrameLeafHardwareAndExactHinge() {
  const cr::CreativeDoorRecipeResult result = cr::planCreativeDoor(singleDoor());
  const cr::CreativeDoorLeafPlan& leaf = result.leaves[0];
  return expect(result.accepted &&
                    result.status == cr::CreativeDoorRecipeStatus::Ready,
                "single door recipe accepted") &&
         expect(result.partCount == 7U && result.leafCount == 1U,
                "single door has bounded frame leaf and hardware") &&
         expect(result.parts[0].kind ==
                        cr::CreativeDoorPartKind::MinimumJamb &&
                    result.parts[1].kind ==
                        cr::CreativeDoorPartKind::MaximumJamb &&
                    result.parts[2].kind == cr::CreativeDoorPartKind::Header,
                "single door begins with canonical fixed frame") &&
         expect(result.parts[3].kind ==
                        cr::CreativeDoorPartKind::PrimaryLeaf &&
                    result.parts[3].objectKind == cr::CreativeObjectKind::Door,
                "primary leaf carries semantic door kind") &&
         expect(leaf.firstPartIndex == 3U && leaf.partCount == 4U,
                "leaf owns itself and three hardware parts") &&
         expect(near(leaf.hingePivotMeters.x, 1.075) &&
                    near(leaf.hingePivotMeters.z, 0.0),
                "minimum hinge uses canonical world-axis edge") &&
         expect(near(leaf.openAngleRadians, -kHalfPi),
                "positive-normal minimum hinge opens ninety degrees") &&
         expect(result.fullSweepBounds.max.z > 0.85,
                "positive-normal sweep extends away from wall");
}

bool hingeAndSwingSettingsRemainIndependent() {
  cr::CreativeDoorRecipeRequest maximum = singleDoor();
  maximum.settings.hingeSide = cr::CreativeDoorHingeSide::MaximumEdge;
  const cr::CreativeDoorRecipeResult maximumResult =
      cr::planCreativeDoor(maximum);

  cr::CreativeDoorRecipeRequest negative = singleDoor();
  negative.settings.swingSide = cr::CreativeDoorSwingSide::NegativeNormal;
  const cr::CreativeDoorRecipeResult negativeResult =
      cr::planCreativeDoor(negative);
  return expect(maximumResult.accepted && negativeResult.accepted,
                "alternate hinge and swing settings accepted") &&
         expect(near(maximumResult.leaves[0].hingePivotMeters.x, 1.925) &&
                    near(maximumResult.leaves[0].openAngleRadians, kHalfPi),
                "maximum hinge changes pivot and rotation direction") &&
         expect(near(negativeResult.leaves[0].hingePivotMeters.x, 1.075) &&
                    near(negativeResult.leaves[0].openAngleRadians, kHalfPi),
                "negative swing changes rotation without changing hinge") &&
         expect(negativeResult.fullSweepBounds.min.z < -0.85,
                "negative-normal sweep extends to negative side");
}

bool doubleDoorProducesTwoOpposedLeafAssemblies() {
  cr::CreativeDoorRecipeRequest request = singleDoor();
  request.cutoutBounds.max.x = 2.8;
  request.settings.leafArrangement =
      cr::CreativeDoorLeafArrangement::Double;
  const cr::CreativeDoorRecipeResult result = cr::planCreativeDoor(request);
  return expect(result.accepted && result.partCount == 11U &&
                    result.leafCount == 2U,
                "double door fills fixed bounded capacities") &&
         expect(result.parts[7].kind ==
                        cr::CreativeDoorPartKind::SecondaryLeaf &&
                    result.parts[7].objectKind == cr::CreativeObjectKind::Prop,
                "secondary leaf remains subordinate to semantic primary") &&
         expect(result.leaves[0].partCount == 4U &&
                    result.leaves[1].partCount == 4U,
                "each double-door leaf owns complete hardware") &&
         expect(result.leaves[0].hingePivotMeters.x <
                        result.leaves[1].hingePivotMeters.x &&
                    result.leaves[0].openAngleRadians < 0.0 &&
                    result.leaves[1].openAngleRadians > 0.0,
                "double leaves hinge at outer edges and open together");
}

bool zWallUsesTheSameCanonicalContract() {
  cr::CreativeDoorRecipeRequest request = singleDoor();
  request.wallFrame.axis = cr::CreativeStructuralWallAxis::Z;
  request.wallFrame.start = {0.0, 0.0, 0.0};
  request.wallFrame.end = {0.0, 0.0, 4.0};
  request.wallFrame.tangent = {0.0, 0.0, 1.0};
  request.wallFrame.normal = {1.0, 0.0, 0.0};
  request.wallFrame.bounds = {{-0.125, 0.0, 0.0}, {0.125, 3.0, 4.0}};
  request.cutoutBounds = {{-0.125, 0.0, 1.0}, {0.125, 2.1, 2.0}};
  const cr::CreativeDoorRecipeResult result = cr::planCreativeDoor(request);
  return expect(result.accepted, "z-wall door accepted") &&
         expect(near(result.leaves[0].hingePivotMeters.x, 0.0) &&
                    near(result.leaves[0].hingePivotMeters.z, 1.075),
                "z-wall hinge follows canonical z extent") &&
         expect(result.fullSweepBounds.max.x > 0.85,
                "z-wall positive-normal sweep extends along x");
}

bool invalidInputsFailClosed() {
  cr::CreativeDoorRecipeRequest outside = singleDoor();
  outside.cutoutBounds.max.x = 5.0;
  const cr::CreativeDoorRecipeResult outsideResult =
      cr::planCreativeDoor(outside);

  cr::CreativeDoorRecipeRequest invalidSettings = singleDoor();
  invalidSettings.settings.transitionSeconds =
      std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeDoorRecipeResult settingsResult =
      cr::planCreativeDoor(invalidSettings);

  cr::CreativeDoorRecipeRequest narrow = singleDoor();
  narrow.cutoutBounds.max.x = 1.1;
  const cr::CreativeDoorRecipeResult narrowResult =
      cr::planCreativeDoor(narrow);
  return expect(!outsideResult.accepted &&
                    outsideResult.status ==
                        cr::CreativeDoorRecipeStatus::InvalidCutout,
                "outside cutout rejected") &&
         expect(!settingsResult.accepted &&
                    settingsResult.status ==
                        cr::CreativeDoorRecipeStatus::InvalidSettings,
                "invalid settings rejected") &&
         expect(!narrowResult.accepted &&
                    narrowResult.status ==
                        cr::CreativeDoorRecipeStatus::InvalidDimensions,
                "unrepresentable narrow door rejected");
}

}  // namespace

int main() {
  const bool ok = singleDoorOwnsFrameLeafHardwareAndExactHinge() &&
                  hingeAndSwingSettingsRemainIndependent() &&
                  doubleDoorProducesTwoOpposedLeafAssemblies() &&
                  zWallUsesTheSameCanonicalContract() &&
                  invalidInputsFailClosed();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_door_recipe_tests: PASS\n";
  return EXIT_SUCCESS;
}
