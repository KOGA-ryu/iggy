#include "runtime/movement/MovementKinematics.hpp"

#include <cmath>

namespace iggy3d {
namespace {

inline constexpr float kTravelEpsilon = 0.0001F;
inline constexpr float kVerticalTravelEpsilon = 0.001F;

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

float horizontalDistance(const Vec3& start, const Vec3& finalPosition) {
  const float x = finalPosition.x - start.x;
  const float z = finalPosition.z - start.z;
  return std::sqrt(x * x + z * z);
}

MovementTravelDirection travelDirection(float horizontalDistanceMeters,
                                        float verticalDeltaMeters) {
  if (horizontalDistanceMeters <= kTravelEpsilon &&
      std::fabs(verticalDeltaMeters) <= kVerticalTravelEpsilon) {
    return MovementTravelDirection::Stationary;
  }
  if (std::fabs(verticalDeltaMeters) <= kVerticalTravelEpsilon) {
    return MovementTravelDirection::Contour;
  }
  return verticalDeltaMeters > 0.0F ? MovementTravelDirection::Uphill
                                    : MovementTravelDirection::Downhill;
}

}  // namespace

MovementTravelFacts computeMovementTravelFacts(const Vec3& start, const Vec3& finalPosition) {
  MovementTravelFacts facts;
  if (!isFinite(start) || !isFinite(finalPosition)) {
    return facts;
  }

  facts.distanceMeters = vectorLength(finalPosition - start);
  facts.horizontalDistanceMeters = horizontalDistance(start, finalPosition);
  facts.verticalDeltaMeters = finalPosition.y - start.y;
  if (!std::isfinite(facts.distanceMeters) ||
      !std::isfinite(facts.horizontalDistanceMeters) ||
      !std::isfinite(facts.verticalDeltaMeters)) {
    return MovementTravelFacts{};
  }

  facts.gradePercent = facts.horizontalDistanceMeters > kTravelEpsilon
                           ? (facts.verticalDeltaMeters / facts.horizontalDistanceMeters) *
                                 100.0F
                           : 0.0F;
  if (!std::isfinite(facts.gradePercent)) {
    facts.gradePercent = 0.0F;
  }
  facts.direction = travelDirection(facts.horizontalDistanceMeters, facts.verticalDeltaMeters);
  return facts;
}

const char* movementTravelDirectionName(MovementTravelDirection direction) {
  switch (direction) {
    case MovementTravelDirection::Stationary:
      return "stationary";
    case MovementTravelDirection::Contour:
      return "contour";
    case MovementTravelDirection::Uphill:
      return "uphill";
    case MovementTravelDirection::Downhill:
      return "downhill";
  }
  return "stationary";
}

}  // namespace iggy3d
