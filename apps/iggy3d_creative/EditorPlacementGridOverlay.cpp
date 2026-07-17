#include "EditorPlacementGridOverlay.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] iggy3d::RenderLineColor placementGridColor(
    cr::CreativePlacementGridLineRole role) noexcept {
  switch (role) {
    case cr::CreativePlacementGridLineRole::Minor:
      return {0.34F, 0.46F, 0.54F, 0.42F};
    case cr::CreativePlacementGridLineRole::Major:
      return {0.34F, 0.72F, 0.82F, 0.72F};
    case cr::CreativePlacementGridLineRole::Boundary:
      return {1.0F, 0.58F, 0.16F, 0.92F};
  }
  return {0.34F, 0.46F, 0.54F, 0.42F};
}

[[nodiscard]] float placementGridThickness(
    cr::CreativePlacementGridLineRole role,
    double minorStepMeters) noexcept {
  const float base = std::clamp(static_cast<float>(minorStepMeters) * 0.025F,
                                0.008F, 0.025F);
  switch (role) {
    case cr::CreativePlacementGridLineRole::Minor:
      return base;
    case cr::CreativePlacementGridLineRole::Major:
      return std::max(0.018F, base * 1.75F);
    case cr::CreativePlacementGridLineRole::Boundary:
      return std::max(0.026F, base * 2.25F);
  }
  return base;
}

[[nodiscard]] iggy3d::RenderLineColor placementGridDotColor(
    cr::CreativePlacementGridLineRole role) noexcept {
  switch (role) {
    case cr::CreativePlacementGridLineRole::Minor:
      return {0.56F, 0.78F, 0.86F, 0.82F};
    case cr::CreativePlacementGridLineRole::Major:
      return {0.30F, 0.90F, 1.0F, 1.0F};
    case cr::CreativePlacementGridLineRole::Boundary:
      return {1.0F, 0.66F, 0.20F, 1.0F};
  }
  return {0.56F, 0.78F, 0.86F, 0.82F};
}

[[nodiscard]] float placementGridDotThickness(
    cr::CreativePlacementGridLineRole role,
    double minorStepMeters) noexcept {
  const float base = std::clamp(static_cast<float>(minorStepMeters) * 0.065F,
                                0.025F, 0.065F);
  return role == cr::CreativePlacementGridLineRole::Boundary
             ? base * 1.35F
         : role == cr::CreativePlacementGridLineRole::Major ? base * 1.18F
                                                            : base;
}

enum class PlacementTargetMarker : std::uint8_t {
  None,
  Valid,
  Invalid,
};

[[nodiscard]] PlacementTargetMarker placementTargetMarker(
    const CreativeEditorState& editor,
    const iggy3d::FrameInput& frame,
    const cr::CreativeGridTarget& target) noexcept {
  const CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  if (feedback.frameIndex == editor.frameIndex &&
      feedback.status == CreativeEditorPlacementFeedbackStatus::Placed) {
    return PlacementTargetMarker::None;
  }
  const std::size_t previewCount =
      std::min(static_cast<std::size_t>(frame.creativePreview.itemCount),
               frame.creativePreview.items.size());
  for (std::size_t index = 0U; index < previewCount; ++index) {
    switch (frame.creativePreview.items[index].role) {
      case iggy3d::RenderCreativePreviewRole::PlacementValid:
        return PlacementTargetMarker::Valid;
      case iggy3d::RenderCreativePreviewRole::PlacementInvalid:
        return PlacementTargetMarker::Invalid;
      default:
        break;
    }
  }
  if (target.resolved && (!target.targetInBounds || !target.adjacentInBounds)) {
    return PlacementTargetMarker::Invalid;
  }
  return PlacementTargetMarker::None;
}

[[nodiscard]] iggy3d::RenderLineColor placementTargetMarkerColor(
    PlacementTargetMarker marker) noexcept {
  return marker == PlacementTargetMarker::Valid
             ? iggy3d::RenderLineColor{0.18F, 1.0F, 0.28F, 1.0F}
             : iggy3d::RenderLineColor{1.0F, 0.18F, 0.14F, 1.0F};
}

}  // namespace

void appendCreativeEditorPlacementGridOverlay(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  const CreativeEditorState& editor = request.editor;
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.assetReplacement.active ||
                         editor.transform.active;
  if (request.captureMode || modalOpen ||
      request.inputContext != cr::CreativeInputContext::EditorViewport) {
    return;
  }

  const cr::CreativeDocument& document = request.appState.facade.document();
  const cr::CreativeGridTarget& target = editor.interaction.target.grid;
  const bool targetPlaneAvailable =
      target.resolved &&
      cr::isFiniteCreativeVec3(target.adjacentCellBounds.min);
  const double activePlaneY =
      targetPlaneAvailable ? target.adjacentCellBounds.min.y
                           : document.gridSettings().origin.y;
  cr::CreativePlacementGridFrame frame = creativeEditorPlacementGridFrame(
      document, editor, activePlaneY, true);
  if (!frame.valid) {
    return;
  }
  const double planeOffset =
      std::clamp(std::min(frame.stepMeters.x, frame.stepMeters.z) * 0.004,
                 0.002, 0.01);
  frame.activePlaneY += planeOffset;
  const cr::CreativePlacementGridOverlayPlan plan =
      cr::buildCreativePlacementGridOverlayPlan(
          {frame, cr::creativeVec3FromCore(request.frame.camera.worldEye)});
  if (!plan.valid) {
    return;
  }

  const std::size_t before = output.combinedWireLines.size();
  for (std::size_t index = 0U; index < plan.lineCount; ++index) {
    const cr::CreativePlacementGridLine& source = plan.lines[index];
    const cr::CreativeCoreVec3Conversion start =
        cr::creativeVec3ToCoreChecked(source.start);
    const cr::CreativeCoreVec3Conversion end =
        cr::creativeVec3ToCoreChecked(source.end);
    if (!start.converted || !end.converted) {
      continue;
    }
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = start.value;
    line.end = end.value;
    line.color = placementGridColor(source.role);
    line.style = static_cast<std::uint32_t>(source.role);
    line.segmentKind = 4U;
    line.thickness =
        placementGridThickness(source.role,
                               std::min(frame.stepMeters.x,
                                        frame.stepMeters.z));
    output.combinedWireLines.push_back(line);
  }
  output.placementGridLineCount =
      output.combinedWireLines.size() - before;
  output.placementGridClipped = plan.clippedX || plan.clippedZ;

  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::Material) {
    return;
  }
  const double minorStep =
      std::min({frame.stepMeters.x, frame.stepMeters.y, frame.stepMeters.z});

  if (frame.depthOffsetSteps > 0U && target.resolved) {
    const cr::CreativeCoreVec3Conversion guideStart =
        cr::creativeVec3ToCoreChecked(target.surfacePlacementAnchor);
    const cr::CreativeCoreVec3Conversion guideEnd =
        cr::creativeVec3ToCoreChecked(target.basePlacementAnchor);
    if (guideStart.converted && guideEnd.converted) {
      iggy3d::RenderCreativeWireframeDebugLine guide;
      guide.start = guideStart.value;
      guide.end = guideEnd.value;
      guide.color = {1.0F, 0.82F, 0.18F, 0.95F};
      guide.segmentKind = 6U;
      guide.thickness = std::clamp(static_cast<float>(minorStep) * 0.045F,
                                   0.025F, 0.065F);
      output.combinedWireLines.push_back(guide);
      output.placementGridGuideLineCount = 1U;
    }
  }

  if (target.anchorSnapped && target.resolved) {
    const cr::CreativeCoreVec3Conversion anchorStart =
        cr::creativeVec3ToCoreChecked(target.basePlacementAnchor);
    const cr::CreativeCoreVec3Conversion anchorEnd =
        cr::creativeVec3ToCoreChecked(target.placementAnchor);
    const cr::CreativeVec3 delta{
        target.placementAnchor.x - target.basePlacementAnchor.x,
        target.placementAnchor.y - target.basePlacementAnchor.y,
        target.placementAnchor.z - target.basePlacementAnchor.z};
    if (anchorStart.converted && anchorEnd.converted &&
        (delta.x != 0.0 || delta.y != 0.0 || delta.z != 0.0)) {
      iggy3d::RenderCreativeWireframeDebugLine anchorGuide;
      anchorGuide.start = anchorStart.value;
      anchorGuide.end = anchorEnd.value;
      anchorGuide.color = {0.20F, 0.92F, 1.0F, 0.98F};
      anchorGuide.segmentKind = 8U;
      anchorGuide.thickness =
          std::clamp(static_cast<float>(minorStep) * 0.05F, 0.03F, 0.075F);
      output.combinedWireLines.push_back(anchorGuide);
      output.placementGridAnchorGuideLineCount = 1U;
    }
  }

  if (target.anchorSnapped && target.anchorCandidates.valid &&
      target.anchorCandidates.count <=
          target.anchorCandidates.positions.size()) {
    constexpr std::array axes{
        cr::CreativeVec3{1.0, 0.0, 0.0},
        cr::CreativeVec3{0.0, 1.0, 0.0},
        cr::CreativeVec3{0.0, 0.0, 1.0},
    };
    const double candidateHalfLength =
        std::clamp(minorStep * 0.055, 0.025, 0.07);
    const std::size_t candidateStart = output.combinedWireLines.size();
    for (std::uint8_t index = 0U;
         index < target.anchorCandidates.count; ++index) {
      if (index == target.anchorIndex) {
        continue;
      }
      const cr::CreativeVec3 center =
          target.anchorCandidates.positions[index];
      for (const cr::CreativeVec3 axis : axes) {
        const cr::CreativeVec3 start{
            center.x - axis.x * candidateHalfLength,
            center.y - axis.y * candidateHalfLength,
            center.z - axis.z * candidateHalfLength};
        const cr::CreativeVec3 end{
            center.x + axis.x * candidateHalfLength,
            center.y + axis.y * candidateHalfLength,
            center.z + axis.z * candidateHalfLength};
        const cr::CreativeCoreVec3Conversion convertedStart =
            cr::creativeVec3ToCoreChecked(start);
        const cr::CreativeCoreVec3Conversion convertedEnd =
            cr::creativeVec3ToCoreChecked(end);
        if (!convertedStart.converted || !convertedEnd.converted) {
          continue;
        }
        iggy3d::RenderCreativeWireframeDebugLine candidateLine;
        candidateLine.start = convertedStart.value;
        candidateLine.end = convertedEnd.value;
        candidateLine.color = {0.20F, 0.78F, 0.94F, 0.74F};
        candidateLine.segmentKind = 9U;
        candidateLine.thickness = std::clamp(
            static_cast<float>(minorStep) * 0.032F, 0.018F, 0.045F);
        output.combinedWireLines.push_back(candidateLine);
      }
    }
    output.placementGridAnchorCandidateLineCount =
        output.combinedWireLines.size() - candidateStart;
  }

  if (target.anchorSnapped && target.resolved &&
      cr::isFiniteCreativeVec3(target.placementNormal)) {
    const double normalLengthSquared =
        target.placementNormal.x * target.placementNormal.x +
        target.placementNormal.y * target.placementNormal.y +
        target.placementNormal.z * target.placementNormal.z;
    if (std::isfinite(normalLengthSquared) &&
        normalLengthSquared > 1.0e-24) {
      const double guideLength =
          std::clamp(minorStep * 0.28, 0.14, 0.42);
      const double inverseLength = 1.0 / std::sqrt(normalLengthSquared);
      const cr::CreativeVec3 guideEnd{
          target.placementAnchor.x +
              target.placementNormal.x * inverseLength * guideLength,
          target.placementAnchor.y +
              target.placementNormal.y * inverseLength * guideLength,
          target.placementAnchor.z +
              target.placementNormal.z * inverseLength * guideLength};
      const cr::CreativeCoreVec3Conversion convertedStart =
          cr::creativeVec3ToCoreChecked(target.placementAnchor);
      const cr::CreativeCoreVec3Conversion convertedEnd =
          cr::creativeVec3ToCoreChecked(guideEnd);
      if (convertedStart.converted && convertedEnd.converted) {
        iggy3d::RenderCreativeWireframeDebugLine contactGuide;
        contactGuide.start = convertedStart.value;
        contactGuide.end = convertedEnd.value;
        contactGuide.color = {1.0F, 0.78F, 0.16F, 1.0F};
        contactGuide.segmentKind = 10U;
        contactGuide.thickness = std::clamp(
            static_cast<float>(minorStep) * 0.055F, 0.032F, 0.08F);
        output.combinedWireLines.push_back(contactGuide);
        output.placementGridContactGuideLineCount = 1U;
      }
    }
  }

  if (editor.toolSettings.placementGridDots ==
      cr::CreativePlacementGridDots::NearestLayer) {
    const cr::CreativePlacementGridDotLayerPlan dots =
        cr::buildCreativePlacementGridDotLayerPlan({frame, target});
    const double halfLength = std::clamp(minorStep * 0.035, 0.008, 0.035);
    const std::size_t dotStart = output.combinedWireLines.size();
    if (dots.valid) {
      for (std::size_t index = 0U; index < dots.dotCount; ++index) {
        const cr::CreativePlacementGridDot& source = dots.dots[index];
        const cr::CreativeVec3 start{
            source.position.x - target.viewDepthAxis.x * halfLength,
            source.position.y - target.viewDepthAxis.y * halfLength,
            source.position.z - target.viewDepthAxis.z * halfLength};
        const cr::CreativeVec3 end{
            source.position.x + target.viewDepthAxis.x * halfLength,
            source.position.y + target.viewDepthAxis.y * halfLength,
            source.position.z + target.viewDepthAxis.z * halfLength};
        const cr::CreativeCoreVec3Conversion convertedStart =
            cr::creativeVec3ToCoreChecked(start);
        const cr::CreativeCoreVec3Conversion convertedEnd =
            cr::creativeVec3ToCoreChecked(end);
        if (!convertedStart.converted || !convertedEnd.converted) {
          continue;
        }
        iggy3d::RenderCreativeWireframeDebugLine dot;
        dot.start = convertedStart.value;
        dot.end = convertedEnd.value;
        dot.color = placementGridDotColor(source.role);
        dot.style = static_cast<std::uint32_t>(source.role);
        dot.segmentKind = 5U;
        dot.thickness = placementGridDotThickness(source.role, minorStep);
        output.combinedWireLines.push_back(dot);
      }
    }
    output.placementGridDotCount =
        output.combinedWireLines.size() - dotStart;
  }

  const PlacementTargetMarker marker =
      placementTargetMarker(editor, request.frame, target);
  const cr::CreativeVec3 markerAxis =
      target.viewDepthAxis.x != 0.0 || target.viewDepthAxis.y != 0.0 ||
              target.viewDepthAxis.z != 0.0
          ? target.viewDepthAxis
          : target.faceNormal;
  if (marker == PlacementTargetMarker::None || !target.resolved ||
      (markerAxis.x == 0.0 && markerAxis.y == 0.0 && markerAxis.z == 0.0)) {
    return;
  }
  const double markerHalfLength =
      std::clamp(minorStep * 0.11, 0.055, 0.14);
  const cr::CreativeVec3 markerStart{
      target.placementAnchor.x - markerAxis.x * markerHalfLength,
      target.placementAnchor.y - markerAxis.y * markerHalfLength,
      target.placementAnchor.z - markerAxis.z * markerHalfLength};
  const cr::CreativeVec3 markerEnd{
      target.placementAnchor.x + markerAxis.x * markerHalfLength,
      target.placementAnchor.y + markerAxis.y * markerHalfLength,
      target.placementAnchor.z + markerAxis.z * markerHalfLength};
  const cr::CreativeCoreVec3Conversion convertedMarkerStart =
      cr::creativeVec3ToCoreChecked(markerStart);
  const cr::CreativeCoreVec3Conversion convertedMarkerEnd =
      cr::creativeVec3ToCoreChecked(markerEnd);
  if (!convertedMarkerStart.converted || !convertedMarkerEnd.converted) {
    return;
  }
  iggy3d::RenderCreativeWireframeDebugLine targetMarker;
  targetMarker.start = convertedMarkerStart.value;
  targetMarker.end = convertedMarkerEnd.value;
  targetMarker.color = placementTargetMarkerColor(marker);
  targetMarker.segmentKind = 7U;
  targetMarker.thickness =
      std::clamp(static_cast<float>(minorStep) * 0.12F, 0.08F, 0.16F);
  output.combinedWireLines.push_back(targetMarker);
  output.placementGridTargetMarkerCount = 1U;
}

}  // namespace iggy3d_creative_app
