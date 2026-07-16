#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

#include "app/iggy3d/creative/document/LogicLink.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d_creative_app {

enum class CreativeLogicLinkOverlayRole : std::uint8_t {
  Toggle,
  Open,
  Close,
  Enable,
  Disable,
  Invalid,
};

enum class CreativeLogicLinkOverlayStatus : std::uint8_t {
  Planned,
  InvalidSourceBounds,
  InvalidTargetBounds,
  MissingTarget,
  InvalidLink,
  DegenerateEndpoints,
};

struct CreativeLogicLinkOverlayBounds {
  iggy3d::Vec3 min;
  iggy3d::Vec3 max;
  bool available = true;
};

struct CreativeLogicLinkOverlaySegment {
  iggy3d::Vec3 start;
  iggy3d::Vec3 end;
};

inline constexpr std::size_t kCreativeLogicLinkOverlaySegmentCapacity = 3U;

struct CreativeLogicLinkOverlayRequest {
  CreativeLogicLinkOverlayBounds source;
  CreativeLogicLinkOverlayBounds target;
  iggy3d::creative::CreativeLogicLinkAction action =
      iggy3d::creative::CreativeLogicLinkAction::Toggle;
  bool linkValid = false;
};

// One shaft and two arrowhead edges. The plan is fixed-layout and allocation
// free so every viewport path can use the same direction/anchor math.
struct CreativeLogicLinkOverlayPlan {
  std::array<CreativeLogicLinkOverlaySegment,
             kCreativeLogicLinkOverlaySegmentCapacity>
      segments{};
  std::size_t segmentCount = 0U;
  iggy3d::Vec3 sourceAnchor;
  iggy3d::Vec3 targetAnchor;
  iggy3d::Vec3 labelWorldPosition;
  CreativeLogicLinkOverlayRole role = CreativeLogicLinkOverlayRole::Invalid;
  CreativeLogicLinkOverlayStatus status =
      CreativeLogicLinkOverlayStatus::InvalidSourceBounds;
  bool accepted = false;
};

[[nodiscard]] inline std::string_view creativeLogicLinkOverlayLabel(
    CreativeLogicLinkOverlayRole role) noexcept {
  switch (role) {
    case CreativeLogicLinkOverlayRole::Toggle:
      return "TOGGLE";
    case CreativeLogicLinkOverlayRole::Open:
      return "OPEN";
    case CreativeLogicLinkOverlayRole::Close:
      return "CLOSE";
    case CreativeLogicLinkOverlayRole::Enable:
      return "ENABLE";
    case CreativeLogicLinkOverlayRole::Disable:
      return "DISABLE";
    case CreativeLogicLinkOverlayRole::Invalid:
      return "INVALID";
  }
  return "INVALID";
}

namespace logic_link_overlay_detail {

inline constexpr float kAxisEpsilon = 1.0e-5F;

[[nodiscard]] inline bool validBounds(
    const CreativeLogicLinkOverlayBounds& bounds) noexcept {
  return bounds.available && iggy3d::isFinite(bounds.min) &&
         iggy3d::isFinite(bounds.max) && bounds.min.x <= bounds.max.x &&
         bounds.min.y <= bounds.max.y && bounds.min.z <= bounds.max.z;
}

[[nodiscard]] inline iggy3d::Vec3 boundsCenter(
    const CreativeLogicLinkOverlayBounds& bounds) noexcept {
  return (bounds.min + bounds.max) * 0.5F;
}

[[nodiscard]] inline iggy3d::Vec3 boundsHalfExtent(
    const CreativeLogicLinkOverlayBounds& bounds) noexcept {
  return (bounds.max - bounds.min) * 0.5F;
}

[[nodiscard]] inline iggy3d::Vec3 boundsSurfaceAnchor(
    const CreativeLogicLinkOverlayBounds& bounds,
    iggy3d::Vec3 direction) noexcept {
  const iggy3d::Vec3 center = boundsCenter(bounds);
  const iggy3d::Vec3 half = boundsHalfExtent(bounds);
  float distance = std::numeric_limits<float>::max();
  const auto admitAxis = [&distance](float halfExtent, float component) {
    const float magnitude = std::fabs(component);
    if (magnitude > kAxisEpsilon) {
      distance = std::min(distance, halfExtent / magnitude);
    }
  };
  admitAxis(half.x, direction.x);
  admitAxis(half.y, direction.y);
  admitAxis(half.z, direction.z);
  return std::isfinite(distance) &&
                 distance != std::numeric_limits<float>::max()
             ? center + direction * distance
             : center;
}

[[nodiscard]] inline CreativeLogicLinkOverlayRole roleForAction(
    iggy3d::creative::CreativeLogicLinkAction action,
    bool linkValid) noexcept {
  if (!linkValid) {
    return CreativeLogicLinkOverlayRole::Invalid;
  }
  switch (action) {
    case iggy3d::creative::CreativeLogicLinkAction::Toggle:
      return CreativeLogicLinkOverlayRole::Toggle;
    case iggy3d::creative::CreativeLogicLinkAction::Open:
      return CreativeLogicLinkOverlayRole::Open;
    case iggy3d::creative::CreativeLogicLinkAction::Close:
      return CreativeLogicLinkOverlayRole::Close;
    case iggy3d::creative::CreativeLogicLinkAction::Enable:
      return CreativeLogicLinkOverlayRole::Enable;
    case iggy3d::creative::CreativeLogicLinkAction::Disable:
      return CreativeLogicLinkOverlayRole::Disable;
    case iggy3d::creative::CreativeLogicLinkAction::Count:
      return CreativeLogicLinkOverlayRole::Invalid;
  }
  return CreativeLogicLinkOverlayRole::Invalid;
}

inline void finishArrowPlan(CreativeLogicLinkOverlayPlan& plan,
                            iggy3d::Vec3 shaftStart,
                            iggy3d::Vec3 shaftEnd) noexcept {
  iggy3d::Vec3 direction;
  if (!iggy3d::tryNormalize(shaftEnd - shaftStart, direction)) {
    return;
  }
  const float shaftLength = iggy3d::length(shaftEnd - shaftStart);
  const iggy3d::Vec3 reference =
      std::fabs(iggy3d::dot(direction, iggy3d::Vec3{0.0F, 1.0F, 0.0F})) <
              0.92F
          ? iggy3d::Vec3{0.0F, 1.0F, 0.0F}
          : iggy3d::Vec3{1.0F, 0.0F, 0.0F};
  const iggy3d::Vec3 side = iggy3d::normalizedOr(
      iggy3d::cross(direction, reference), iggy3d::Vec3{0.0F, 0.0F, 1.0F});
  const iggy3d::Vec3 markerUp = iggy3d::normalizedOr(
      iggy3d::cross(side, direction), iggy3d::Vec3{0.0F, 1.0F, 0.0F});
  const float arrowLength =
      std::clamp(shaftLength * 0.22F, 0.08F, 0.40F);
  const float arrowHalfWidth =
      std::clamp(arrowLength * 0.55F, 0.045F, 0.22F);
  const iggy3d::Vec3 arrowBase = shaftEnd - direction * arrowLength;
  plan.segments[0] = {shaftStart, shaftEnd};
  plan.segments[1] = {
      shaftEnd, arrowBase + side * arrowHalfWidth};
  plan.segments[2] = {
      shaftEnd, arrowBase - side * arrowHalfWidth};
  plan.segmentCount = kCreativeLogicLinkOverlaySegmentCapacity;
  plan.sourceAnchor = shaftStart;
  plan.targetAnchor = shaftEnd;
  plan.labelWorldPosition =
      (shaftStart + shaftEnd) * 0.5F + markerUp * 0.16F;
  plan.accepted = true;
}

}  // namespace logic_link_overlay_detail

[[nodiscard]] inline CreativeLogicLinkOverlayPlan
planCreativeLogicLinkOverlay(
    const CreativeLogicLinkOverlayRequest& request) noexcept {
  using namespace logic_link_overlay_detail;
  CreativeLogicLinkOverlayPlan plan;
  if (!validBounds(request.source)) {
    plan.status = CreativeLogicLinkOverlayStatus::InvalidSourceBounds;
    return plan;
  }

  const iggy3d::Vec3 sourceCenter = boundsCenter(request.source);
  iggy3d::Vec3 targetCenter;
  if (!request.target.available) {
    const float sourceHeight = request.source.max.y - request.source.min.y;
    targetCenter = sourceCenter +
                   iggy3d::Vec3{0.0F, std::max(0.8F, sourceHeight + 0.4F),
                                0.0F};
    plan.role = CreativeLogicLinkOverlayRole::Invalid;
    plan.status = CreativeLogicLinkOverlayStatus::MissingTarget;
    const iggy3d::Vec3 direction{0.0F, 1.0F, 0.0F};
    finishArrowPlan(plan, boundsSurfaceAnchor(request.source, direction),
                    targetCenter);
    return plan;
  }
  if (!validBounds(request.target)) {
    plan.status = CreativeLogicLinkOverlayStatus::InvalidTargetBounds;
    return plan;
  }

  targetCenter = boundsCenter(request.target);
  iggy3d::Vec3 direction;
  if (!iggy3d::tryNormalize(targetCenter - sourceCenter, direction)) {
    plan.role = CreativeLogicLinkOverlayRole::Invalid;
    plan.status = CreativeLogicLinkOverlayStatus::DegenerateEndpoints;
    const iggy3d::Vec3 fallbackDirection{0.0F, 1.0F, 0.0F};
    const iggy3d::Vec3 fallbackStart =
        boundsSurfaceAnchor(request.source, fallbackDirection);
    finishArrowPlan(plan, fallbackStart,
                    fallbackStart + iggy3d::Vec3{0.0F, 0.8F, 0.0F});
    return plan;
  }

  iggy3d::Vec3 shaftStart = boundsSurfaceAnchor(request.source, direction);
  iggy3d::Vec3 shaftEnd = boundsSurfaceAnchor(request.target, direction * -1.0F);
  if (iggy3d::dot(shaftEnd - shaftStart, direction) <= kAxisEpsilon) {
    shaftStart = sourceCenter;
    shaftEnd = targetCenter;
  }
  plan.role = roleForAction(request.action, request.linkValid);
  plan.status = request.linkValid &&
                        plan.role != CreativeLogicLinkOverlayRole::Invalid
                    ? CreativeLogicLinkOverlayStatus::Planned
                    : CreativeLogicLinkOverlayStatus::InvalidLink;
  finishArrowPlan(plan, shaftStart, shaftEnd);
  return plan;
}

}  // namespace iggy3d_creative_app
