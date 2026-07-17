#include "EditorPlacementGridOverlay.hpp"

#include <algorithm>

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
}

}  // namespace iggy3d_creative_app
