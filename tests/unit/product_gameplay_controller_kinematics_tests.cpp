#include "app/iggy3d/gameplay/ControllerKinematics.hpp"

#include "ProductTestSupport.hpp"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <string_view>

namespace {

using iggy3d::ProductGameplayMovementTuning;
using iggy3d::Vec3;
using iggy3d::test::expect;
using iggy3d::test::nearlyEqual;

bool vecNearlyEqual(Vec3 lhs, Vec3 rhs, float epsilon = 0.0001F) {
  return nearlyEqual(lhs.x, rhs.x, epsilon) &&
         nearlyEqual(lhs.y, rhs.y, epsilon) &&
         nearlyEqual(lhs.z, rhs.z, epsilon);
}

bool yawZeroForwardMapsToNegativeZ() {
  const Vec3 direction =
      iggy3d::productManualFirstPersonDirection(0.0F, 1.0F, 0.0F);
  return expect(vecNearlyEqual(direction, {0.0F, 0.0F, -1.0F}),
                "yaw 0 forward maps to negative z");
}

bool yawNinetyForwardMapsToPositiveX() {
  const Vec3 direction =
      iggy3d::productManualFirstPersonDirection(0.0F, 1.0F, 90.0F);
  return expect(vecNearlyEqual(direction, {1.0F, 0.0F, 0.0F}),
                "yaw 90 forward maps to positive x");
}

bool diagonalInputNormalizesToUnitHorizontalDirection() {
  const float invSqrt2 = 1.0F / std::sqrt(2.0F);
  const Vec3 direction =
      iggy3d::productManualFirstPersonDirection(1.0F, 1.0F, 0.0F);
  const float length =
      std::sqrt(direction.x * direction.x + direction.z * direction.z);
  return expect(nearlyEqual(length, 1.0F), "diagonal direction is unit length") &&
         expect(vecNearlyEqual(direction, {invSqrt2, 0.0F, -invSqrt2}),
                "diagonal direction preserves normalized x/z");
}

bool zeroInputReturnsCameraForwardDirection() {
  const float invSqrt2 = 1.0F / std::sqrt(2.0F);
  const Vec3 direction =
      iggy3d::productManualFirstPersonDirection(0.0F, 0.0F, 45.0F);
  return expect(vecNearlyEqual(direction, {invSqrt2, 0.0F, -invSqrt2}),
                "zero input returns camera forward direction");
}

bool sprintWalkMaxSpeedAndProfileUseTuningFields() {
  ProductGameplayMovementTuning tuning;
  tuning.walkSpeedMetersPerSecond = 2.5F;
  tuning.sprintSpeedMetersPerSecond = 7.25F;
  tuning.walkProfile = "walk_profile";
  tuning.sprintProfile = "sprint_profile";

  return expect(nearlyEqual(iggy3d::productManualFirstPersonMaxSpeedMetersPerSecond(
                                tuning, false),
                            2.5F),
                "walk speed uses tuning") &&
         expect(nearlyEqual(iggy3d::productManualFirstPersonMaxSpeedMetersPerSecond(
                                tuning, true),
                            7.25F),
                "sprint speed uses tuning") &&
         expect(iggy3d::productManualFirstPersonMovementProfile(tuning, false) ==
                    std::string_view{"walk_profile"},
                "walk profile uses tuning") &&
         expect(iggy3d::productManualFirstPersonMovementProfile(tuning, true) ==
                    std::string_view{"sprint_profile"},
                "sprint profile uses tuning");
}

bool moveDeltaScalesBySpeedStepResponseAndDiagonalInput() {
  ProductGameplayMovementTuning tuning;
  tuning.walkSpeedMetersPerSecond = 6.0F;
  tuning.inputStepSeconds = 0.5F;
  const float responseMultiplier = 0.25F;
  const float expectedAxis =
      tuning.walkSpeedMetersPerSecond * tuning.inputStepSeconds *
      responseMultiplier / std::sqrt(2.0F);

  const Vec3 delta = iggy3d::productManualFirstPersonMoveDelta(
      1.0F, 1.0F, 0.0F, false, tuning, responseMultiplier);

  return expect(vecNearlyEqual(delta, {expectedAxis, 0.0F, -expectedAxis}),
                "move delta scales speed step response and diagonal input");
}

bool desiredVelocityZeroWithoutMovement() {
  ProductGameplayMovementTuning tuning;
  tuning.walkSpeedMetersPerSecond = 5.0F;
  const Vec3 velocity = iggy3d::productManualFirstPersonDesiredVelocity(
      0.0F, 0.0F, 15.0F, false, tuning);
  return expect(vecNearlyEqual(velocity, {}),
                "no movement has zero desired velocity");
}

bool desiredVelocityPreservesNormalizedDoubleAxisSpeed() {
  ProductGameplayMovementTuning tuning;
  tuning.walkSpeedMetersPerSecond = 10.0F;
  const float expectedAxis = tuning.walkSpeedMetersPerSecond / std::sqrt(2.0F);
  const Vec3 velocity = iggy3d::productManualFirstPersonDesiredVelocity(
      1.0F, 1.0F, 0.0F, false, tuning);
  const float speed =
      std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
  return expect(nearlyEqual(speed, tuning.walkSpeedMetersPerSecond),
                "desired velocity preserves normalized speed") &&
         expect(vecNearlyEqual(velocity, {expectedAxis, 0.0F, -expectedAxis}),
                "desired velocity keeps diagonal x/z speed");
}

bool horizontalVelocityMoveReachesTargetWhenDeltaIsLargeEnough() {
  const Vec3 target{3.0F, 0.0F, 4.0F};
  const Vec3 result =
      iggy3d::moveProductHorizontalVelocityToward({}, target, 5.0F);
  return expect(vecNearlyEqual(result, target),
                "horizontal velocity reaches target when max delta is large enough");
}

bool horizontalVelocityMoveAdvancesPartiallyWhenDeltaIsSmaller() {
  const Vec3 result = iggy3d::moveProductHorizontalVelocityToward(
      {}, {3.0F, 0.0F, 4.0F}, 2.5F);
  return expect(vecNearlyEqual(result, {1.5F, 0.0F, 2.0F}),
                "horizontal velocity advances partially by max delta");
}

bool horizontalVelocityMoveReturnsTargetForNonFiniteDistance() {
  const float inf = std::numeric_limits<float>::infinity();
  const Vec3 target{1.0F, 7.0F, 2.0F};
  const Vec3 result = iggy3d::moveProductHorizontalVelocityToward(
      {inf, 0.0F, 0.0F}, target, 0.5F);
  return expect(vecNearlyEqual(result, target),
                "horizontal velocity returns target for non-finite distance");
}

bool clampHorizontalVelocityPreservesSubLimitVelocity() {
  const Vec3 velocity{3.0F, 2.0F, 4.0F};
  const Vec3 result = iggy3d::clampProductHorizontalVelocity(velocity, 6.0F);
  return expect(vecNearlyEqual(result, velocity),
                "horizontal velocity clamp preserves sub-limit velocity");
}

bool clampHorizontalVelocityClampsOverLimitXZSpeed() {
  const Vec3 result =
      iggy3d::clampProductHorizontalVelocity({6.0F, 2.0F, 8.0F}, 5.0F);
  const float speed = std::sqrt(result.x * result.x + result.z * result.z);
  return expect(nearlyEqual(speed, 5.0F),
                "horizontal velocity clamp limits x/z speed") &&
         expect(vecNearlyEqual(result, {3.0F, 0.0F, 4.0F}),
                "horizontal velocity clamp scales x/z axes");
}

}  // namespace

int main() {
  const bool ok =
      yawZeroForwardMapsToNegativeZ() && yawNinetyForwardMapsToPositiveX() &&
      diagonalInputNormalizesToUnitHorizontalDirection() &&
      zeroInputReturnsCameraForwardDirection() &&
      sprintWalkMaxSpeedAndProfileUseTuningFields() &&
      moveDeltaScalesBySpeedStepResponseAndDiagonalInput() &&
      desiredVelocityZeroWithoutMovement() &&
      desiredVelocityPreservesNormalizedDoubleAxisSpeed() &&
      horizontalVelocityMoveReachesTargetWhenDeltaIsLargeEnough() &&
      horizontalVelocityMoveAdvancesPartiallyWhenDeltaIsSmaller() &&
      horizontalVelocityMoveReturnsTargetForNonFiniteDistance() &&
      clampHorizontalVelocityPreservesSubLimitVelocity() &&
      clampHorizontalVelocityClampsOverLimitXZSpeed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
