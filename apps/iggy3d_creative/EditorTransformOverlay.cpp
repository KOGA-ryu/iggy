#include "EditorTransform.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

#include "EditorPreviewProxies.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace {

constexpr std::size_t kDetailedTransformPreviewObjectCapacity = 512U;
constexpr float kTau = 6.28318530717958647692F;

[[nodiscard]] bool renderPoint(cr::CreativeVec3 value,
                               iggy3d::Vec3& output) noexcept {
  const cr::CreativeCoreVec3Conversion conversion =
      cr::creativeVec3ToCoreChecked(value);
  if (!conversion.converted) {
    return false;
  }
  output = conversion.value;
  return true;
}

[[nodiscard]] bool finiteBounds(VisualBounds bounds) noexcept {
  return std::isfinite(bounds.min.x) && std::isfinite(bounds.min.y) &&
         std::isfinite(bounds.min.z) && std::isfinite(bounds.max.x) &&
         std::isfinite(bounds.max.y) && std::isfinite(bounds.max.z);
}

void includeBounds(VisualBounds& aggregate,
                   bool& initialized,
                   VisualBounds bounds) noexcept {
  if (!initialized) {
    aggregate = bounds;
    initialized = true;
    return;
  }
  aggregate.min.x = std::min(aggregate.min.x, bounds.min.x);
  aggregate.min.y = std::min(aggregate.min.y, bounds.min.y);
  aggregate.min.z = std::min(aggregate.min.z, bounds.min.z);
  aggregate.max.x = std::max(aggregate.max.x, bounds.max.x);
  aggregate.max.y = std::max(aggregate.max.y, bounds.max.y);
  aggregate.max.z = std::max(aggregate.max.z, bounds.max.z);
}

std::size_t appendObjectBounds(
    std::span<const cr::CreativeObject> objects,
    iggy3d::RenderLineColor color,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const std::size_t before = wireLines.size();
  if (objects.size() <= kDetailedTransformPreviewObjectCapacity) {
    wireLines.reserve(wireLines.size() + objects.size() * 12U);
    for (const cr::CreativeObject& object : objects) {
      const VisualBounds bounds = visualBoundsForObject(object);
      if (finiteBounds(bounds)) {
        appendStandaloneWireframeBoxEdges(wireLines, bounds.min, bounds.max,
                                          color, thickness);
      }
    }
    return wireLines.size() - before;
  }

  VisualBounds aggregate{};
  bool initialized = false;
  for (const cr::CreativeObject& object : objects) {
    const VisualBounds bounds = visualBoundsForObject(object);
    if (finiteBounds(bounds)) {
      includeBounds(aggregate, initialized, bounds);
    }
  }
  if (initialized) {
    appendStandaloneWireframeBoxEdges(wireLines, aggregate.min, aggregate.max,
                                      color, thickness);
  }
  return wireLines.size() - before;
}

void appendAnchorMarker(
    cr::CreativeVec3 anchor,
    iggy3d::RenderLineColor color,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  iggy3d::Vec3 point;
  if (!renderPoint(anchor, point)) {
    return;
  }
  constexpr float kHalfSize = 0.09F;
  const iggy3d::Vec3 half{kHalfSize, kHalfSize, kHalfSize};
  appendStandaloneWireframeBoxEdges(wireLines, point - half, point + half, color,
                                    thickness);
}

void appendAxisSegment(
    iggy3d::Vec3 start,
    iggy3d::Vec3 end,
    iggy3d::RenderLineColor color,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  if (start.x == end.x && start.y == end.y && start.z == end.z) {
    return;
  }
  wireLines.push_back({start, end, color, 0, 0, 0, 0, thickness});
}

void appendAnchorConnector(
    cr::CreativeVec3 source,
    cr::CreativeVec3 target,
    iggy3d::RenderLineColor color,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  iggy3d::Vec3 start;
  iggy3d::Vec3 end;
  if (!renderPoint(source, start) || !renderPoint(target, end)) {
    return;
  }
  const iggy3d::Vec3 xCorner{end.x, start.y, start.z};
  const iggy3d::Vec3 yCorner{end.x, end.y, start.z};
  appendAxisSegment(start, xCorner, color, thickness, wireLines);
  appendAxisSegment(xCorner, yCorner, color, thickness, wireLines);
  appendAxisSegment(yCorner, end, color, thickness, wireLines);
}

[[nodiscard]] iggy3d::RenderLineColor constraintColor(
    cr::CreativeSelectionPlacementAxis axis) noexcept {
  switch (axis) {
    case cr::CreativeSelectionPlacementAxis::X:
      return {1.0F, 0.24F, 0.20F, 0.98F};
    case cr::CreativeSelectionPlacementAxis::Y:
      return {0.22F, 1.0F, 0.38F, 0.98F};
    case cr::CreativeSelectionPlacementAxis::Z:
      return {0.24F, 0.56F, 1.0F, 0.98F};
    case cr::CreativeSelectionPlacementAxis::Free:
    case cr::CreativeSelectionPlacementAxis::Count:
      return {0.82F, 0.88F, 0.92F, 0.72F};
  }
  return {0.82F, 0.88F, 0.92F, 0.72F};
}

void appendConstraintGuide(
    cr::CreativeVec3 source,
    cr::CreativeVec3 target,
    cr::CreativeSelectionPlacementAxis axis,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  if (axis == cr::CreativeSelectionPlacementAxis::Free ||
      axis == cr::CreativeSelectionPlacementAxis::Count) {
    return;
  }
  iggy3d::Vec3 start;
  iggy3d::Vec3 end;
  if (!renderPoint(source, start) || !renderPoint(target, end)) {
    return;
  }
  if (start.x == end.x && start.y == end.y && start.z == end.z) {
    constexpr float kHalfGuideLength = 0.65F;
    switch (axis) {
      case cr::CreativeSelectionPlacementAxis::X:
        start.x -= kHalfGuideLength;
        end.x += kHalfGuideLength;
        break;
      case cr::CreativeSelectionPlacementAxis::Y:
        start.y -= kHalfGuideLength;
        end.y += kHalfGuideLength;
        break;
      case cr::CreativeSelectionPlacementAxis::Z:
        start.z -= kHalfGuideLength;
        end.z += kHalfGuideLength;
        break;
      case cr::CreativeSelectionPlacementAxis::Free:
      case cr::CreativeSelectionPlacementAxis::Count:
        return;
    }
  }
  appendAxisSegment(start, end, constraintColor(axis), thickness, wireLines);
}

void appendText(std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
                std::string_view text,
                std::int32_t x,
                std::int32_t y,
                std::uint32_t width,
                std::uint32_t height,
                float r,
                float g,
                float b) {
  iggy3d::DebugHudLayoutResult layout =
      iggy3d::layoutDebugHudTextAt(text, x, y, width, height);
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quad.r = r;
    quad.g = g;
    quad.b = b;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

[[nodiscard]] std::string controlLabel(
    const CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformControl control) {
  switch (control) {
    case CreativeEditorTransformControl::RotatePositive: return "ROTATE +90";
    case CreativeEditorTransformControl::MirrorX: return "MIRROR X";
    case CreativeEditorTransformControl::CycleConstraint:
      return std::string{"AXIS "} + std::string{cr::toString(state.constraint)};
    case CreativeEditorTransformControl::ToggleMode:
      return state.mode == cr::CreativeSelectionPlacementMode::Move
                 ? "MODE COPY"
                 : state.moveAvailable ? "MODE MOVE" : "MOVE LOCKED";
    case CreativeEditorTransformControl::Confirm: return "CONFIRM";
    case CreativeEditorTransformControl::Cancel: return "CANCEL";
    case CreativeEditorTransformControl::MirrorZ: return "MIRROR Z";
    case CreativeEditorTransformControl::RotateNegative: return "ROTATE -90";
    case CreativeEditorTransformControl::Reset: return "RESET";
    case CreativeEditorTransformControl::Count: return "UNKNOWN";
  }
  return "UNKNOWN";
}

}  // namespace

std::size_t appendCreativeEditorSelectionTransformPreview(
    const CreativeEditorSelectionTransformState& state,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  if (!state.active || cr::creativeClipboardEmpty(state.sourceClipboard)) {
    return 0U;
  }
  const std::size_t before = wireLines.size();
  const float thickness = std::isfinite(wireThickness)
                              ? std::max(0.025F, wireThickness * 0.9F)
                              : 0.025F;
  const iggy3d::RenderLineColor amber{1.0F, 0.63F, 0.16F, 0.95F};
  const iggy3d::RenderLineColor mint{0.18F, 1.0F, 0.70F, 0.95F};
  const iggy3d::RenderLineColor red{1.0F, 0.24F, 0.20F, 0.96F};
  const iggy3d::RenderLineColor connector{0.82F, 0.88F, 0.92F, 0.72F};

  if (state.mode == cr::CreativeSelectionPlacementMode::Move) {
    static_cast<void>(appendObjectBounds(state.sourceClipboard.objects, amber,
                                         thickness, wireLines));
    appendAnchorMarker(state.sourceClipboard.placementAnchor, amber, thickness,
                       wireLines);
  }
  if (!state.targetPositionable || state.plan.objects.empty()) {
    return wireLines.size() - before;
  }

  const bool failedCommit =
      state.lastCommit.requested && !state.lastCommit.accepted;
  const iggy3d::RenderLineColor destination =
      state.plan.accepted && !failedCommit ? mint : red;
  static_cast<void>(appendObjectBounds(state.plan.objects, destination,
                                       thickness, wireLines));
  appendAnchorMarker(state.request.targetAnchor, destination, thickness,
                     wireLines);
  if (state.mode == cr::CreativeSelectionPlacementMode::Move) {
    appendAnchorConnector(state.request.sourceAnchor, state.request.targetAnchor,
                          connector, std::max(0.025F, thickness * 0.65F),
                          wireLines);
  }
  appendConstraintGuide(state.request.sourceAnchor, state.request.targetAnchor,
                        state.constraint, std::max(0.035F, thickness * 1.1F),
                        wireLines);
  return wireLines.size() - before;
}

void appendCreativeEditorTransformOverlay(
    const CreativeEditorSelectionTransformState& state,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  if (!state.active || drawableWidth == 0U || drawableHeight == 0U) {
    return;
  }
  const std::int32_t width = static_cast<std::int32_t>(drawableWidth);
  const std::int32_t height = static_cast<std::int32_t>(drawableHeight);
  const std::uint32_t statusWidth =
      std::min(860U, drawableWidth > 16U ? drawableWidth - 16U : drawableWidth);
  const std::int32_t statusX =
      std::max(0, (width - static_cast<std::int32_t>(statusWidth)) / 2);
  const std::int32_t statusY = std::max(4, height - 112);
  uiRects.push_back(
      {statusX, statusY, statusWidth, 34U, 0.045F, 0.052F, 0.058F, 0.94F});
  const cr::CreativeVec3 displacement{
      state.request.targetAnchor.x - state.request.sourceAnchor.x,
      state.request.targetAnchor.y - state.request.sourceAnchor.y,
      state.request.targetAnchor.z - state.request.sourceAnchor.z};
  const double visibleStep =
      state.snapStepMeters * (state.fineNudgeActive ? 0.25 : 1.0);
  char status[256];
  std::snprintf(
      status, sizeof(status),
      "%s | %s | %s %.2fM | D %+.2f %+.2f %+.2f | %uDEG%s%s | R",
      std::string(cr::toString(state.mode)).c_str(),
      std::string(cr::toString(state.constraint)).c_str(),
      state.fineNudgeActive ? "FINE" : "STEP", visibleStep, displacement.x,
      displacement.y, displacement.z,
      static_cast<unsigned>(state.request.quarterTurns) * 90U,
      state.request.mirrorX ? " | MX" : "",
      state.request.mirrorZ ? " | MZ" : "");
  const bool ready = state.targetPositionable && state.plan.accepted;
  appendText(glyphs, status, statusX + 12, statusY + 10, drawableWidth,
             drawableHeight, ready ? 0.72F : 1.0F,
             ready ? 0.94F : 0.42F, ready ? 0.82F : 0.36F);

  if (!state.controlsOpen) {
    return;
  }
  uiRects.push_back(
      {0, 0, drawableWidth, drawableHeight, 0.01F, 0.015F, 0.02F, 0.64F});
  const std::int32_t centerX = width / 2;
  const std::int32_t centerY = height / 2;
  const std::uint32_t availableWidth =
      drawableWidth > 16U ? drawableWidth - 16U : drawableWidth;
  const std::uint32_t tileWidth = std::min(136U, availableWidth);
  const std::uint32_t tileHeight = std::min(38U, drawableHeight);
  const std::int32_t radiusX =
      std::min(286, std::max(0, (width - static_cast<std::int32_t>(tileWidth) -
                                32) /
                                   2));
  const std::int32_t radiusY =
      std::min(192, std::max(0, (height - static_cast<std::int32_t>(tileHeight) -
                                100) /
                                   2));
  for (std::size_t index = 0; index < kCreativeEditorTransformControlCount;
       ++index) {
    const float angle = kTau * static_cast<float>(index) /
                        static_cast<float>(kCreativeEditorTransformControlCount);
    const std::int32_t x = std::clamp(
        centerX + static_cast<std::int32_t>(
                      std::round(std::sin(angle) * static_cast<float>(radiusX))) -
            static_cast<std::int32_t>(tileWidth / 2U),
        0, std::max(0, width - static_cast<std::int32_t>(tileWidth)));
    const std::int32_t y = std::clamp(
        centerY - static_cast<std::int32_t>(
                      std::round(std::cos(angle) * static_cast<float>(radiusY))) -
            static_cast<std::int32_t>(tileHeight / 2U),
        0, std::max(0, height - static_cast<std::int32_t>(tileHeight)));
    const bool selected = index == state.selectedControl;
    uiRects.push_back({x, y, tileWidth, tileHeight,
                       selected ? 0.86F : 0.07F,
                       selected ? 0.76F : 0.08F,
                       selected ? 0.28F : 0.09F,
                       selected ? 0.98F : 0.94F});
    const std::string label = controlLabel(
        state, static_cast<CreativeEditorTransformControl>(index));
    appendText(glyphs, label, x + 8, y + 11, drawableWidth, drawableHeight,
               selected ? 0.06F : 0.88F, selected ? 0.065F : 0.91F,
               selected ? 0.07F : 0.94F);
  }

  const std::uint32_t centerWidth = std::min(210U, availableWidth);
  const std::int32_t panelX =
      std::max(0, centerX - static_cast<std::int32_t>(centerWidth / 2U));
  const std::int32_t panelY = std::max(0, centerY - 32);
  uiRects.push_back(
      {panelX, panelY, centerWidth, 64U, 0.045F, 0.052F, 0.058F, 0.98F});
  appendText(glyphs, "TRANSFORM", panelX + 12, panelY + 13, drawableWidth,
             drawableHeight, 0.92F, 0.94F, 0.96F);
  appendText(glyphs,
             ready ? "CROSS APPLY | CIRCLE BACK" : "TARGET NOT READY",
             panelX + 12, panelY + 38, drawableWidth, drawableHeight,
             ready ? 0.65F : 1.0F, ready ? 0.72F : 0.38F,
             ready ? 0.76F : 0.30F);
}

}  // namespace iggy3d_creative_app
