#include "EditorTerrain.hpp"

#include <algorithm>
#include <cstddef>

#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] bool contourCacheMatches(
    const CreativeTerrainContourDisplayState& state,
    const cr::CreativeDocument& document,
    bool sourceOverride,
    std::uint64_t sourceKey) noexcept {
  return state.cacheValid && state.documentId == document.id() &&
         state.terrainRevision == document.terrainField().revision() &&
         state.terrainHeightRevision ==
             document.terrainHeightField().revision() &&
         state.sourceOverride == sourceOverride &&
         state.sourceKey == sourceKey &&
         state.cachedIntervalCells == state.intervalCells &&
         state.cachedMajorEvery == state.majorEvery;
}

[[nodiscard]] bool contourDisplayHidden(const CreativeEditorState& editor,
                                        bool captureMode) noexcept {
  return captureMode || editor.catalog.model.open ||
         editor.catalog.toolWheel.open || editor.toolOptions.open ||
         editor.controls.open || editor.transform.active ||
         editor.transform.controlsOpen || editor.assetReplacement.active ||
         editor.assetEdit.menuOpen;
}

void appendContourLine(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeVec3 start,
    cr::CreativeVec3 end,
    iggy3d::RenderLineColor color,
    float thickness) {
  const cr::CreativeCoreVec3Conversion coreStart =
      cr::creativeVec3ToCoreChecked(start);
  const cr::CreativeCoreVec3Conversion coreEnd =
      cr::creativeVec3ToCoreChecked(end);
  if (!coreStart.converted || !coreEnd.converted) {
    return;
  }
  iggy3d::RenderCreativeWireframeDebugLine line;
  line.start = coreStart.value;
  line.end = coreEnd.value;
  line.color = color;
  line.thickness = thickness;
  lines.push_back(line);
}

}  // namespace

bool refreshCreativeEditorTerrainContours(
    CreativeTerrainContourDisplayState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeTerrainSurfacePlan* surfaceOverride,
    std::uint64_t sourceKey) {
  if (!state.visible) {
    return false;
  }
  const bool hasOverride = surfaceOverride != nullptr;
  if (contourCacheMatches(state, document, hasOverride, sourceKey)) {
    return false;
  }

  cr::CreativeTerrainSurfacePlan composed;
  const cr::CreativeTerrainSurfacePlan* surface = surfaceOverride;
  if (surface == nullptr) {
    composed = cr::buildCreativeComposedTerrainSurfacePlan(
        document.terrainField(), document.terrainHeightField());
    surface = &composed;
  }
  state.plan = cr::buildCreativeTerrainContourPlan(
      *surface,
      {state.intervalCells, state.majorEvery,
       cr::kCreativeTerrainContourSegmentCapacity});
  state.cacheValid = true;
  state.sourceOverride = hasOverride;
  state.documentId = document.id();
  state.terrainRevision = document.terrainField().revision();
  state.terrainHeightRevision = document.terrainHeightField().revision();
  state.sourceKey = sourceKey;
  state.cachedIntervalCells = state.intervalCells;
  state.cachedMajorEvery = state.majorEvery;
  ++state.buildCount;
  return true;
}

std::size_t appendCreativeEditorTerrainContours(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    bool captureMode) {
  const CreativeTerrainContourDisplayState& state = editor.terrain.contours;
  if (!state.visible || !state.cacheValid || !state.plan.accepted ||
      state.plan.status != cr::CreativeTerrainContourPlanStatus::Ready ||
      contourDisplayHidden(editor, captureMode)) {
    return 0U;
  }

  constexpr iggy3d::RenderLineColor minorColor{0.18F, 0.70F, 0.78F, 1.0F};
  constexpr iggy3d::RenderLineColor majorColor{0.98F, 0.82F, 0.18F, 1.0F};
  const cr::CreativeGridSettings grid = document.gridSettings();
  const double lift = std::max(0.01, grid.cellSizeMeters * 0.02);
  const float minorThickness = std::max(0.012F, wireThickness * 0.55F);
  const float majorThickness = std::max(0.02F, wireThickness * 1.05F);
  const std::size_t before = wireLines.size();
  for (const cr::CreativeTerrainContourSegment& segment :
       state.plan.segments) {
    const double y =
        grid.origin.y +
        (static_cast<double>(segment.levelCells) - 0.5) *
            grid.cellSizeMeters +
        lift;
    appendContourLine(
        wireLines,
        {grid.origin.x + segment.start.x * grid.cellSizeMeters, y,
         grid.origin.z + segment.start.z * grid.cellSizeMeters},
        {grid.origin.x + segment.end.x * grid.cellSizeMeters, y,
         grid.origin.z + segment.end.z * grid.cellSizeMeters},
        segment.major ? majorColor : minorColor,
        segment.major ? majorThickness : minorThickness);
  }
  return wireLines.size() - before;
}

}  // namespace iggy3d_creative_app
