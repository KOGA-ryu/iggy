#include "core/geom/StairMesh.hpp"

#include <cmath>

namespace iggy3d {

namespace {

[[nodiscard]] bool positiveFinite(float value) noexcept {
  return std::isfinite(value) && value > 0.0F;
}

}  // namespace

StairMesh generateStairs(const StairSpec& spec) {
  StairMesh mesh;

  if (!positiveFinite(spec.width) || !positiveFinite(spec.totalRise) ||
      !positiveFinite(spec.totalRun)) {
    mesh.reasonCode = "stair_invalid_dimensions";
    return mesh;
  }
  if (spec.stepCount < 1U || spec.stepCount > kMaxStairSteps) {
    mesh.reasonCode = "stair_invalid_step_count";
    return mesh;
  }

  const float stepRise = spec.totalRise / static_cast<float>(spec.stepCount);
  const float stepRun = spec.totalRun / static_cast<float>(spec.stepCount);

  mesh.steps.reserve(spec.stepCount);
  for (std::uint32_t i = 0; i < spec.stepCount; ++i) {
    const float zMin = static_cast<float>(i) * stepRun;
    const float zMax = static_cast<float>(i + 1U) * stepRun;
    const float yMax = static_cast<float>(i + 1U) * stepRise;
    mesh.steps.push_back(makeAabb3(Vec3{0.0F, 0.0F, zMin},
                                   Vec3{spec.width, yMax, zMax}));
  }

  mesh.ok = true;
  mesh.reasonCode = "stair_ok";
  mesh.stepCount = spec.stepCount;
  mesh.stepRise = stepRise;
  mesh.stepRun = stepRun;
  return mesh;
}

}  // namespace iggy3d
