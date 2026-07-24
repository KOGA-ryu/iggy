#include "EditorWorldLayoutCanvasInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorToolDescriptor.hpp"
#include "EditorWorldLayoutCanvasPlanner.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "EditorWorldLayoutPlan.hpp"
#include "EditorWorldLayoutBuildings.hpp"
#include "EditorWorldLayoutOpenings.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

using CanvasTransform = CreativeEditorWorldLayoutCanvasTransform;

ImVec2 toScreen(const CanvasTransform& transform, double x, double z) {
  return creativeEditorWorldLayoutCanvasToScreen(transform, x, z);
}

CreativeEditorWorldLayoutPoint toWorld(const CanvasTransform& transform,
                                       ImVec2 screen) {
  return creativeEditorWorldLayoutCanvasToWorld(transform, screen);
}

ImGuiMouseCursor rectHandleCursor(
    CreativeEditorWorldLayoutRectHandle handle) noexcept {
  switch (handle) {
    case CreativeEditorWorldLayoutRectHandle::Move:
      return ImGuiMouseCursor_ResizeAll;
    case CreativeEditorWorldLayoutRectHandle::North:
    case CreativeEditorWorldLayoutRectHandle::South:
      return ImGuiMouseCursor_ResizeNS;
    case CreativeEditorWorldLayoutRectHandle::East:
    case CreativeEditorWorldLayoutRectHandle::West:
      return ImGuiMouseCursor_ResizeEW;
    case CreativeEditorWorldLayoutRectHandle::NorthWest:
    case CreativeEditorWorldLayoutRectHandle::SouthEast:
      return ImGuiMouseCursor_ResizeNWSE;
    case CreativeEditorWorldLayoutRectHandle::NorthEast:
    case CreativeEditorWorldLayoutRectHandle::SouthWest:
      return ImGuiMouseCursor_ResizeNESW;
    case CreativeEditorWorldLayoutRectHandle::None:
    case CreativeEditorWorldLayoutRectHandle::Count:
      break;
  }
  return ImGuiMouseCursor_Arrow;
}

ImGuiMouseCursor terrainRegionHandleCursor(
    CreativeEditorWorldLayoutTerrainRegionHandle handle) noexcept {
  switch (handle) {
    case CreativeEditorWorldLayoutTerrainRegionHandle::Body:
      return ImGuiMouseCursor_ResizeAll;
    case CreativeEditorWorldLayoutTerrainRegionHandle::MinimumX:
    case CreativeEditorWorldLayoutTerrainRegionHandle::MaximumX:
      return ImGuiMouseCursor_ResizeEW;
    case CreativeEditorWorldLayoutTerrainRegionHandle::MinimumZ:
    case CreativeEditorWorldLayoutTerrainRegionHandle::MaximumZ:
      return ImGuiMouseCursor_ResizeNS;
    case CreativeEditorWorldLayoutTerrainRegionHandle::MinimumXMinimumZ:
    case CreativeEditorWorldLayoutTerrainRegionHandle::MaximumXMaximumZ:
      return ImGuiMouseCursor_ResizeNWSE;
    case CreativeEditorWorldLayoutTerrainRegionHandle::MaximumXMinimumZ:
    case CreativeEditorWorldLayoutTerrainRegionHandle::MinimumXMaximumZ:
      return ImGuiMouseCursor_ResizeNESW;
    case CreativeEditorWorldLayoutTerrainRegionHandle::None:
    case CreativeEditorWorldLayoutTerrainRegionHandle::Count:
      break;
  }
  return ImGuiMouseCursor_Hand;
}

void queueRoomManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateRoom,
      CreativeDesktopWorldLayoutRoomManipulationPayload{phase, point,
                                                        toleranceCells});
}

void queueRoomBoundaryManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoomBoundaryManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
      CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload{
          phase, point, toleranceCells});
}

void queueRoomCornerManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoomCornerManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner,
      CreativeDesktopWorldLayoutRoomCornerManipulationPayload{
          phase, point, toleranceCells});
}

void queueVerticalConnectorManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          phase, point, toleranceCells, {}});
}

void queueBuildingManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutBuildingManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
      CreativeDesktopWorldLayoutBuildingManipulationPayload{
          phase, point, toleranceCells});
}

void queueBoxManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutBoxManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateBox,
      CreativeDesktopWorldLayoutBoxManipulationPayload{phase, point,
                                                       toleranceCells});
}

void queueWallManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutWallManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateWall,
      CreativeDesktopWorldLayoutWallManipulationPayload{phase, point,
                                                        toleranceCells});
}

void queueOpeningManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutOpeningManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateOpening,
      CreativeDesktopWorldLayoutOpeningManipulationPayload{
          phase, point, toleranceCells});
}

void queueRoofApertureManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoofApertureManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
      CreativeDesktopWorldLayoutRoofApertureManipulationPayload{
          phase, point, toleranceCells});
}

void queueRoofManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoofManipulationPhase phase,
    CreativeEditorWorldLayoutRoofTarget target,
    double coordinateCells) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutManipulateRoof,
      CreativeDesktopWorldLayoutRoofManipulationPayload{
          phase, target, coordinateCells});
}

ImGuiMouseCursor roofHandleCursor(
    CreativeEditorWorldLayoutRoofHandleKind handle) noexcept {
  switch (handle) {
    case CreativeEditorWorldLayoutRoofHandleKind::NorthEave:
    case CreativeEditorWorldLayoutRoofHandleKind::SouthEave:
    case CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight:
      return ImGuiMouseCursor_ResizeNS;
    case CreativeEditorWorldLayoutRoofHandleKind::EastEave:
    case CreativeEditorWorldLayoutRoofHandleKind::WestEave:
      return ImGuiMouseCursor_ResizeEW;
    case CreativeEditorWorldLayoutRoofHandleKind::None:
    case CreativeEditorWorldLayoutRoofHandleKind::Count:
      break;
  }
  return ImGuiMouseCursor_Arrow;
}

bool queuePlanObjectSelection(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeSelectionState& currentSelection,
    std::span<const cr::CreativeWorldLayoutSourceRef> sources,
    CreativeEditorSelectionComposition composition,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorObjectSelectionPlan plan =
      planCreativeEditorWorldLayoutObjectSelection(
          state, document, currentSelection, sources, composition);
  if (!plan.accepted) {
    state.statusMessage = plan.overflowed
                              ? "selection exceeds the 4096-object limit"
                              : "selection source is unavailable";
    return false;
  }
  commands.enqueue(CreativeDesktopCommandId::SelectObjects,
                CreativeDesktopSelectPayload{std::move(plan.objectIds),
                                             plan.primaryObjectId});
  return true;
}

ImU32 planRegionColor(CreativeEditorDraftingRole role,
                      std::uint8_t alpha) noexcept {
  const CreativeEditorDraftingColor tint =
      creativeEditorDraftingStyle(role).tint;
  return IM_COL32(tint.r, tint.g, tint.b, alpha);
}

void drawPlanRegionSelection(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CanvasTransform& transform,
    const CreativeEditorWorldLayoutPlanRegionSelectionGesture& gesture) {
  if (!gesture.active) {
    return;
  }
  const ImVec2 anchor =
      toScreen(transform, gesture.anchor.x, gesture.anchor.z);
  const ImVec2 current =
      toScreen(transform, gesture.current.x, gesture.current.z);
  const ImVec2 regionMinimum{std::min(anchor.x, current.x),
                             std::min(anchor.y, current.y)};
  const ImVec2 regionMaximum{std::max(anchor.x, current.x),
                             std::max(anchor.y, current.y)};
  const CreativeEditorDraftingRole role =
      gesture.current.x >= gesture.anchor.x
          ? CreativeEditorDraftingRole::SelectedOverlay
          : CreativeEditorDraftingRole::HoverOverlay;
  drawList.PushClipRect(minimum, maximum, true);
  drawList.AddRectFilled(regionMinimum, regionMaximum,
                         planRegionColor(role, 28U));
  drawList.AddRect(regionMinimum, regionMaximum,
                   planRegionColor(role, 230U), 0.0F, 0, 1.5F);
  drawList.PopClipRect();
}

void queueBuildingTemplatePlacement(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutBuildingTemplatePlacementPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    cr::CreativeWorldLayoutBuildingTransformOperation operation =
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
      CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
          phase, point, operation});
}

void queueLayoutManipulationCancel(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  if (state.planRegionSelection.active) {
    state.planRegionSelection = {};
  } else if (state.elevationManipulation.active) {
    state.elevationManipulation = {};
  } else if (state.objectManipulation.active) {
    static_cast<void>(
        cancelCreativeEditorWorldLayoutObjectManipulation(state));
  } else if (state.buildingTransform.active) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutTransformBuilding,
                  CreativeDesktopWorldLayoutBuildingTransformPayload{
                      CreativeEditorWorldLayoutBuildingTransformPhase::Cancel,
                      state.buildingTransform.operation});
  } else if (state.buildingTemplatePlacement.active) {
    queueBuildingTemplatePlacement(
        commands,
        CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Cancel);
  } else if (state.openingManipulation.active) {
    queueOpeningManipulation(
        commands, CreativeEditorWorldLayoutOpeningManipulationPhase::Cancel, {},
        0.25);
  } else if (state.roofManipulation.active) {
    queueRoofManipulation(
        commands, CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
        state.roofManipulation.target, 0.0);
  } else if (state.roofApertureManipulation.active) {
    queueRoofApertureManipulation(
        commands,
        CreativeEditorWorldLayoutRoofApertureManipulationPhase::Cancel, {},
        0.25);
  } else if (state.verticalConnectorManipulation.active) {
    queueVerticalConnectorManipulation(
        commands,
        CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel,
        {}, 0.25);
  } else if (state.buildingManipulation.active) {
    queueBuildingManipulation(
        commands,
        CreativeEditorWorldLayoutBuildingManipulationPhase::Cancel, {}, 0.25);
  } else if (state.wallManipulation.active) {
    queueWallManipulation(
        commands, CreativeEditorWorldLayoutWallManipulationPhase::Cancel, {},
        0.25);
  } else if (state.roomManipulation.active) {
    queueRoomManipulation(
        commands, CreativeEditorWorldLayoutRoomManipulationPhase::Cancel, {},
        0.25);
  } else if (state.roomCornerManipulation.active) {
    queueRoomCornerManipulation(
        commands,
        CreativeEditorWorldLayoutRoomCornerManipulationPhase::Cancel, {},
        0.25);
  } else if (state.roomBoundaryManipulation.active) {
    queueRoomBoundaryManipulation(
        commands,
        CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Cancel, {},
        0.25);
  } else if (state.boxManipulation.active) {
    queueBoxManipulation(
        commands, CreativeEditorWorldLayoutBoxManipulationPhase::Cancel, {},
        0.25);
  }
}

ImGuiMouseCursor openingHandleCursor(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutOpeningTarget target) {
  const CreativeEditorWorldLayoutOpeningHost host =
      resolveCreativeEditorWorldLayoutOpeningHost(state, target.openingIndex);
  if (!host.valid) {
    return ImGuiMouseCursor_Arrow;
  }
  return std::fabs(host.end.x - host.start.x) >=
                 std::fabs(host.end.z - host.start.z)
             ? ImGuiMouseCursor_ResizeEW
             : ImGuiMouseCursor_ResizeNS;
}

ImGuiMouseCursor wallHandleCursor(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutWallTarget target) {
  if (target.handle == CreativeEditorWorldLayoutWallHandle::Move) {
    return ImGuiMouseCursor_ResizeAll;
  }
  if (target.wallIndex >= state.source.walls.size()) {
    return ImGuiMouseCursor_Arrow;
  }
  const cr::CreativeWorldLayoutWall& wall = state.source.walls[target.wallIndex];
  return wall.start.z == wall.end.z ? ImGuiMouseCursor_ResizeEW
                                   : ImGuiMouseCursor_ResizeNS;
}

bool dragTool(CreativeEditorWorldLayoutTool tool) noexcept {
  return describeCreativeEditorWorldLayoutTool(tool)
             .worldLayoutInputProfile ==
         CreativeEditorWorldLayoutInputProfile::Drag;
}

// Compact canvas navigation only. Authoring parameters stay in Properties and
// workflow Apply/Cancel controls stay in Build, so each dock owns one concern.
void drawWorldLayoutViewControls(CreativeEditorWorldLayoutState& state,
                                 CreativeEditorWorldLayoutTopographyState&
                                     topography,
                                 CreativeDesktopCommandFrame& commands) {
  constexpr float kTileSize = 24.0F;
  const bool plan = state.viewMode == CreativeEditorWorldLayoutViewMode::Plan;
  if (drawCreativeEditorWorldLayoutGlyphButton(
          "##world_layout_view_plan", CreativeEditorToolGlyph::ViewPlan,
          kTileSize, plan, "Plan view") &&
      !plan) {
    queueLayoutManipulationCancel(state, commands);
    state.viewMode = CreativeEditorWorldLayoutViewMode::Plan;
  }
  ImGui::SameLine();
  const bool elevation =
      state.viewMode == CreativeEditorWorldLayoutViewMode::Elevation;
  if (drawCreativeEditorWorldLayoutGlyphButton(
          "##world_layout_view_elevation",
          CreativeEditorToolGlyph::ViewElevation, kTileSize, elevation,
          "Elevation view") &&
      !elevation) {
    queueLayoutManipulationCancel(state, commands);
    if (topography.region.editingEnabled) {
      topography.region.editingEnabled = false;
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
    state.viewMode = CreativeEditorWorldLayoutViewMode::Elevation;
  }
  if (state.viewMode == CreativeEditorWorldLayoutViewMode::Plan) {
    const cr::CreativeWorldLayoutLevelNavigationResult lower =
        cr::navigateCreativeWorldLayoutLevel(
            state.source, state.activeLevelIndex,
            cr::CreativeWorldLayoutLevelNavigationDirection::Lower);
    const cr::CreativeWorldLayoutLevelNavigationResult higher =
        cr::navigateCreativeWorldLayoutLevel(
            state.source, state.activeLevelIndex,
            cr::CreativeWorldLayoutLevelNavigationDirection::Higher);
    const bool lowerReady =
        lower.status == cr::CreativeWorldLayoutLevelNavigationStatus::Ready;
    const bool higherReady =
        higher.status == cr::CreativeWorldLayoutLevelNavigationStatus::Ready;

    ImGui::SameLine();
    ImGui::BeginDisabled(!lowerReady);
    const bool lowerPressed = drawCreativeEditorWorldLayoutGlyphButton(
        "##world_layout_level_down", CreativeEditorToolGlyph::LevelDown,
        kTileSize, false,
        lowerReady ? "Show the next lower floor" : "No lower floor");
    ImGui::EndDisabled();
    if (lowerPressed && lowerReady) {
      const cr::CreativeWorldLayoutLevel& target =
          state.source.levels[lower.targetLevelIndex];
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutLevelOperation,
          CreativeDesktopWorldLayoutLevelOperationPayload{
              CreativeEditorWorldLayoutLevelOperation::Select,
              target.buildingIndex, lower.targetLevelIndex});
    }

    ImGui::SameLine();
    if (state.activeLevelIndex < state.source.levels.size()) {
      const cr::CreativeWorldLayoutLevel& active =
          state.source.levels[state.activeLevelIndex];
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Floor %.3f", active.floorTopLayer);
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s\nElevation %.3f grid layers",
                          active.name.c_str(), active.floorTopLayer);
      }
    } else {
      ImGui::AlignTextToFramePadding();
      ImGui::TextDisabled("No active floor");
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!higherReady);
    const bool higherPressed = drawCreativeEditorWorldLayoutGlyphButton(
        "##world_layout_level_up", CreativeEditorToolGlyph::LevelUp,
        kTileSize, false,
        higherReady ? "Show the next higher floor" : "No higher floor");
    ImGui::EndDisabled();
    if (higherPressed && higherReady) {
      const cr::CreativeWorldLayoutLevel& target =
          state.source.levels[higher.targetLevelIndex];
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutLevelOperation,
          CreativeDesktopWorldLayoutLevelOperationPayload{
              CreativeEditorWorldLayoutLevelOperation::Select,
              target.buildingIndex, higher.targetLevelIndex});
    }

    ImGui::SameLine();
    if (drawCreativeEditorWorldLayoutGlyphButton(
            "##world_layout_lower_context",
            CreativeEditorToolGlyph::LowerLevelContext, kTileSize,
            state.planLowerLevelContextVisible,
            "Show the nearest lower floor as context")) {
      state.planLowerLevelContextVisible =
          !state.planLowerLevelContextVisible;
    }
    ImGui::SameLine();
    if (drawCreativeEditorWorldLayoutGlyphButton(
            "##world_layout_upper_context",
            CreativeEditorToolGlyph::UpperLevelContext, kTileSize,
            state.planUpperLevelContextVisible,
            "Show the nearest upper floor as context")) {
      state.planUpperLevelContextVisible =
          !state.planUpperLevelContextVisible;
    }
    ImGui::SameLine();
    if (drawCreativeEditorWorldLayoutGlyphButton(
            "##world_layout_roof_overhead",
            CreativeEditorToolGlyph::RoofVisibility, kTileSize,
            state.planRoofOverheadVisible, "Show roof overhead")) {
      state.planRoofOverheadVisible = !state.planRoofOverheadVisible;
    }
    ImGui::SameLine();
    if (drawCreativeEditorWorldLayoutGlyphButton(
            "##world_layout_contours", CreativeEditorToolGlyph::Contours,
            kTileSize, topography.visible, "Show terrain contours")) {
      topography.visible = !topography.visible;
    }

    if (topography.visible) {
      ImGui::Checkbox("Elevation bands", &topography.elevationBandsVisible);
      ImGui::SameLine();
      ImGui::Checkbox("Slope bands", &topography.slopeBandsVisible);
      ImGui::SameLine();
      ImGui::Checkbox("Labels", &topography.contourLabelsVisible);
      ImGui::SameLine();
      ImGui::Checkbox("Cut / fill", &topography.cutFillVisible);
      int interval = static_cast<int>(topography.intervalCells);
      ImGui::SetNextItemWidth(112.0F);
      if (ImGui::SliderInt(
              "Contour##world_layout_topography", &interval, 1,
              static_cast<int>(
                  cr::kCreativeTerrainContourMaximumIntervalCells),
              "%d cells")) {
        topography.intervalCells = static_cast<std::uint16_t>(interval);
      }
      int majorEvery = static_cast<int>(topography.majorEvery);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(90.0F);
      if (ImGui::SliderInt(
              "Index##world_layout_topography", &majorEvery, 1,
              static_cast<int>(cr::kCreativeTerrainContourMaximumMajorEvery),
              "%d")) {
        topography.majorEvery = static_cast<std::uint16_t>(majorEvery);
      }
    }
    return;
  }
  ImGui::SameLine();
  ImGui::TextDisabled("Axis");
  ImGui::SameLine();
  const bool axisX =
      state.elevationAxis == CreativeEditorWorldLayoutElevationAxis::X;
  if (ImGui::RadioButton("X", axisX) && !axisX) {
    state.elevationAxis = CreativeEditorWorldLayoutElevationAxis::X;
    state.elevationManipulation = {};
  }
  ImGui::SameLine();
  const bool axisZ =
      state.elevationAxis == CreativeEditorWorldLayoutElevationAxis::Z;
  if (ImGui::RadioButton("Z", axisZ) && !axisZ) {
    state.elevationAxis = CreativeEditorWorldLayoutElevationAxis::Z;
    state.elevationManipulation = {};
  }
}

void drawLayoutCanvas(CreativeEditorState& editor,
                      const cr::CreativeDocument& document,
                      const cr::CreativeSelectionState& selection,
                      const cr::CreativeMeasurementState& measurement,
                      CreativeDesktopCommandFrame& commands,
                      bool interactionEnabled,
                      const CreativeEditorUiInputFrame& input,
                      CreativeEditorWorldLayoutCanvasHoverStatus* hoverStatus) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  CreativeEditorWorldLayoutTopographyState& topography =
      editor.worldLayoutTopography;
  const bool selectionToolActive =
      state.tool == CreativeEditorWorldLayoutTool::Select;
  const bool previewActive = creativeEditorWorldLayoutPreviewActive(state);
  const cr::CreativeDocument& renderDocument =
      creativeEditorWorldLayoutRenderDocument(state, document);
  const bool terrainGenerationPreviewActive =
      !previewActive && &renderDocument == &document &&
      creativeEditorTerrainGenerationPreviewMatches(
          editor.terrainGeneration, document);
  const cr::CreativeTerrainHeightField* terrainHeightOverride =
      terrainGenerationPreviewActive
          ? &editor.terrainGeneration.operationPreview.heightField
          : nullptr;
  const bool topographySourceOverride =
      previewActive || terrainGenerationPreviewActive;
  const std::uint64_t topographySourceKey =
      terrainGenerationPreviewActive
          ? editor.terrainGeneration.operationPreview.receipt.replay.heightHash
          : previewActive ? state.previewContentRevision : 0U;
  const cr::CreativeGridSettings grid = renderDocument.gridSettings();
  static_cast<void>(refreshCreativeEditorWorldLayoutTopography(
      topography, renderDocument, topographySourceOverride,
      topographySourceKey, terrainHeightOverride));
  static_cast<void>(refreshCreativeEditorWorldLayoutPlanView(
      editor.worldLayoutPlanView, state, topography, grid));
  const ImVec2 available = ImGui::GetContentRegionAvail();
  const ImVec2 canvasSize{std::max(available.x, 160.0F),
                          std::max(available.y, 160.0F)};
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  const ImVec2 maximum{minimum.x + canvasSize.x, minimum.y + canvasSize.y};
  ImGui::InvisibleButton(
      "##world_layout_canvas", canvasSize,
      ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle |
          ImGuiButtonFlags_MouseButtonRight);
  const bool hovered = ImGui::IsItemHovered();
  const ImVec2 pointerPosition{input.pointer.x, input.pointer.y};

  CanvasTransform transform{
      {minimum.x + canvasSize.x * 0.5F + state.canvasPanX,
       minimum.y + canvasSize.y * 0.5F + state.canvasPanZ},
      state.canvasPixelsPerCell};

  if (hovered && input.pointer.wheelY != 0.0F) {
    const CreativeEditorWorldLayoutPoint before =
        toWorld(transform, pointerPosition);
    state.canvasPixelsPerCell = std::clamp(
        state.canvasPixelsPerCell *
            (input.pointer.wheelY > 0.0F ? 1.15F : 0.87F),
        12.0F, 80.0F);
    transform.pixelsPerCell = state.canvasPixelsPerCell;
    const ImVec2 anchored = toScreen(transform, before.x, before.z);
    state.canvasPanX += pointerPosition.x - anchored.x;
    state.canvasPanZ += pointerPosition.y - anchored.y;
    transform.origin.x += pointerPosition.x - anchored.x;
    transform.origin.y += pointerPosition.y - anchored.y;
  }
  if (hovered && input.pointer.middleDragging) {
    state.canvasPanX += input.pointer.deltaX;
    state.canvasPanZ += input.pointer.deltaY;
    transform.origin.x += input.pointer.deltaX;
    transform.origin.y += input.pointer.deltaY;
  }
  const ImVec2 boundedPointer{
      std::clamp(pointerPosition.x, minimum.x, maximum.x),
      std::clamp(pointerPosition.y, minimum.y, maximum.y)};
  const CreativeEditorWorldLayoutPoint semanticHoverPoint =
      toWorld(transform, boundedPointer);
  const double semanticHitTolerance = std::clamp(
      8.0 / static_cast<double>(transform.pixelsPerCell), 0.10, 0.45);
  const bool manipulationActive =
      state.objectManipulation.active || state.buildingManipulation.active ||
      state.openingManipulation.active || state.wallManipulation.active ||
      state.verticalConnectorManipulation.active ||
      state.roomManipulation.active ||
      state.roomCornerManipulation.active ||
      state.roomBoundaryManipulation.active || state.boxManipulation.active ||
      state.roofApertureManipulation.active || state.roofManipulation.active ||
      state.planRegionSelection.active;
  const bool semanticHoverEnabled =
      interactionEnabled && hovered && selectionToolActive &&
      !topography.region.editingEnabled &&
      !state.buildingTemplatePlacement.active && !manipulationActive;
  const cr::CreativeWorldLayout& displaySource =
      creativeEditorWorldLayoutDisplaySource(state);
  const CreativeEditorWorldLayoutPlanHitStack hoveredPlanStack =
      planCreativeEditorWorldLayoutCanvasHitStack(
          editor.worldLayoutPlanView, state, displaySource,
          semanticHoverPoint, semanticHitTolerance, semanticHoverEnabled);
  const CreativeEditorWorldLayoutPlanHit hoveredPlanHit =
      hoveredPlanStack.count > 0U ? hoveredPlanStack.items.front()
                                  : CreativeEditorWorldLayoutPlanHit{};
  if (hovered && hoverStatus != nullptr) {
    hoverStatus->present = true;
    hoverStatus->cellX = semanticHoverPoint.x;
    hoverStatus->cellZ = semanticHoverPoint.z;
    if (hoveredPlanHit.hit &&
        hoveredPlanHit.primitiveIndex <
            editor.worldLayoutPlanView.projection.primitives.size()) {
      hoverStatus->semanticRole = toString(
          creativeEditorWorldLayoutPlanDraftingRole(
              editor.worldLayoutPlanView.projection
                  .primitives[hoveredPlanHit.primitiveIndex]));
    }
  }

  if (state.planRegionSelection.active) {
    state.planRegionSelection.current = semanticHoverPoint;
  }

  const CreativeEditorWorldLayoutCanvasPointerGeometry pointerGeometry =
      drawCreativeEditorWorldLayoutCanvasScene(
          *ImGui::GetWindowDrawList(), minimum, maximum, transform, state,
          topography, editor.worldLayoutPlanView,
          hoveredPlanHit.primitiveIndex, grid,
          document.measurementAnnotationStore(), measurement, pointerPosition,
          hovered);
  const CreativeEditorWorldLayoutPoint pointerPoint =
      pointerGeometry.pointerPoint;
  const CreativeEditorWorldLayoutPoint hoveredPoint =
      pointerGeometry.hoveredPoint;
  const double handleTolerance = pointerGeometry.handleToleranceCells;
  drawPlanRegionSelection(*ImGui::GetWindowDrawList(), minimum, maximum,
                          transform, state.planRegionSelection);

  if (!interactionEnabled) {
    return;
  }

  CreativeEditorWorldLayoutTerrainRegionState& terrainRegion =
      topography.region;
  if (terrainRegion.editingEnabled) {
    const CreativeEditorWorldLayoutTerrainRegionHandle regionHandle =
        terrainRegion.manipulation.active
            ? terrainRegion.manipulation.handle
            : hitCreativeEditorWorldLayoutTerrainRegionHandle(
                  terrainRegion, hoveredPoint.x, hoveredPoint.z,
                  handleTolerance);
    if (hovered) {
      ImGui::SetMouseCursor(terrainRegionHandleCursor(regionHandle));
    }
    const bool cancel =
        input.pointer.focusLost ||
        (hovered && input.pointer.secondaryPressed) || input.cancelPressed;
    if (cancel) {
      if (terrainRegion.manipulation.active) {
        if (cancelCreativeEditorWorldLayoutTerrainRegionManipulation(
                terrainRegion)) {
          commands.enqueue(
              CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
        }
        return;
      }
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
      return;
    }
    const bool previewOwnedElsewhere =
        editor.terrainGeneration.previewActive &&
        !terrainRegion.ownsPreview;
    if (previewOwnedElsewhere) {
      return;
    }
    const bool leftClicked = hovered && input.pointer.primaryPressed;
    if (leftClicked && input.selectionAdditiveDown) {
      const CreativeEditorWorldLayoutTerrainAnalysisEditPlan edit =
          planCreativeEditorWorldLayoutTerrainAnalysisEdit(
              topography.plan, semanticHoverPoint.x, semanticHoverPoint.z,
              semanticHitTolerance,
              cr::CreativeTerrainAnalysisHitMode::HeightHandleOnly);
      if (selectCreativeEditorWorldLayoutTerrainAnalysisEdit(terrainRegion,
                                                              edit)) {
        commands.enqueue(
            CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
      }
      return;
    }
    if (leftClicked) {
      const CreativeEditorWorldLayoutTerrainAnalysisEditPlan edit =
          planCreativeEditorWorldLayoutTerrainAnalysisEdit(
              topography.plan, semanticHoverPoint.x, semanticHoverPoint.z,
              semanticHitTolerance,
              cr::CreativeTerrainAnalysisHitMode::ContourOnly);
      if (edit.accepted &&
          selectCreativeEditorWorldLayoutTerrainAnalysisEdit(terrainRegion,
                                                              edit)) {
        commands.enqueue(
            CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
        return;
      }
    }
    if (leftClicked) {
      if (regionHandle !=
          CreativeEditorWorldLayoutTerrainRegionHandle::None) {
        static_cast<void>(
            beginCreativeEditorWorldLayoutTerrainRegionManipulation(
                terrainRegion, regionHandle, hoveredPoint.x, hoveredPoint.z));
      } else {
        static_cast<void>(beginCreativeEditorWorldLayoutTerrainRegion(
            terrainRegion, hoveredPoint.x, hoveredPoint.z));
      }
    }
    if (terrainRegion.manipulation.active && input.pointer.primaryDown) {
      static_cast<void>(
          updateCreativeEditorWorldLayoutTerrainRegionManipulation(
              terrainRegion, hoveredPoint.x, hoveredPoint.z));
    } else if (terrainRegion.selecting && input.pointer.primaryDown) {
      static_cast<void>(updateCreativeEditorWorldLayoutTerrainRegion(
          terrainRegion, hoveredPoint.x, hoveredPoint.z));
    }
    if (terrainRegion.manipulation.active && input.pointer.primaryReleased) {
      if (finishCreativeEditorWorldLayoutTerrainRegionManipulation(
              terrainRegion, hoveredPoint.x, hoveredPoint.z)) {
        commands.enqueue(
            CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
      }
    } else if (terrainRegion.selecting && input.pointer.primaryReleased) {
      if (finishCreativeEditorWorldLayoutTerrainRegion(
              terrainRegion, hoveredPoint.x, hoveredPoint.z)) {
        commands.enqueue(
            CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
      }
    }
    return;
  }

  if (state.buildingTemplatePlacement.active) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    const bool cancelPlacement =
        input.pointer.focusLost ||
        (hovered && input.pointer.secondaryPressed) || input.cancelPressed;
    if (cancelPlacement) {
      queueBuildingTemplatePlacement(
          commands,
          CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Cancel);
      return;
    }
    if (hovered) {
      queueBuildingTemplatePlacement(
          commands,
          CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Update,
          hoveredPoint);
      if (input.pointer.primaryPressed) {
        queueBuildingTemplatePlacement(
            commands,
            CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Commit,
            hoveredPoint);
      }
    }
    return;
  }

  if (state.planRegionSelection.active) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    const bool cancelSelection =
        input.pointer.focusLost || input.cancelPressed ||
        input.pointer.secondaryPressed;
    if (cancelSelection) {
      state.planRegionSelection = {};
      return;
    }
    if (!input.pointer.primaryReleased) {
      return;
    }

    const CreativeEditorWorldLayoutPlanRegionSelectionGesture gesture =
        state.planRegionSelection;
    state.planRegionSelection = {};
    const double dragPixelsX =
        (gesture.current.x - gesture.anchor.x) * transform.pixelsPerCell;
    const double dragPixelsZ =
        (gesture.current.z - gesture.anchor.z) * transform.pixelsPerCell;
    constexpr double kRegionDragThresholdPixels = 4.0;
    if (dragPixelsX * dragPixelsX + dragPixelsZ * dragPixelsZ <
        kRegionDragThresholdPixels * kRegionDragThresholdPixels) {
      if (!gesture.additive && !gesture.toggle) {
        commands.enqueue(
            CreativeDesktopCommandId::WorldLayoutClearSelection);
      }
      return;
    }

    const CreativeEditorWorldLayoutPlanRegionSelection region =
        selectCreativeEditorWorldLayoutPlanRegion(
            editor.worldLayoutPlanView, state, displaySource,
            {gesture.anchor.x, gesture.anchor.z},
            {gesture.current.x, gesture.current.z});
    if (!region.accepted) {
      state.statusMessage = region.overflowed
                                ? "region exceeds the 4096-source limit"
                                : "region selection is unavailable";
      return;
    }
    static_cast<void>(queuePlanObjectSelection(
        state, document, selection, region.sources,
        creativeEditorWorldLayoutCanvasSelectionComposition(
            gesture.additive, gesture.toggle),
        commands));
    return;
  }

  const CreativeEditorWorldLayoutCanvasTargetSelection hoveredTargets =
      planCreativeEditorWorldLayoutCanvasTargets(
          state, grid, hoveredPlanHit, hoveredPoint, handleTolerance,
          hovered && selectionToolActive);
  if (state.objectManipulation.active) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  } else if (state.buildingManipulation.active) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  } else if (state.openingManipulation.active) {
    ImGui::SetMouseCursor(openingHandleCursor(
        state, state.openingManipulation.target));
  } else if (state.wallManipulation.active) {
    ImGui::SetMouseCursor(
        wallHandleCursor(state, state.wallManipulation.target));
  } else if (state.verticalConnectorManipulation.active) {
    ImGui::SetMouseCursor(
        state.verticalConnectorManipulation.target.directionHandle
            ? ImGuiMouseCursor_Hand
            : rectHandleCursor(
                  state.verticalConnectorManipulation.target.handle));
  } else if (state.roomManipulation.active) {
    ImGui::SetMouseCursor(
        rectHandleCursor(state.roomManipulation.target.handle));
  } else if (state.roomCornerManipulation.active) {
    ImGui::SetMouseCursor(
        state.roomCornerManipulation.target.northWestSouthEast
            ? ImGuiMouseCursor_ResizeNWSE
            : ImGuiMouseCursor_ResizeNESW);
  } else if (state.roomBoundaryManipulation.active) {
    ImGui::SetMouseCursor(
        state.roomBoundaryManipulation.target.horizontal
            ? ImGuiMouseCursor_ResizeNS
            : ImGuiMouseCursor_ResizeEW);
  } else if (state.boxManipulation.active) {
    ImGui::SetMouseCursor(
        rectHandleCursor(state.boxManipulation.target.handle));
  } else if (state.roofManipulation.active) {
    ImGui::SetMouseCursor(
        roofHandleCursor(state.roofManipulation.target.handle));
  } else if (state.roofApertureManipulation.active) {
    ImGui::SetMouseCursor(
        rectHandleCursor(state.roofApertureManipulation.target.handle));
  } else if (hovered && selectionToolActive) {
    if (hoveredTargets.opening.handle !=
        CreativeEditorWorldLayoutOpeningHandle::None) {
      ImGui::SetMouseCursor(
          openingHandleCursor(state, hoveredTargets.opening));
    } else if (hoveredTargets.wall.handle !=
               CreativeEditorWorldLayoutWallHandle::None) {
      ImGui::SetMouseCursor(wallHandleCursor(state, hoveredTargets.wall));
    } else if (hoveredTargets.objectIndex !=
               cr::kInvalidCreativeWorldLayoutIndex) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    } else if (hoveredTargets.building) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    } else if (hoveredTargets.verticalConnector.directionHandle) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    } else if (hoveredTargets.verticalConnector.handle !=
               CreativeEditorWorldLayoutRectHandle::None) {
      ImGui::SetMouseCursor(
          rectHandleCursor(hoveredTargets.verticalConnector.handle));
    } else if (hoveredTargets.room.handle !=
               CreativeEditorWorldLayoutRoomHandle::None) {
      ImGui::SetMouseCursor(rectHandleCursor(hoveredTargets.room.handle));
    } else if (hoveredTargets.roomCorner.topologyVertexIndex !=
               cr::kInvalidCreativeWorldLayoutIndex) {
      ImGui::SetMouseCursor(hoveredTargets.roomCorner.northWestSouthEast
                                ? ImGuiMouseCursor_ResizeNWSE
                                : ImGuiMouseCursor_ResizeNESW);
    } else if (hoveredTargets.roomBoundary.topologyEdgeIndex !=
               cr::kInvalidCreativeWorldLayoutIndex) {
      ImGui::SetMouseCursor(hoveredTargets.roomBoundary.horizontal
                                ? ImGuiMouseCursor_ResizeNS
                                : ImGuiMouseCursor_ResizeEW);
    } else if (hoveredTargets.box.handle !=
               CreativeEditorWorldLayoutBoxHandle::None) {
      ImGui::SetMouseCursor(rectHandleCursor(hoveredTargets.box.handle));
    } else if (hoveredTargets.roof.handle !=
               CreativeEditorWorldLayoutRoofHandleKind::None) {
      ImGui::SetMouseCursor(roofHandleCursor(hoveredTargets.roof.handle));
    } else if (hoveredTargets.roofAperture.handle !=
               CreativeEditorWorldLayoutRoofApertureHandle::None) {
      ImGui::SetMouseCursor(
          rectHandleCursor(hoveredTargets.roofAperture.handle));
    } else if (hoveredPlanHit.hit) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
  }

  const CreativeEditorWorldLayoutCanvasPressPlan pressPlan =
      planCreativeEditorWorldLayoutCanvasPress(
          {hovered && input.pointer.primaryPressed,
           selectionToolActive,
           input.selectionAdditiveDown,
           input.selectionToggleDown,
           dragTool(state.tool),
           pointerPoint,
           hoveredPoint,
           handleTolerance},
          hoveredTargets, hoveredPlanStack, state.selection);
  switch (pressPlan.kind) {
    case CreativeEditorWorldLayoutCanvasPressKind::SelectSource:
    case CreativeEditorWorldLayoutCanvasPressKind::SelectBuilding:
      static_cast<void>(queuePlanObjectSelection(
          state, document, selection,
          std::span<const cr::CreativeWorldLayoutSourceRef>{
              &pressPlan.source, 1U},
          pressPlan.composition, commands));
      break;
    case CreativeEditorWorldLayoutCanvasPressKind::InteractSource: {
      CreativeEditorWorldLayoutCanvasSourceInteractionPlan interaction =
          planCreativeEditorWorldLayoutCanvasSourceInteraction(
              state, pressPlan.hit, hoveredPoint, handleTolerance);
      if (interaction.beginObjectManipulation) {
        const CreativeEditorWorldLayoutEditReceipt begun =
            beginCreativeEditorWorldLayoutObjectManipulation(
                state, interaction.hit.sourceIndex, hoveredPoint);
        if (!begun.accepted) {
          break;
        }
      }
      enqueueCreativeEditorWorldLayoutCanvasCommandPlan(
          commands, std::move(interaction.commands));
      break;
    }
    case CreativeEditorWorldLayoutCanvasPressKind::BeginRegionSelection:
      state.planRegionSelection = pressPlan.regionSelection;
      break;
    case CreativeEditorWorldLayoutCanvasPressKind::BeginRoof:
    case CreativeEditorWorldLayoutCanvasPressKind::BeginBuilding:
    case CreativeEditorWorldLayoutCanvasPressKind::BeginGesture:
    case CreativeEditorWorldLayoutCanvasPressKind::ApplyPoint:
      enqueueCreativeEditorWorldLayoutCanvasCommandPlan(
          commands, pressPlan.commands);
      break;
    case CreativeEditorWorldLayoutCanvasPressKind::None:
    case CreativeEditorWorldLayoutCanvasPressKind::Count:
      break;
  }

  const bool cancelObjectManipulation =
      state.objectManipulation.active &&
      (input.pointer.focusLost ||
       (hovered && input.pointer.secondaryPressed) ||
       input.cancelPressed);
  if (cancelObjectManipulation) {
    static_cast<void>(
        cancelCreativeEditorWorldLayoutObjectManipulation(state));
  } else if (state.objectManipulation.active &&
             input.pointer.primaryReleased) {
    const CreativeEditorWorldLayoutEditReceipt updated =
        updateCreativeEditorWorldLayoutObjectManipulation(state, pointerPoint);
    if (updated.accepted && state.objectManipulation.active &&
        state.objectManipulation.previewValid) {
      const std::size_t objectIndex = state.objectManipulation.objectIndex;
      std::string stableKey = state.objectManipulation.stableKey;
      CreativeEditorWorldLayoutObjectSettings settings =
          std::move(state.objectManipulation.previewSettings);
      state.objectManipulation = {};
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutSetObjectSettings,
          CreativeDesktopWorldLayoutObjectSettingsPayload{
              objectIndex, std::move(stableKey), std::move(settings)});
    } else {
      state.objectManipulation = {};
    }
  } else if (state.objectManipulation.active &&
             input.pointer.primaryDown) {
    static_cast<void>(updateCreativeEditorWorldLayoutObjectManipulation(
        state, pointerPoint));
  }

  enqueueCreativeEditorWorldLayoutCanvasCommandPlan(
      commands,
      planCreativeEditorWorldLayoutCanvasManipulations(
          {state.objectManipulation.active,
           state.roofManipulation.active,
           state.roofManipulation.target,
           state.buildingManipulation.active,
           state.openingManipulation.active,
           state.wallManipulation.active,
           state.roomManipulation.active,
           state.roomBoundaryManipulation.active,
           state.roomCornerManipulation.active,
           state.verticalConnectorManipulation.active,
           state.boxManipulation.active,
           state.roofApertureManipulation.active,
           state.anchorActive,
           state.terrainPathDraft.active,
           dragTool(state.tool)},
          {hovered,
           input.pointer.focusLost,
           input.pointer.secondaryPressed,
           input.cancelPressed,
           input.pointer.primaryReleased,
           input.pointer.primaryDown,
           ImGui::IsItemDeactivated(),
           input.confirmPressed,
           input.pointer.primaryDoubleClicked,
           pointerPoint,
           hoveredPoint,
           handleTolerance}));
}


}  // namespace

void cancelCreativeEditorWorldLayoutPanelManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  queueLayoutManipulationCancel(state, commands);
}

void drawCreativeEditorWorldLayoutViewControls(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutTopographyState& topography,
    CreativeDesktopCommandFrame& commands) {
  drawWorldLayoutViewControls(state, topography, commands);
}

void drawCreativeEditorWorldLayoutCanvas(
    CreativeEditorState& editor, const cr::CreativeDocument& document,
    const cr::CreativeSelectionState& selection,
    const cr::CreativeMeasurementState& measurement,
    CreativeDesktopCommandFrame& commands, bool interactionEnabled,
    const CreativeEditorUiInputFrame& input,
    CreativeEditorWorldLayoutCanvasHoverStatus* hoverStatus) {
  drawLayoutCanvas(editor, document, selection, measurement, commands,
                   interactionEnabled, input, hoverStatus);
}

}  // namespace iggy3d_creative_app
