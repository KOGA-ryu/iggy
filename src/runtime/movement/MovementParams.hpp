#pragma once

#include "runtime/movement/MovementDefaults.hpp"

#include <cstdint>

namespace iggy3d {

struct MovementParams {
  float maxSpeedMetersPerSecond =
      static_cast<float>(kDefaultPlayerMoveSpeedMetersPerSecond);
  float radiusMeters = static_cast<float>(kDefaultPlayerBodyRadiusMeters);
  float heightMeters = static_cast<float>(kDefaultPlayerStandingHeightMeters);
  float maxWalkableSlopeDegrees = kDefaultMaxWalkableSlopeDegrees;
  float stepHeightMeters = static_cast<float>(kDefaultPlayerStepHeightMeters);
  float groundSnapMeters = static_cast<float>(kDefaultPlayerGroundSnapMeters);
  float skinMeters = static_cast<float>(kDefaultPlayerSkinMeters);

  // Clamber (flow feat v1) -- THE numeric truth for the mechanic; the
  // stealth_blockout kit README cites these fields by name. Band bottom is
  // the autoStep ceiling and is EXCLUSIVE (a top edge <= stepHeightMeters is
  // walking, never clamber); band top is INCLUSIVE (a ledge at exactly
  // heightMeters engages, anything above refuses).
  float clamberBandBottomMeters =
      static_cast<float>(kDefaultPlayerStepHeightMeters);
  float clamberBandTopMeters =
      static_cast<float>(kDefaultPlayerStandingHeightMeters);
  float clamberMaxReachMeters = 0.55F;    // horizontal gap to the ledge face
  std::uint32_t clamberDurationTicks = 8U;  // fixed-tick phase length
  // Engage noise for the per-tick sound bus; guards hear it through the same
  // attenuation kernel as footsteps (footstep base is 24 dB -- a clamber is
  // deliberately louder). Armour/surface multipliers are a later seam.
  float clamberLoudnessDb = 30.0F;
};

}  // namespace iggy3d
