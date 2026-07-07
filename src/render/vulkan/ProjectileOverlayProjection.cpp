#include "render/vulkan/ProjectileOverlayProjection.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d::vulkan {
namespace {

struct ScreenPoint {
  std::int32_t x = 0;
  std::int32_t y = 0;
};

bool projectWorldToScreen(const FrameInput& frame, Vec3 world, ScreenPoint& out) {
  if (frame.viewport.width == 0U || frame.viewport.height == 0U) {
    return false;
  }
  const Vec3 ndc = transformPoint(frame.camera.clipFromWorld, world);
  if (!isFinite(ndc) || ndc.z < -0.05F || ndc.z > 1.05F ||
      ndc.x < -1.20F || ndc.x > 1.20F || ndc.y < -1.20F ||
      ndc.y > 1.20F) {
    return false;
  }
  const float screenX =
      (ndc.x * 0.5F + 0.5F) * static_cast<float>(frame.viewport.width);
  const float screenY =
      (1.0F - (ndc.y * 0.5F + 0.5F)) *
      static_cast<float>(frame.viewport.height);
  if (!std::isfinite(screenX) || !std::isfinite(screenY)) {
    return false;
  }
  out.x = static_cast<std::int32_t>(std::lround(std::clamp(
      screenX, 0.0F, static_cast<float>(frame.viewport.width - 1U))));
  out.y = static_cast<std::int32_t>(std::lround(std::clamp(
      screenY, 0.0F, static_cast<float>(frame.viewport.height - 1U))));
  return true;
}

OverlayRect centeredOverlayRect(const FrameInput& frame,
                                ScreenPoint point,
                                std::uint32_t requestedSize,
                                float r,
                                float g,
                                float b) {
  OverlayRect rect;
  if (frame.viewport.width == 0U || frame.viewport.height == 0U ||
      requestedSize == 0U) {
    return rect;
  }
  rect.width = std::min(requestedSize, frame.viewport.width);
  rect.height = std::min(requestedSize, frame.viewport.height);
  const std::int32_t maxX =
      static_cast<std::int32_t>(frame.viewport.width - rect.width);
  const std::int32_t maxY =
      static_cast<std::int32_t>(frame.viewport.height - rect.height);
  rect.x = std::clamp(point.x - static_cast<std::int32_t>(rect.width / 2U), 0,
                      maxX);
  rect.y = std::clamp(point.y - static_cast<std::int32_t>(rect.height / 2U), 0,
                      maxY);
  rect.r = r;
  rect.g = g;
  rect.b = b;
  rect.a = 1.0F;
  return rect;
}

}  // namespace

ProjectileOverlayLayout projectileOverlayLayoutFor(const FrameInput& frame) {
  ProjectileOverlayLayout layout;
  if (frame.projections.scene == nullptr ||
      frame.projections.scene->projectiles.empty()) {
    return layout;
  }
  const std::uint32_t markerSize = frame.viewport.height > 900U ? 18U : 12U;
  const std::uint32_t trailSize = frame.viewport.height > 900U ? 8U : 5U;
  for (const SceneProjectileItem& projectile : frame.projections.scene->projectiles) {
    ++layout.projectileCount;
    const Vec3 current =
        projectile.impact ? projectile.impactPointMeters : projectile.positionMeters;
    ScreenPoint markerPoint;
    if (projectWorldToScreen(frame, current, markerPoint)) {
      layout.projected = true;
      ++layout.markerCount;
      layout.impactVisible = layout.impactVisible || projectile.impact;
      layout.rects.push_back(centeredOverlayRect(
          frame, markerPoint, markerSize, projectile.impact ? 1.0F : 0.30F,
          projectile.impact ? 0.40F : 0.95F, projectile.impact ? 0.12F : 1.0F));
    }

    constexpr std::uint32_t kTrailSamples = 4U;
    for (std::uint32_t sample = 1U; sample <= kTrailSamples; ++sample) {
      const float t = static_cast<float>(sample) /
                      static_cast<float>(kTrailSamples + 1U);
      const Vec3 trailPoint =
          projectile.previousPositionMeters +
          (current - projectile.previousPositionMeters) * t;
      ScreenPoint screenTrail;
      if (projectWorldToScreen(frame, trailPoint, screenTrail)) {
        layout.projected = true;
        ++layout.trailRectCount;
        layout.rects.push_back(centeredOverlayRect(frame, screenTrail, trailSize,
                                                   0.55F, 0.72F, 1.0F));
      }
    }
  }
  return layout;
}

}  // namespace iggy3d::vulkan
