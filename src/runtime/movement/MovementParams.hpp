#pragma once

namespace iggy3d {

struct MovementParams {
  float maxSpeedMetersPerSecond = 3.0F;
  float radiusMeters = 0.30F;
  float heightMeters = 1.80F;
  float maxWalkableSlopeDegrees = 40.0F;
  float stepHeightMeters = 0.35F;
  float groundSnapMeters = 0.60F;
  float skinMeters = 0.02F;
};

}  // namespace iggy3d
