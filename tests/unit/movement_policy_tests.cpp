#include "runtime/movement/MovementPolicy.hpp"

#include <cmath>
#include <iostream>
#include <span>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool approx(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::Vec3 slopeNormal(float degrees) {
  const float radians = degrees * 3.14159265358979323846F / 180.0F;
  return {0.0F, std::cos(radians), -std::sin(radians)};
}

bool defaultBandsHaveEditablePolicyShape() {
  const std::span<const iggy3d::SlopeBand> bands = iggy3d::defaultSlopeBands();
  return expect(bands.size() == 5U, "slope band count") &&
         expect(bands[0].id == "flat", "flat band") &&
         expect(bands[1].id == "easy", "easy band") &&
         expect(bands[2].id == "moderate", "moderate band") &&
         expect(bands[3].id == "steep", "steep band") &&
         expect(bands[4].id == "blocked", "blocked band") &&
         expect(approx(bands[2].speedMultiplier, 0.75F), "moderate speed multiplier") &&
         expect(bands[2].carefulFooting, "moderate careful footing");
}

bool slopeSamplesResolveBandsAndDots() {
  const iggy3d::MovementParams params;
  const iggy3d::SlopeSample flat = iggy3d::sampleSlope({0.0F, 1.0F, 0.0F}, params);
  const iggy3d::SlopeSample moderate = iggy3d::sampleSlope(slopeNormal(20.0F), params);
  const iggy3d::SlopeSample blocked = iggy3d::sampleSlope(slopeNormal(45.0F), params);
  return expect(flat.valid && flat.walkable, "flat walkable") &&
         expect(flat.bandId == "flat", "flat id") &&
         expect(approx(flat.angleDegrees, 0.0F), "flat angle") &&
         expect(moderate.valid && moderate.walkable, "moderate walkable") &&
         expect(moderate.bandId == "moderate", "moderate id") &&
         expect(approx(moderate.speedMultiplier, 0.75F), "moderate multiplier") &&
         expect(blocked.valid && !blocked.walkable, "blocked rejected") &&
         expect(blocked.bandId == "blocked", "blocked id") &&
         expect(approx(blocked.speedMultiplier, 0.0F), "blocked multiplier");
}

bool maxSlopeCanRejectHardcodedBand() {
  iggy3d::MovementParams params;
  params.maxWalkableSlopeDegrees = 25.0F;
  const iggy3d::SlopeSample steep = iggy3d::sampleSlope(slopeNormal(30.0F), params);
  return expect(steep.valid, "steep valid") && expect(steep.bandId == "steep", "steep band") &&
         expect(!steep.walkable, "max slope rejects steep");
}

}  // namespace

int main() {
  const bool ok = defaultBandsHaveEditablePolicyShape() && slopeSamplesResolveBandsAndDots() &&
                  maxSlopeCanRejectHardcodedBand();
  return ok ? 0 : 1;
}
