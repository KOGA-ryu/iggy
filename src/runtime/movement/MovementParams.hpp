#pragma once

#include <cstdint>

namespace iggy3d {

struct MovementParams {
  float maxSpeedMetersPerSecond = 3.0F;
  float radiusMeters = 0.30F;
  float heightMeters = 1.80F;
  float maxWalkableSlopeDegrees = 40.0F;
  float stepHeightMeters = 0.35F;
  float groundSnapMeters = 0.60F;
  float skinMeters = 0.02F;

  // Clamber (flow feat v1) -- THE numeric truth for the mechanic; the
  // stealth_blockout kit README cites these fields by name. Band bottom is
  // the autoStep ceiling and is EXCLUSIVE (a top edge <= stepHeightMeters is
  // walking, never clamber); band top is INCLUSIVE (a ledge at exactly
  // heightMeters engages, anything above refuses).
  float clamberBandBottomMeters = 0.35F;  // == stepHeightMeters
  float clamberBandTopMeters = 1.80F;     // == heightMeters
  float clamberMaxReachMeters = 0.55F;    // horizontal gap to the ledge face
  std::uint32_t clamberDurationTicks = 8U;  // fixed-tick phase length
  // Engage noise for the per-tick sound bus; guards hear it through the same
  // attenuation kernel as footsteps (footstep base is 24 dB -- a clamber is
  // deliberately louder). Armour/surface multipliers are a later seam.
  float clamberLoudnessDb = 30.0F;
};

}  // namespace iggy3d
