#pragma once

#include <cstdint>

#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d::creative {

enum class CreativeScreenProjectionStatus : std::uint8_t {
  NotRequested,
  Projected,
  PartiallyProjected,
  InvalidViewport,
  InvalidMatrix,
  InvalidWorldPoint,
  InvalidWorldBounds,
  BehindCamera,
  DegenerateClipW,
  NonFiniteProjection,
  NoProjectedCorners,
  Count,
};

struct CreativeScreenPoint {
  float x = 0.0F;
  float y = 0.0F;
  float ndcDepth = 0.0F;
  float clipW = 0.0F;
  CreativeScreenProjectionStatus status =
      CreativeScreenProjectionStatus::NotRequested;
  bool valid = false;
  bool insideViewport = false;
};

struct CreativeScreenBounds {
  float minX = 0.0F;
  float minY = 0.0F;
  float maxX = 0.0F;
  float maxY = 0.0F;
  CreativeScreenProjectionStatus status =
      CreativeScreenProjectionStatus::NotRequested;
  std::uint8_t projectedCornerCount = 0U;
  bool valid = false;
  bool allCornersProjected = false;
  bool intersectsViewport = false;
};

inline constexpr float kCreativeMinimumPositiveClipW = 1.0e-6F;

[[nodiscard]] CreativeScreenPoint projectCreativeWorldPointToScreen(
    const Mat4& clipFromWorld,
    Vec3 world,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight) noexcept;

// Projects the eight corners without clipping edges against the near plane.
// A bounds result can therefore be valid and partially projected.
[[nodiscard]] CreativeScreenBounds projectCreativeWorldBoundsToScreen(
    const Mat4& clipFromWorld,
    Vec3 worldMin,
    Vec3 worldMax,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight) noexcept;

}  // namespace iggy3d::creative
