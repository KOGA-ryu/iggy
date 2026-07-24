#include "EditorOverlayWireframesInternal.hpp"

#include "EditorState.hpp"
#include "EditorWorldLayoutBuildings.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

#include <algorithm>
#include <cstddef>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void appendLine(CreativeEditorOverlayFrame& output,
                cr::CreativeVec3 start,
                cr::CreativeVec3 end,
                iggy3d::RenderLineColor color,
                float thickness) {
  output.combinedWireLines.push_back(
      {{static_cast<float>(start.x), static_cast<float>(start.y),
        static_cast<float>(start.z)},
       {static_cast<float>(end.x), static_cast<float>(end.y),
        static_cast<float>(end.z)},
       color,
       cr::kInvalidObjectId,
       0U,
       0U,
       0U,
       thickness});
}

void appendHumanReference(CreativeEditorOverlayFrame& output,
                          double x,
                          double floorY,
                          double z) {
  constexpr iggy3d::RenderLineColor kHumanColor{
      0.20F, 0.88F, 1.0F, 1.0F};
  constexpr float kThickness = 0.045F;
  constexpr double kHip = 0.82;
  constexpr double kShoulder = 1.34;
  constexpr double kNeck = 1.52;
  constexpr double kHeadBottom = 1.52;
  constexpr double kHeadSide = 0.14;
  constexpr double kHeadTop =
      cr::kCreativeArchitecturalHumanReferenceHeightMeters;

  appendLine(output, {x - 0.16, floorY, z}, {x, floorY + kHip, z},
             kHumanColor, kThickness);
  appendLine(output, {x + 0.16, floorY, z}, {x, floorY + kHip, z},
             kHumanColor, kThickness);
  appendLine(output, {x, floorY + kHip, z}, {x, floorY + kNeck, z},
             kHumanColor, kThickness);
  appendLine(output, {x - 0.34, floorY + kShoulder, z},
             {x + 0.34, floorY + kShoulder, z}, kHumanColor, kThickness);
  appendLine(output, {x, floorY + kHeadBottom, z},
             {x + kHeadSide, floorY + (kHeadBottom + kHeadTop) * 0.5, z},
             kHumanColor, kThickness);
  appendLine(output,
             {x + kHeadSide, floorY + (kHeadBottom + kHeadTop) * 0.5, z},
             {x, floorY + kHeadTop, z}, kHumanColor, kThickness);
  appendLine(output, {x, floorY + kHeadTop, z},
             {x - kHeadSide, floorY + (kHeadBottom + kHeadTop) * 0.5, z},
             kHumanColor, kThickness);
  appendLine(output,
             {x - kHeadSide, floorY + (kHeadBottom + kHeadTop) * 0.5, z},
             {x, floorY + kHeadBottom, z}, kHumanColor, kThickness);
}

}  // namespace

void appendCreativeEditorArchitectureScaleGuide(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  const CreativeEditorWorldLayoutState& state = request.editor.worldLayout;
  if (state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::Building ||
      state.generatedRevision != state.revision) {
    return;
  }
  const std::size_t buildingIndex =
      creativeEditorWorldLayoutSelectedBuilding(state);
  const cr::CreativeDocument& document = request.appState.facade.document();
  const cr::CreativeWorldLayoutBuildingDimensions dimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          document.gridSettings(), state.source, buildingIndex);
  if (!dimensions.accepted) {
    return;
  }

  const double cellSize = document.gridSettings().cellSizeMeters;
  const double margin = std::max(0.75, cellSize * 0.75);
  const double tickHalfWidth = std::max(0.25, cellSize * 0.35);
  const double guideX = dimensions.footprintMinimumXMeters - margin;
  const double guideZ = dimensions.footprintMinimumZMeters - margin;
  constexpr iggy3d::RenderLineColor kDimensionColor{
      0.28F, 1.0F, 0.36F, 1.0F};
  constexpr float kDimensionThickness = 0.045F;
  const std::size_t firstLine = output.combinedWireLines.size();

  appendLine(output,
             {guideX, dimensions.lowestFloorBottomMeters, guideZ},
             {guideX, dimensions.roofTopMeters, guideZ}, kDimensionColor,
             kDimensionThickness);
  appendLine(output,
             {guideX - tickHalfWidth,
              dimensions.lowestFloorBottomMeters, guideZ},
             {guideX + tickHalfWidth,
              dimensions.lowestFloorBottomMeters, guideZ},
             kDimensionColor, kDimensionThickness);
  appendLine(output,
             {guideX - tickHalfWidth, dimensions.roofTopMeters, guideZ},
             {guideX + tickHalfWidth, dimensions.roofTopMeters, guideZ},
             kDimensionColor, kDimensionThickness);

  for (std::size_t levelIndex = 0U;
       levelIndex < state.source.levels.size(); ++levelIndex) {
    const cr::CreativeWorldLayoutLevel& level =
        state.source.levels[levelIndex];
    if (level.buildingIndex != buildingIndex ||
        !cr::creativeWorldLayoutLevelHasRooms(state.source, levelIndex)) {
      continue;
    }
    const cr::CreativeWorldLayoutLevelDimensions levelDimensions =
        cr::measureCreativeWorldLayoutLevelDimensions(
            document.gridSettings(), state.source, levelIndex);
    if (!levelDimensions.accepted) {
      return;
    }
    appendLine(output,
               {guideX - tickHalfWidth * 0.65,
                levelDimensions.floorTopMeters, guideZ},
               {guideX + tickHalfWidth * 0.65,
                levelDimensions.floorTopMeters, guideZ},
               kDimensionColor, kDimensionThickness);
  }

  appendHumanReference(output, guideX - margin,
                       dimensions.lowestFloorTopMeters, guideZ);
  output.architectureScaleGuideActive = true;
  output.architectureScaleGuideLineCount =
      output.combinedWireLines.size() - firstLine;
  output.architecturalDimensions = dimensions;
}

}  // namespace iggy3d_creative_app
