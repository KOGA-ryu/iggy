#pragma once

#include <cstdint>
#include <type_traits>

#include "app/iggy3d/creative/document/Object.hpp"

namespace iggy3d::creative {

enum class CreativePlacementSurfaceFrameStatus : std::uint8_t {
  InvalidNormal,
  InvalidReference,
  InvalidLocalForward,
  InvalidRotation,
  DegenerateFrame,
  Ready,
};

struct CreativePlacementSurfaceFrameRequest {
  CreativeVec3 surfaceNormal{};
  CreativeVec3 placerForward{0.0, 0.0, -1.0};
  CreativeVec3 localForward{0.0, 0.0, 1.0};
  double rotationAroundNormalRadians = 0.0;
};

struct CreativePlacementSurfaceFramePlan {
  CreativePlacementSurfaceFrameStatus status =
      CreativePlacementSurfaceFrameStatus::InvalidNormal;
  CreativeVec3 surfaceNormal{};
  CreativeVec3 surfaceRight{};
  CreativeVec3 surfaceUp{};
  CreativeVec3 localRight{};
  CreativeVec3 localUp{};
  CreativeVec3 rotationEulerRadians{};
  bool usedPlacerFallback = false;
  bool valid = false;
};

static_assert(
    std::is_trivially_copyable_v<CreativePlacementSurfaceFrameRequest>);
static_assert(std::is_standard_layout_v<CreativePlacementSurfaceFrameRequest>);
static_assert(std::is_trivially_copyable_v<CreativePlacementSurfaceFramePlan>);
static_assert(std::is_standard_layout_v<CreativePlacementSurfaceFramePlan>);

// Maps localForward onto the surface normal. Local up follows projected world
// up, falling back to placer-facing on horizontal surfaces.
[[nodiscard]] CreativePlacementSurfaceFramePlan
resolveCreativePlacementSurfaceFrame(
    const CreativePlacementSurfaceFrameRequest& request) noexcept;

}  // namespace iggy3d::creative
