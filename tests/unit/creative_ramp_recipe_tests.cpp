#include "app/iggy3d/creative/recipes/RampRecipe.hpp"

#include "runtime/movement/MovementPolicy.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs, double epsilon = 1.0e-6) {
  return std::fabs(lhs - rhs) <= epsilon;
}

cr::CreativeRampRecipeRequest defaultRequest() {
  cr::CreativeRampRecipeRequest request;
  request.authoredBounds = {{-1.0, 0.0, -2.0}, {1.0, 3.0, 2.0}};
  request.transform.position = {0.0, 1.5, 0.0};
  request.availableHeadroomMeters = 3.0;
  request.material = cr::CreativeStructuralMaterial::Stone;
  return request;
}

bool acceptedRampOwnsTraversalGeometry() {
  const cr::CreativeRampRecipeResult ramp =
      cr::planCreativeRamp(defaultRequest());
  const iggy3d::SlopeSample movement = iggy3d::sampleSlope(
      {static_cast<float>(ramp.surfaceNormal.x),
       static_cast<float>(ramp.surfaceNormal.y),
       static_cast<float>(ramp.surfaceNormal.z)});

  return expect(ramp.accepted && ramp.walkable &&
                    ramp.status == cr::CreativeRampRecipeStatus::Ready,
                "valid ramp is accepted and walkable") &&
         expect(near(ramp.widthMeters, 2.0) && near(ramp.riseMeters, 3.0) &&
                    near(ramp.runMeters, 4.0) &&
                    near(ramp.surfaceLengthMeters, 5.0) &&
                    near(ramp.slopeAngleDegrees, 36.8698976458),
                "recipe owns width rise run length and angle") &&
         expect(ramp.material == cr::CreativeStructuralMaterial::Stone &&
                    ramp.socketCount == 2U &&
                    cr::creativeRampSocketCompatibility(
                        ramp.sockets[0].kind) == "ramp.edge" &&
                    cr::creativeRampSocketCompatibility(
                        ramp.sockets[1].kind) == "ramp.edge",
                "recipe owns material and both side-edge sockets") &&
         expect(near(ramp.lowerLanding.centerMeters.y, 0.0) &&
                    near(ramp.upperLanding.centerMeters.y, 3.0) &&
                    near(ramp.lowSurfaceCenterMeters.z, -2.0) &&
                    near(ramp.highSurfaceCenterMeters.z, 2.0),
                "landings and traversal endpoints align to the slope") &&
         expect(near(ramp.sideEdges[0].lowMeters.x, -1.0) &&
                    near(ramp.sideEdges[1].lowMeters.x, 1.0) &&
                    near(ramp.sideEdges[0].highMeters.y, 3.0) &&
                    near(ramp.sideEdges[1].highMeters.y, 3.0),
                "both side edges span the complete sloped surface") &&
         expect(movement.valid && movement.walkable &&
                    near(movement.angleDegrees, ramp.slopeAngleDegrees, 1.0e-4),
                "runtime movement reads the recipe normal as the same slope");
}

bool directionRotationMovesEveryWorldFrame() {
  cr::CreativeRampRecipeRequest request = defaultRequest();
  request.transform.position = {10.0, 1.5, -5.0};
  request.authoredBounds = {{9.0, 0.0, -7.0}, {11.0, 3.0, -3.0}};
  request.transform.rotationEulerRadians.y = std::numbers::pi * 0.5;
  const cr::CreativeRampRecipeResult ramp = cr::planCreativeRamp(request);

  return expect(ramp.accepted && near(ramp.lowerLanding.centerMeters.x, 7.5) &&
                    near(ramp.upperLanding.centerMeters.x, 12.5) &&
                    near(ramp.lowerLanding.centerMeters.z, -5.0) &&
                    near(ramp.upperLanding.centerMeters.z, -5.0),
                "yaw rotates low and high landings into the rise direction") &&
         expect(ramp.surfaceNormal.x < 0.0 && ramp.surfaceNormal.y > 0.0 &&
                    near(ramp.surfaceNormal.z, 0.0),
                "yaw rotates the collision normal with the visible ramp") &&
         expect(near(ramp.sideEdges[0].lowMeters.z, -4.0) &&
                    near(ramp.sideEdges[1].lowMeters.z, -6.0),
                "side edges rotate with the ramp frame");
}

bool unsafeOrMalformedRampsFailClosed() {
  cr::CreativeRampRecipeRequest steep = defaultRequest();
  steep.authoredBounds.min.z = -1.0;
  steep.authoredBounds.max.z = 1.0;
  const auto steepResult = cr::planCreativeRamp(steep);

  cr::CreativeRampRecipeRequest lowHeadroom = defaultRequest();
  lowHeadroom.availableHeadroomMeters = 1.5;
  const auto headroomResult = cr::planCreativeRamp(lowHeadroom);

  cr::CreativeRampRecipeRequest invalidMaterial = defaultRequest();
  invalidMaterial.material = cr::CreativeStructuralMaterial::Count;
  const auto materialResult = cr::planCreativeRamp(invalidMaterial);

  cr::CreativeRampRecipeRequest tilted = defaultRequest();
  tilted.transform.rotationEulerRadians.x = 0.1;
  const auto tiltedResult = cr::planCreativeRamp(tilted);

  cr::CreativeRampRecipeRequest nonFinite = defaultRequest();
  nonFinite.maximumWalkableSlopeDegrees =
      std::numeric_limits<double>::quiet_NaN();
  const auto nonFiniteResult = cr::planCreativeRamp(nonFinite);

  return expect(!steepResult.accepted &&
                    steepResult.status ==
                        cr::CreativeRampRecipeStatus::InvalidSlope,
                "runtime-blocked slope rejects before authoring") &&
         expect(!headroomResult.accepted &&
                    headroomResult.status ==
                        cr::CreativeRampRecipeStatus::InvalidHeadroom,
                "insufficient ramp headroom rejects") &&
         expect(!materialResult.accepted &&
                    materialResult.status ==
                        cr::CreativeRampRecipeStatus::InvalidMaterial,
                "invalid ramp material rejects") &&
         expect(!tiltedResult.accepted &&
                    tiltedResult.status ==
                        cr::CreativeRampRecipeStatus::InvalidTransform,
                "non-upright authored transform rejects") &&
         expect(!nonFiniteResult.accepted &&
                    nonFiniteResult.status ==
                        cr::CreativeRampRecipeStatus::InvalidDimensions,
                "non-finite movement policy rejects");
}

}  // namespace

int main() {
  const bool ok = acceptedRampOwnsTraversalGeometry() &&
                  directionRotationMovesEveryWorldFrame() &&
                  unsafeOrMalformedRampsFailClosed();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_ramp_recipe_tests: PASS\n";
  return EXIT_SUCCESS;
}
