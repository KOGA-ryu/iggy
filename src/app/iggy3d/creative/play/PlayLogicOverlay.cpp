#include "app/iggy3d/creative/play/PlayLogicOverlay.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

#include "app/iggy3d/creative/overlay/LogicLinkOverlay.hpp"
#include "core/math/OrientedBox.hpp"

namespace iggy3d_creative_app {
namespace {

constexpr auto kBoxEdgeIndices = std::to_array<std::array<std::size_t, 2U>>({
    {0U, 1U}, {2U, 3U}, {4U, 5U}, {6U, 7U},
    {0U, 2U}, {1U, 3U}, {4U, 6U}, {5U, 7U},
    {0U, 4U}, {1U, 5U}, {2U, 6U}, {3U, 7U},
});

[[nodiscard]] bool automaticLogicSourceMode(
    iggy3d::creative::CreativeRuntimeLogicSourceMode mode) noexcept {
  using Mode = iggy3d::creative::CreativeRuntimeLogicSourceMode;
  switch (mode) {
    case Mode::PulseOnEnter:
    case Mode::HoldWhileOccupied:
      return true;
    case Mode::None:
    case Mode::Manual:
    case Mode::Count:
      return false;
  }
  return false;
}

[[nodiscard]] std::array<iggy3d::Vec3, 8U> runtimeLogicCorners(
    const iggy3d::creative::CreativeRuntimeInteractableDefinition& definition) {
  return iggy3d::orientedBoxCorners(
      iggy3d::makeOrientedBox(definition.transform, definition.localBounds));
}

[[nodiscard]] CreativeLogicLinkOverlayBounds runtimeLogicBounds(
    const iggy3d::creative::CreativeRuntimeInteractableDefinition& definition) {
  const std::array<iggy3d::Vec3, 8U> corners =
      runtimeLogicCorners(definition);
  for (const iggy3d::Vec3 corner : corners) {
    if (!iggy3d::isFinite(corner)) {
      return {{}, {}, false};
    }
  }
  CreativeLogicLinkOverlayBounds bounds{corners[0], corners[0], true};
  for (std::size_t index = 1U; index < corners.size(); ++index) {
    bounds.min.x = std::min(bounds.min.x, corners[index].x);
    bounds.min.y = std::min(bounds.min.y, corners[index].y);
    bounds.min.z = std::min(bounds.min.z, corners[index].z);
    bounds.max.x = std::max(bounds.max.x, corners[index].x);
    bounds.max.y = std::max(bounds.max.y, corners[index].y);
    bounds.max.z = std::max(bounds.max.z, corners[index].z);
  }
  return bounds;
}

[[nodiscard]] iggy3d::RenderLineColor renderLogicLinkColor(
    CreativeLogicLinkOverlayRole role) noexcept {
  const CreativeLogicLinkOverlayColor color =
      creativeLogicLinkOverlayColor(role);
  return {color.r, color.g, color.b, color.a};
}

std::size_t appendRuntimeLogicBox(
    const iggy3d::creative::CreativeRuntimeInteractableDefinition& definition,
    iggy3d::RenderLineColor color,
    std::uint32_t style,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const std::array<iggy3d::Vec3, 8U> corners =
      runtimeLogicCorners(definition);
  for (const iggy3d::Vec3 corner : corners) {
    if (!iggy3d::isFinite(corner)) {
      return 0U;
    }
  }
  const std::size_t before = lines.size();
  for (const auto& edge : kBoxEdgeIndices) {
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = corners[edge[0]];
    line.end = corners[edge[1]];
    line.color = color;
    line.objectId = definition.objectId;
    line.style = style;
    line.segmentKind = 2U;
    line.thickness = thickness;
    lines.push_back(line);
  }
  return lines.size() - before;
}

}  // namespace

CreativePlayLogicOverlay buildCreativePlayLogicOverlay(
    const iggy3d::creative::CreativeRuntimeSandbox& sandbox,
    iggy3d::creative::CreativeObjectId highlightedLogicSourceObjectId) {
  using InteractableKind =
      iggy3d::creative::CreativeRuntimeInteractableKind;
  using SourceMode = iggy3d::creative::CreativeRuntimeLogicSourceMode;

  CreativePlayLogicOverlay overlay;
  overlay.lines.reserve(sandbox.interactables.size() * kBoxEdgeIndices.size() +
                        sandbox.logicLinks.size() *
                            (kCreativeLogicLinkOverlaySegmentCapacity +
                             kBoxEdgeIndices.size()));
  for (const iggy3d::creative::CreativeRuntimeInteractableState& source :
       sandbox.interactables) {
    if (source.definition.kind != InteractableKind::Control ||
        !automaticLogicSourceMode(source.definition.logicSourceMode)) {
      continue;
    }
    const bool active = source.occupantCount > 0U;
    const iggy3d::RenderLineColor color = active
        ? iggy3d::RenderLineColor{0.15F, 1.0F, 0.25F, 1.0F}
        : iggy3d::RenderLineColor{0.45F, 0.55F, 0.60F, 1.0F};
    overlay.sourceEdgeCount += appendRuntimeLogicBox(
        source.definition, color, active ? 1U : 0U,
        active ? 0.06F : 0.035F, overlay.lines);
  }

  const iggy3d::creative::CreativeRuntimeInteractableState*
      highlightedSource =
          iggy3d::creative::findCreativeRuntimeInteractableByObjectId(
              sandbox, highlightedLogicSourceObjectId);
  const bool highlightedLogicSource =
      highlightedSource != nullptr &&
      highlightedSource->definition.kind == InteractableKind::Control &&
      highlightedSource->definition.logicSourceMode != SourceMode::None &&
      highlightedSource->definition.logicSourceMode != SourceMode::Count;
  if (!highlightedLogicSource) {
    return overlay;
  }

  const bool automatic =
      automaticLogicSourceMode(highlightedSource->definition.logicSourceMode);
  const bool active = automatic && highlightedSource->occupantCount > 0U;
  if (!automatic) {
    overlay.sourceEdgeCount += appendRuntimeLogicBox(
        highlightedSource->definition,
        iggy3d::RenderLineColor{0.20F, 0.82F, 1.0F, 1.0F}, 0U, 0.05F,
        overlay.lines);
  }
  const CreativeLogicLinkOverlayBounds sourceBounds =
      runtimeLogicBounds(highlightedSource->definition);
  if (!sourceBounds.available) {
    return overlay;
  }

  for (const iggy3d::creative::CreativeRuntimeLogicLink& link :
       sandbox.logicLinks) {
    if (link.sourceObjectId != highlightedLogicSourceObjectId) {
      continue;
    }
    const iggy3d::creative::CreativeRuntimeInteractableState* target =
        iggy3d::creative::findCreativeRuntimeInteractableByObjectId(
            sandbox, link.targetObjectId);
    CreativeLogicLinkOverlayBounds targetBounds;
    if (target != nullptr) {
      targetBounds = runtimeLogicBounds(target->definition);
    } else {
      targetBounds.available = false;
    }
    const bool validTarget =
        target != nullptr && targetBounds.available &&
        iggy3d::creative::creativeRuntimeInteractableIsLogicTarget(
            target->definition.kind) &&
        iggy3d::creative::creativeRuntimeLogicActionSupported(
            target->definition.kind, link.action);

    CreativeLogicLinkOverlayRequest planRequest;
    planRequest.source = sourceBounds;
    planRequest.target = targetBounds;
    planRequest.action = link.action;
    planRequest.linkValid = validTarget;
    const CreativeLogicLinkOverlayPlan plan =
        planCreativeLogicLinkOverlay(planRequest);
    if (!plan.accepted) {
      continue;
    }
    const iggy3d::RenderLineColor color =
        plan.role == CreativeLogicLinkOverlayRole::Invalid
            ? renderLogicLinkColor(plan.role)
            : active
                  ? iggy3d::RenderLineColor{0.15F, 1.0F, 0.25F, 1.0F}
                  : renderLogicLinkColor(plan.role);
    for (std::size_t index = 0U; index < plan.segmentCount; ++index) {
      iggy3d::RenderCreativeWireframeDebugLine line;
      line.start = plan.segments[index].start;
      line.end = plan.segments[index].end;
      line.color = color;
      line.objectId = highlightedLogicSourceObjectId;
      line.style = static_cast<std::uint32_t>(plan.role);
      line.segmentKind = index == 0U ? 0U : 1U;
      line.thickness = index == 0U ? (active ? 0.055F : 0.04F)
                                   : (active ? 0.045F : 0.032F);
      overlay.lines.push_back(line);
    }
    ++overlay.linkShaftCount;
    overlay.linkArrowEdgeCount += plan.segmentCount - 1U;
    if (plan.role == CreativeLogicLinkOverlayRole::Invalid) {
      ++overlay.invalidLinkCount;
    }
    if (validTarget) {
      const iggy3d::RenderLineColor targetColor =
          target->targetActive
              ? iggy3d::RenderLineColor{0.15F, 1.0F, 0.25F, 1.0F}
              : iggy3d::RenderLineColor{1.0F, 0.64F, 0.18F, 1.0F};
      overlay.targetEdgeCount += appendRuntimeLogicBox(
          target->definition, targetColor, target->targetActive ? 1U : 0U,
          0.045F, overlay.lines);
    }
  }
  return overlay;
}

}  // namespace iggy3d_creative_app
