#include "app/iggy3d/map_maker/Grid.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace iggy3d {
namespace {

constexpr std::array<float, 4U> kGridPitches{1.0F, 0.5F, 0.25F, 0.1F};
constexpr float kEpsilon = 0.0001F;

bool finitePositive(float value) {
  return std::isfinite(value) && value > kEpsilon;
}

bool finiteVec3(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

float snapDown(float value, float pitch) {
  return std::floor(value / pitch) * pitch;
}

float snapUp(float value, float pitch) {
  return std::ceil(value / pitch) * pitch;
}

bool nearMultiple(float value, float step) {
  const float scaled = value / step;
  return std::fabs(scaled - std::round(scaled)) <= 0.001F;
}

std::size_t nearestPitchIndex(float pitchMeters) {
  std::size_t best = 0U;
  float bestDistance = std::fabs(pitchMeters - kGridPitches[0]);
  for (std::size_t index = 1U; index < kGridPitches.size(); ++index) {
    const float distance = std::fabs(pitchMeters - kGridPitches[index]);
    // branch-gate: BG-1205
    if (distance < bestDistance) {
      best = index;
      bestDistance = distance;
    }
  }
  return best;
}

}  // namespace

bool isValidProductMapMakerGridConfig(const ProductMapMakerGridConfig& config) {
  return finitePositive(config.pitchMeters) &&
         finitePositive(config.majorStepMeters) &&
         finitePositive(config.extentXMeters) &&
         finitePositive(config.extentZMeters) &&
         std::isfinite(config.planeY) &&
         finiteVec3(config.anchorWorld);
}

std::string_view productMapMakerGridStatusName(ProductMapMakerGridStatus status) {
  // branch-gate: BG-1205
  switch (status) {
    case ProductMapMakerGridStatus::Ready:
      return "map_maker_grid_ready";
    case ProductMapMakerGridStatus::Disabled:
      return "map_maker_grid_disabled";
    case ProductMapMakerGridStatus::InvalidConfig:
      return "map_maker_grid_invalid_config";
  }
  return "map_maker_grid_invalid_config";
}

ProductMapMakerGridSnapshot buildProductMapMakerGridSnapshot(
    const ProductMapMakerGridConfig& config) {
  ProductMapMakerGridSnapshot snapshot;
  snapshot.pitchMeters = config.pitchMeters;
  snapshot.majorStepMeters = config.majorStepMeters;
  snapshot.planeY = config.planeY;

  // branch-gate: BG-1205
  if (!config.enabled) {
    snapshot.status = ProductMapMakerGridStatus::Disabled;
    snapshot.reasonCode = std::string(productMapMakerGridStatusName(snapshot.status));
    return snapshot;
  }

  // branch-gate: BG-1205
  if (!isValidProductMapMakerGridConfig(config)) {
    snapshot.status = ProductMapMakerGridStatus::InvalidConfig;
    snapshot.reasonCode = std::string(productMapMakerGridStatusName(snapshot.status));
    return snapshot;
  }

  const float halfX = config.extentXMeters * 0.5F;
  const float halfZ = config.extentZMeters * 0.5F;
  const float minX = snapDown(config.anchorWorld.x - halfX, config.pitchMeters);
  const float maxX = snapUp(config.anchorWorld.x + halfX, config.pitchMeters);
  const float minZ = snapDown(config.anchorWorld.z - halfZ, config.pitchMeters);
  const float maxZ = snapUp(config.anchorWorld.z + halfZ, config.pitchMeters);

  for (float z = minZ; z <= maxZ + kEpsilon; z += config.pitchMeters) {
    for (float x = minX; x <= maxX + kEpsilon; x += config.pitchMeters) {
      ProductMapMakerGridDot dot;
      dot.worldPosition = {x, config.planeY, z};
      dot.major = nearMultiple(x, config.majorStepMeters) &&
                  nearMultiple(z, config.majorStepMeters);
      snapshot.dots.push_back(dot);
      ++snapshot.dotCount;
      // branch-gate: BG-1205
      if (dot.major) {
        ++snapshot.majorDotCount;
      }
    }
  }

  snapshot.ok = true;
  snapshot.visible = !snapshot.dots.empty();
  snapshot.status = ProductMapMakerGridStatus::Ready;
  snapshot.reasonCode = std::string(productMapMakerGridStatusName(snapshot.status));
  return snapshot;
}

float nextProductMapMakerGridPitch(float currentPitchMeters) {
  const std::size_t index = nearestPitchIndex(currentPitchMeters);
  return kGridPitches[(index + 1U) % kGridPitches.size()];
}

float previousProductMapMakerGridPitch(float currentPitchMeters) {
  const std::size_t index = nearestPitchIndex(currentPitchMeters);
  return kGridPitches[(index + kGridPitches.size() - 1U) % kGridPitches.size()];
}

}  // namespace iggy3d
