#pragma once

#include <cstdint>

#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class MovementTravelDirection : std::uint8_t {
  Stationary,
  Contour,
  Uphill,
  Downhill,
};

struct MovementTravelFacts {
  float distanceMeters = 0.0F;
  float horizontalDistanceMeters = 0.0F;
  float verticalDeltaMeters = 0.0F;
  float gradePercent = 0.0F;
  MovementTravelDirection direction = MovementTravelDirection::Stationary;
};

MovementTravelFacts computeMovementTravelFacts(const Vec3& start, const Vec3& finalPosition);
const char* movementTravelDirectionName(MovementTravelDirection direction);

}  // namespace iggy3d
