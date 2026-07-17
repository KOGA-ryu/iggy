#include "app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
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

cr::CreativeTransformedBounds resolvedPart(
    const cr::CreativeStructuralRoofPart& part) {
  cr::CreativeTransform transform;
  transform.position = {
      (part.bounds.min.x + part.bounds.max.x) * 0.5,
      (part.bounds.min.y + part.bounds.max.y) * 0.5,
      (part.bounds.min.z + part.bounds.max.z) * 0.5,
  };
  transform.rotationEulerRadians = part.rotationEulerRadians;
  return cr::resolveCreativeTransformedBounds(part.bounds, transform);
}

cr::CreativeStructuralRoofRecipeRequest request() {
  cr::CreativeStructuralRoofRecipeRequest value;
  value.minimumX = 0.0;
  value.maximumX = 8.0;
  value.minimumZ = 0.0;
  value.maximumZ = 6.0;
  value.supportPlaneMeters = 3.0;
  return value;
}

bool flatRoofPreservesStructuralLayerSemantics() {
  auto value = request();
  value.layerCount = 2U;
  value.overhangMeters = 0.5;
  const auto plan = cr::planCreativeStructuralRoof(value);
  return expect(plan.accepted && plan.partCount == 1U,
                "flat roof emits one structural part") &&
         expect(plan.parts[0].kind == cr::CreativeObjectKind::Roof &&
                    plan.parts[0].bounds.min.x == -0.5 &&
                    plan.parts[0].bounds.max.x == 8.5 &&
                    plan.parts[0].bounds.min.z == -0.5 &&
                    plan.parts[0].bounds.max.z == 6.5 &&
                    plan.parts[0].bounds.min.y == 3.0 &&
                    plan.parts[0].bounds.max.y == 5.0 &&
                    plan.baseThicknessMeters == 2.0,
                "flat roof uses descriptor thickness and exact overhang");
}

bool gableRoofOwnsPitchRidgeAndMirroredWedges() {
  auto value = request();
  value.style = cr::CreativeStructuralRoofStyle::Gable;
  value.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  value.pitchDegrees = 45.0;
  value.overhangMeters = 1.0;
  const auto xPlan = cr::planCreativeStructuralRoof(value);

  value.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  const auto zPlan = cr::planCreativeStructuralRoof(value);
  const cr::CreativeTransformedBounds west = resolvedPart(zPlan.parts[1]);
  const cr::CreativeTransformedBounds east = resolvedPart(zPlan.parts[2]);
  return expect(xPlan.accepted && xPlan.partCount == 3U &&
                    near(xPlan.riseMeters, 4.0),
                "x-ridge gable emits base and two exact slopes") &&
         expect(xPlan.parts[0].kind == cr::CreativeObjectKind::Roof &&
                    xPlan.parts[1].kind ==
                        cr::CreativeObjectKind::GableRoof &&
                    xPlan.parts[2].kind ==
                        cr::CreativeObjectKind::GableRoof &&
                    xPlan.parts[1].bounds.min.z == -1.0 &&
                    xPlan.parts[1].bounds.max.z == 3.0 &&
                    xPlan.parts[2].bounds.min.z == 3.0 &&
                    xPlan.parts[2].bounds.max.z == 7.0 &&
                    near(xPlan.parts[2].rotationEulerRadians.y,
                         std::numbers::pi),
                "x-ridge wedges meet once and rise toward the same ridge") &&
         expect(near(xPlan.ridgeStart.x, -1.0) &&
                    near(xPlan.ridgeStart.y, 8.0) &&
                    near(xPlan.ridgeStart.z, 3.0) &&
                    near(xPlan.ridgeEnd.x, 9.0) &&
                    near(xPlan.ridgeEnd.y, 8.0) &&
                    near(xPlan.ridgeEnd.z, 3.0),
                "x-ridge endpoints include overhang and exact pitch") &&
         expect(zPlan.accepted && zPlan.partCount == 3U &&
                    near(zPlan.riseMeters, 5.0) &&
                    near(zPlan.parts[1].rotationEulerRadians.y,
                         std::numbers::pi * 0.5) &&
                    near(zPlan.parts[2].rotationEulerRadians.y,
                         -std::numbers::pi * 0.5),
                "z-ridge swaps span and rotates both slope wedges") &&
         expect(west.valid && east.valid &&
                    near(west.worldBounds.min.x, -1.0) &&
                    near(west.worldBounds.max.x, 4.0) &&
                    near(west.worldBounds.min.z, -1.0) &&
                    near(west.worldBounds.max.z, 7.0) &&
                    near(east.worldBounds.min.x, 4.0) &&
                    near(east.worldBounds.max.x, 9.0) &&
                    near(east.worldBounds.min.z, -1.0) &&
                    near(east.worldBounds.max.z, 7.0),
                "rotated z-ridge wedges resolve to exact world halves");
}

bool invalidRoofInputsFailClosed() {
  auto invalidPitch = request();
  invalidPitch.style = cr::CreativeStructuralRoofStyle::Gable;
  invalidPitch.pitchDegrees = 90.0;
  auto invalidOverhang = request();
  invalidOverhang.overhangMeters = -0.1;
  auto nonFinite = request();
  nonFinite.maximumX = std::numeric_limits<double>::infinity();
  auto zeroLayers = request();
  zeroLayers.layerCount = 0U;
  return expect(!cr::planCreativeStructuralRoof(invalidPitch).accepted,
                "unsafe roof pitch rejects") &&
         expect(!cr::planCreativeStructuralRoof(invalidOverhang).accepted,
                "negative roof overhang rejects") &&
         expect(!cr::planCreativeStructuralRoof(nonFinite).accepted,
                "non-finite roof footprint rejects") &&
         expect(!cr::planCreativeStructuralRoof(zeroLayers).accepted,
                "zero roof layers reject");
}

}  // namespace

int main() {
  const bool ok = flatRoofPreservesStructuralLayerSemantics() &&
                  gableRoofOwnsPitchRidgeAndMirroredWedges() &&
                  invalidRoofInputsFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
