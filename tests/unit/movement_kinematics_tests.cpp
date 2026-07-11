#include "runtime/movement/MovementKinematics.hpp"

#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool travelFactsClassifyStationaryContourAndGrade() {
  const iggy3d::MovementTravelFacts stationary =
      iggy3d::computeMovementTravelFacts({0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F});
  const iggy3d::MovementTravelFacts contour =
      iggy3d::computeMovementTravelFacts({0.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F});
  const iggy3d::MovementTravelFacts uphill =
      iggy3d::computeMovementTravelFacts({0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 4.0F});
  const iggy3d::MovementTravelFacts downhill =
      iggy3d::computeMovementTravelFacts({0.0F, 2.0F, 0.0F}, {0.0F, 1.0F, 4.0F});

  return expect(stationary.direction == iggy3d::MovementTravelDirection::Stationary,
                "stationary direction") &&
         expect(stationary.distanceMeters == 0.0F, "stationary distance") &&
         expect(contour.direction == iggy3d::MovementTravelDirection::Contour,
                "contour direction") &&
         expect(contour.horizontalDistanceMeters > 1.99F &&
                    contour.horizontalDistanceMeters < 2.01F,
                "contour horizontal distance") &&
         expect(contour.gradePercent == 0.0F, "contour grade") &&
         expect(uphill.direction == iggy3d::MovementTravelDirection::Uphill,
                "uphill direction") &&
         expect(uphill.verticalDeltaMeters > 0.99F && uphill.verticalDeltaMeters < 1.01F,
                "uphill vertical delta") &&
         expect(uphill.gradePercent > 24.9F && uphill.gradePercent < 25.1F,
                "uphill grade") &&
         expect(downhill.direction == iggy3d::MovementTravelDirection::Downhill,
                "downhill direction") &&
         expect(downhill.verticalDeltaMeters < -0.99F &&
                    downhill.verticalDeltaMeters > -1.01F,
                "downhill vertical delta") &&
         expect(downhill.gradePercent < -24.9F && downhill.gradePercent > -25.1F,
                "downhill grade");
}

bool nonFiniteInputReturnsStationaryZeroFacts() {
  const float inf = std::numeric_limits<float>::infinity();
  const iggy3d::MovementTravelFacts facts =
      iggy3d::computeMovementTravelFacts({0.0F, 0.0F, 0.0F}, {inf, 1.0F, 0.0F});
  return expect(facts.direction == iggy3d::MovementTravelDirection::Stationary,
                "nonfinite stationary") &&
         expect(facts.distanceMeters == 0.0F, "nonfinite distance") &&
         expect(facts.horizontalDistanceMeters == 0.0F, "nonfinite horizontal") &&
         expect(facts.verticalDeltaMeters == 0.0F, "nonfinite vertical") &&
         expect(facts.gradePercent == 0.0F, "nonfinite grade");
}

bool directionNamesAreStableReceiptTokens() {
  return expect(std::string_view(iggy3d::movementTravelDirectionName(
                    iggy3d::MovementTravelDirection::Stationary)) == "stationary",
                "stationary name") &&
         expect(std::string_view(iggy3d::movementTravelDirectionName(
                    iggy3d::MovementTravelDirection::Contour)) == "contour",
                "contour name") &&
         expect(std::string_view(iggy3d::movementTravelDirectionName(
                    iggy3d::MovementTravelDirection::Uphill)) == "uphill",
                "uphill name") &&
         expect(std::string_view(iggy3d::movementTravelDirectionName(
                    iggy3d::MovementTravelDirection::Downhill)) == "downhill",
                "downhill name");
}

}  // namespace

int main() {
  const bool ok = travelFactsClassifyStationaryContourAndGrade() &&
                  nonFiniteInputReturnsStationaryZeroFacts() &&
                  directionNamesAreStableReceiptTokens();
  return ok ? 0 : 1;
}
