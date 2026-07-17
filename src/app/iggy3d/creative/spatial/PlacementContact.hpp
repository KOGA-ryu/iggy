#pragma once

#include <cstdint>

#include "app/iggy3d/creative/document/Object.hpp"

namespace iggy3d::creative {

enum class CreativePlacementContactStatus : std::uint8_t {
  InvalidSource,
  InvalidTarget,
  Ready,
};

struct CreativePlacementContactRequest {
  CreativeTransformedBounds source{};
  CreativeVec3 targetPoint{};
  CreativeVec3 targetNormal{};
};

struct CreativePlacementContactPlan {
  CreativePlacementContactStatus status =
      CreativePlacementContactStatus::InvalidSource;
  CreativeVec3 targetPoint{};
  CreativeVec3 sourcePointBeforeTranslation{};
  CreativeVec3 sourcePointAfterTranslation{};
  CreativeVec3 normal{};
  CreativeVec3 translation{};
  double minimumSignedDistanceMeters = 0.0;
  double maximumSignedDistanceMeters = 0.0;
  std::uint8_t sourceFeatureVertexCount = 0U;
  bool valid = false;
};

// Aligns the source's extreme feature to targetPoint. The resulting
// translation keeps every source corner on or outside the target plane.
[[nodiscard]] CreativePlacementContactPlan resolveCreativePlacementContact(
    const CreativePlacementContactRequest& request) noexcept;

}  // namespace iggy3d::creative
