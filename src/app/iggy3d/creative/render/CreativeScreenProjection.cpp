#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool validViewport(std::uint32_t width,
                                 std::uint32_t height) noexcept {
  return width > 0U && height > 0U;
}

[[nodiscard]] bool validWorldBounds(Vec3 minimum, Vec3 maximum) noexcept {
  return isFinite(minimum) && isFinite(maximum) && minimum.x <= maximum.x &&
         minimum.y <= maximum.y && minimum.z <= maximum.z;
}

[[nodiscard]] CreativeScreenPoint projectValidatedPoint(
    const Mat4& clipFromWorld,
    Vec3 world,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight) noexcept {
  CreativeScreenPoint result;
  const ProjectedPoint3 projected = projectPoint(clipFromWorld, world);
  result.clipW = projected.w;
  result.ndcDepth = projected.ndc.z;
  if (!std::isfinite(projected.w)) {
    result.status = CreativeScreenProjectionStatus::NonFiniteProjection;
    return result;
  }
  if (projected.w <= 0.0F) {
    result.status = CreativeScreenProjectionStatus::BehindCamera;
    return result;
  }
  if (projected.w <= kCreativeMinimumPositiveClipW) {
    result.status = CreativeScreenProjectionStatus::DegenerateClipW;
    return result;
  }
  if (!projected.finite || !isFinite(projected.ndc)) {
    result.status = CreativeScreenProjectionStatus::NonFiniteProjection;
    return result;
  }

  result.x = (projected.ndc.x * 0.5F + 0.5F) *
             static_cast<float>(viewportWidth);
  result.y = (1.0F - (projected.ndc.y * 0.5F + 0.5F)) *
             static_cast<float>(viewportHeight);
  if (!std::isfinite(result.x) || !std::isfinite(result.y)) {
    result.status = CreativeScreenProjectionStatus::NonFiniteProjection;
    return result;
  }
  result.status = CreativeScreenProjectionStatus::Projected;
  result.valid = true;
  result.insideViewport =
      result.x >= 0.0F && result.y >= 0.0F &&
      result.x <= static_cast<float>(viewportWidth) &&
      result.y <= static_cast<float>(viewportHeight);
  return result;
}

}  // namespace

static_assert(std::is_trivially_copyable_v<CreativeScreenPoint>);
static_assert(std::is_trivially_copyable_v<CreativeScreenBounds>);
static_assert(sizeof(CreativeScreenPoint) <= 24U);
static_assert(sizeof(CreativeScreenBounds) <= 24U);

CreativeScreenPoint projectCreativeWorldPointToScreen(
    const Mat4& clipFromWorld,
    Vec3 world,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight) noexcept {
  CreativeScreenPoint result;
  if (!validViewport(viewportWidth, viewportHeight)) {
    result.status = CreativeScreenProjectionStatus::InvalidViewport;
    return result;
  }
  if (!isFinite(clipFromWorld)) {
    result.status = CreativeScreenProjectionStatus::InvalidMatrix;
    return result;
  }
  if (!isFinite(world)) {
    result.status = CreativeScreenProjectionStatus::InvalidWorldPoint;
    return result;
  }
  return projectValidatedPoint(clipFromWorld, world, viewportWidth,
                               viewportHeight);
}

CreativeScreenBounds projectCreativeWorldBoundsToScreen(
    const Mat4& clipFromWorld,
    Vec3 worldMin,
    Vec3 worldMax,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight) noexcept {
  CreativeScreenBounds result;
  if (!validViewport(viewportWidth, viewportHeight)) {
    result.status = CreativeScreenProjectionStatus::InvalidViewport;
    return result;
  }
  if (!isFinite(clipFromWorld)) {
    result.status = CreativeScreenProjectionStatus::InvalidMatrix;
    return result;
  }
  if (!validWorldBounds(worldMin, worldMax)) {
    result.status = CreativeScreenProjectionStatus::InvalidWorldBounds;
    return result;
  }

  float minimumX = std::numeric_limits<float>::max();
  float minimumY = std::numeric_limits<float>::max();
  float maximumX = std::numeric_limits<float>::lowest();
  float maximumY = std::numeric_limits<float>::lowest();
  for (std::uint32_t corner = 0U; corner < 8U; ++corner) {
    const Vec3 world{
        (corner & 1U) != 0U ? worldMax.x : worldMin.x,
        (corner & 2U) != 0U ? worldMax.y : worldMin.y,
        (corner & 4U) != 0U ? worldMax.z : worldMin.z,
    };
    const CreativeScreenPoint point = projectValidatedPoint(
        clipFromWorld, world, viewportWidth, viewportHeight);
    if (!point.valid) {
      continue;
    }
    minimumX = std::min(minimumX, point.x);
    minimumY = std::min(minimumY, point.y);
    maximumX = std::max(maximumX, point.x);
    maximumY = std::max(maximumY, point.y);
    ++result.projectedCornerCount;
  }

  if (result.projectedCornerCount == 0U) {
    result.status = CreativeScreenProjectionStatus::NoProjectedCorners;
    return result;
  }
  result.minX = minimumX;
  result.minY = minimumY;
  result.maxX = maximumX;
  result.maxY = maximumY;
  result.valid = true;
  result.allCornersProjected = result.projectedCornerCount == 8U;
  result.intersectsViewport =
      maximumX >= 0.0F && maximumY >= 0.0F &&
      minimumX <= static_cast<float>(viewportWidth) &&
      minimumY <= static_cast<float>(viewportHeight);
  result.status = result.allCornersProjected
                      ? CreativeScreenProjectionStatus::Projected
                      : CreativeScreenProjectionStatus::PartiallyProjected;
  return result;
}

}  // namespace iggy3d::creative
