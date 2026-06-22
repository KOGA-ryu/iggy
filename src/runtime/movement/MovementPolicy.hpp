#pragma once

#include <span>
#include <string_view>

#include "core/math/Vec3.hpp"
#include "runtime/movement/MovementParams.hpp"

namespace iggy3d {

struct SlopeBand {
  std::string_view id;
  float minDegrees = 0.0F;
  float maxDegrees = 0.0F;
  float speedMultiplier = 1.0F;
  float staminaCostMultiplier = 1.0F;
  float stepPenaltyMultiplier = 1.0F;
  bool carefulFooting = false;
};

struct SlopeSample {
  bool valid = false;
  bool walkable = false;
  float angleDegrees = 0.0F;
  float upDot = 0.0F;
  Vec3 normal;
  std::string_view bandId = "invalid";
  float speedMultiplier = 0.0F;
  float staminaCostMultiplier = 0.0F;
  float stepPenaltyMultiplier = 0.0F;
  bool carefulFooting = false;
};

std::span<const SlopeBand> defaultSlopeBands();
const SlopeBand& resolveSlopeBand(float slopeAngleDegrees,
                                  std::span<const SlopeBand> bands = defaultSlopeBands());
SlopeSample sampleSlope(Vec3 normal,
                        const MovementParams& params = {},
                        std::span<const SlopeBand> bands = defaultSlopeBands());

}  // namespace iggy3d
