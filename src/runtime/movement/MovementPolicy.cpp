#include "runtime/movement/MovementPolicy.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr std::array<SlopeBand, 5> kDefaultSlopeBands{{
    {"flat", 0.0F, 5.0F, 1.00F, 1.00F, 1.00F, false},
    {"easy", 5.0F, 15.0F, 0.92F, 1.10F, 1.05F, false},
    {"moderate", 15.0F, 28.0F, 0.75F, 1.35F, 1.20F, true},
    {"steep", 28.0F, 40.0F, 0.45F, 1.80F, 1.60F, true},
    {"blocked", 40.0F, 90.0F, 0.00F, 0.00F, 0.00F, true},
}};

const SlopeBand& blockedBand() {
  return kDefaultSlopeBands.back();
}

float length(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

bool normalized(Vec3 value, Vec3& out) {
  if (!isFinite(value)) {
    return false;
  }
  const float magnitude = length(value);
  if (!std::isfinite(magnitude) || magnitude <= 0.000001F) {
    return false;
  }
  out = value / magnitude;
  return isFinite(out);
}

float degreesFromUpDot(float upDot) {
  const float clamped = std::clamp(upDot, -1.0F, 1.0F);
  return std::acos(clamped) * 180.0F / kPi;
}

}  // namespace

std::span<const SlopeBand> defaultSlopeBands() {
  return kDefaultSlopeBands;
}

const SlopeBand& resolveSlopeBand(float slopeAngleDegrees, std::span<const SlopeBand> bands) {
  if (!std::isfinite(slopeAngleDegrees) || bands.empty()) {
    return blockedBand();
  }
  for (const SlopeBand& band : bands) {
    if (slopeAngleDegrees >= band.minDegrees && slopeAngleDegrees < band.maxDegrees) {
      return band;
    }
  }
  return bands.back();
}

SlopeSample sampleSlope(Vec3 normal, const MovementParams& params, std::span<const SlopeBand> bands) {
  SlopeSample sample;
  Vec3 normalizedNormal;
  if (!normalized(normal, normalizedNormal) || !std::isfinite(params.maxWalkableSlopeDegrees)) {
    sample.bandId = "invalid";
    return sample;
  }

  sample.valid = true;
  sample.normal = normalizedNormal;
  sample.upDot = dot(normalizedNormal, vec3UnitY());
  sample.angleDegrees = degreesFromUpDot(sample.upDot);

  const SlopeBand& band = resolveSlopeBand(sample.angleDegrees, bands);
  sample.bandId = band.id;
  sample.speedMultiplier = band.speedMultiplier;
  sample.staminaCostMultiplier = band.staminaCostMultiplier;
  sample.stepPenaltyMultiplier = band.stepPenaltyMultiplier;
  sample.carefulFooting = band.carefulFooting;
  sample.walkable = sample.angleDegrees <= params.maxWalkableSlopeDegrees &&
                    band.speedMultiplier > 0.0F && sample.upDot > 0.0F;
  return sample;
}

}  // namespace iggy3d
