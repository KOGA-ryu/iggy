#include "app/iggy3d/creative/recipes/StairRecipe.hpp"

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

cr::CreativeStairRecipeRequest defaultRequest() {
  cr::CreativeStairRecipeRequest request;
  request.authoredBounds = {{-1.0, 0.0, -2.0}, {1.0, 3.0, 2.0}};
  request.transform.position = {0.0, 1.5, 0.0};
  return request;
}

bool defaultStairOwnsExactTraversalFacts() {
  const cr::CreativeStairRecipeResult result =
      cr::planCreativeStair(defaultRequest());
  return expect(result.accepted &&
                    result.status == cr::CreativeStairRecipeStatus::Ready,
                "default stair accepted") &&
         expect(result.stepCount == 12U && near(result.widthMeters, 2.0) &&
                    near(result.riseMeters, 3.0) &&
                    near(result.runMeters, 4.0) &&
                    near(result.riserHeightMeters, 0.25) &&
                    near(result.treadDepthMeters, 1.0 / 3.0),
                "stair schedule fits rise and run") &&
         expect(near(result.lowerLanding.centerMeters.z, -2.5) &&
                    near(result.lowerLanding.centerMeters.y, 0.0) &&
                    near(result.upperLanding.centerMeters.z, 2.5) &&
                    near(result.upperLanding.centerMeters.y, 3.0),
                "stair publishes both landing frames") &&
         expect(near(result.lowTreadCenterMeters.y, 0.25) &&
                    near(result.highTreadCenterMeters.y, 3.0),
                "stair publishes low and high traversal points") &&
         expect(result.socketCount == 4U &&
                    result.sockets[0].kind ==
                        cr::CreativeStairSocketKind::RailLeft &&
                    result.sockets[3].kind ==
                        cr::CreativeStairSocketKind::StringerRight &&
                    cr::creativeStairSocketCompatibility(
                        result.sockets[0].kind) == "stair.rail" &&
                    cr::creativeStairSocketCompatibility(
                        result.sockets[3].kind) == "stair.stringer",
                "stair exposes bounded rail and stringer receivers");
}

bool directionRotationMovesTraversalFrames() {
  cr::CreativeStairRecipeRequest request = defaultRequest();
  request.transform.rotationEulerRadians.y =
      3.14159265358979323846 * 0.5;
  const cr::CreativeStairRecipeResult result =
      cr::planCreativeStair(request);
  return expect(result.accepted, "quarter-turn stair accepted") &&
         expect(result.lowTreadCenterMeters.x < 0.0 &&
                    result.highTreadCenterMeters.x > 0.0 &&
                    near(result.lowTreadCenterMeters.z, 0.0) &&
                    near(result.highTreadCenterMeters.z, 0.0),
                "stair traversal points rotate with rise direction");
}

bool shippedEstateScaleFitsInsideTheExplicitCapacity() {
  cr::CreativeStairRecipeRequest estate = defaultRequest();
  estate.authoredBounds = {{-0.5, 0.0, -11.0}, {0.5, 22.0, 11.0}};
  estate.transform.position = {0.0, 11.0, 0.0};
  estate.availableHeadroomMeters = 22.0;
  const cr::CreativeStairRecipeResult accepted =
      cr::planCreativeStair(estate);

  cr::CreativeStairRecipeRequest excessive = estate;
  excessive.authoredBounds.max.y = 24.25;
  excessive.authoredBounds.min.z = -12.125;
  excessive.authoredBounds.max.z = 12.125;
  excessive.transform.position.y = 12.125;
  excessive.availableHeadroomMeters = 24.25;
  const cr::CreativeStairRecipeResult rejected =
      cr::planCreativeStair(excessive);

  return expect(accepted.accepted && accepted.stepCount == 88U &&
                    near(accepted.riserHeightMeters, 0.25) &&
                    near(accepted.treadDepthMeters, 0.25),
                "shipped estate stair fits the bounded safe schedule") &&
         expect(!rejected.accepted &&
                    rejected.status ==
                        cr::CreativeStairRecipeStatus::StepCapacityExceeded,
                "stairs beyond the explicit 96-step capacity reject");
}

bool impossibleGeometryFailsClosed() {
  cr::CreativeStairRecipeRequest shortRun = defaultRequest();
  shortRun.authoredBounds.min.z = -0.5;
  shortRun.authoredBounds.max.z = 0.5;
  const auto shortResult = cr::planCreativeStair(shortRun);

  cr::CreativeStairRecipeRequest lowHeadroom = defaultRequest();
  lowHeadroom.availableHeadroomMeters = 1.5;
  const auto headroomResult = cr::planCreativeStair(lowHeadroom);

  cr::CreativeStairRecipeRequest tilted = defaultRequest();
  tilted.transform.rotationEulerRadians.x = 0.1;
  const auto tiltedResult = cr::planCreativeStair(tilted);

  cr::CreativeStairRecipeRequest nonFinite = defaultRequest();
  nonFinite.minimumTreadDepthMeters =
      std::numeric_limits<double>::quiet_NaN();
  const auto nonFiniteResult = cr::planCreativeStair(nonFinite);

  return expect(!shortResult.accepted &&
                    shortResult.status ==
                        cr::CreativeStairRecipeStatus::InvalidDimensions,
                "short run rejects unsafe treads") &&
         expect(!headroomResult.accepted &&
                    headroomResult.status ==
                        cr::CreativeStairRecipeStatus::InvalidHeadroom,
                "insufficient headroom rejects") &&
         expect(!tiltedResult.accepted &&
                    tiltedResult.status ==
                        cr::CreativeStairRecipeStatus::InvalidTransform,
                "tilted stair transform rejects") &&
         expect(!nonFiniteResult.accepted &&
                    nonFiniteResult.status ==
                        cr::CreativeStairRecipeStatus::InvalidDimensions,
                "non-finite stair dimensions reject");
}

}  // namespace

int main() {
  const bool ok = defaultStairOwnsExactTraversalFacts() &&
                  directionRotationMovesTraversalFrames() &&
                  shippedEstateScaleFitsInsideTheExplicitCapacity() &&
                  impossibleGeometryFailsClosed();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_stair_recipe_tests: PASS\n";
  return EXIT_SUCCESS;
}
