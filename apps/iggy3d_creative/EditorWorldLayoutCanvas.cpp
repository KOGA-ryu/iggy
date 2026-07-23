#include "EditorWorldLayoutCanvasInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorToolDescriptor.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
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

void queueGesture(CreativeDesktopCommandFrame& commands,
                  CreativeEditorWorldLayoutGesturePhase phase,
                  CreativeEditorWorldLayoutPoint point = {}) {
  commands.push(CreativeDesktopCommandId::WorldLayoutCanvasGesture,
                CreativeDesktopWorldLayoutGesturePayload{phase, point});
}

void queueRoomManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateRoom,
      CreativeDesktopWorldLayoutRoomManipulationPayload{phase, point,
                                                        toleranceCells});
}

void queueRoomBoundaryManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoomBoundaryManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
      CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload{
          phase, point, toleranceCells});
}

void queueRoomCornerManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoomCornerManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner,
      CreativeDesktopWorldLayoutRoomCornerManipulationPayload{
          phase, point, toleranceCells});
}

void queueVerticalConnectorManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          phase, point, toleranceCells, {}});
}

void queueBuildingManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutBuildingManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
      CreativeDesktopWorldLayoutBuildingManipulationPayload{
          phase, point, toleranceCells});
}

void queueBoxManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutBoxManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateBox,
      CreativeDesktopWorldLayoutBoxManipulationPayload{phase, point,
                                                       toleranceCells});
}

void queueWallManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutWallManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateWall,
      CreativeDesktopWorldLayoutWallManipulationPayload{phase, point,
                                                        toleranceCells});
}

void queueOpeningManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutOpeningManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateOpening,
      CreativeDesktopWorldLayoutOpeningManipulationPayload{
          phase, point, toleranceCells});
}

void queueRoofApertureManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoofApertureManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
      CreativeDesktopWorldLayoutRoofApertureManipulationPayload{
          phase, point, toleranceCells});
}

void queueRoofManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoofManipulationPhase phase,
    CreativeEditorWorldLayoutRoofTarget target,
    double coordinateCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateRoof,
      CreativeDesktopWorldLayoutRoofManipulationPayload{
          phase, target, coordinateCells});
}

double planRoofHandleCoordinate(
    CreativeEditorWorldLayoutRoofHandleKind handle,
    CreativeEditorWorldLayoutPoint point) noexcept {
  switch (handle) {
    case CreativeEditorWorldLayoutRoofHandleKind::NorthEave:
      return -point.z;
    case CreativeEditorWorldLayoutRoofHandleKind::EastEave:
      return point.x;
    case CreativeEditorWorldLayoutRoofHandleKind::SouthEave:
      return point.z;
    case CreativeEditorWorldLayoutRoofHandleKind::WestEave:
      return -point.x;
    case CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight:
      return point.z;
    case CreativeEditorWorldLayoutRoofHandleKind::None:
    case CreativeEditorWorldLayoutRoofHandleKind::Count:
      break;
  }
  return 0.0;
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

void queuePlanSourceSelection(
    CreativeDesktopCommandFrame& commands,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutPlanHit& hit) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
      CreativeDesktopWorldLayoutSourcePayload{
          hit.table, hit.sourceIndex,
          std::string(creativeEditorWorldLayoutSourceStableKey(
              state, hit.table, hit.sourceIndex)),
          hit.sourceLevelIndex});
}

CreativeEditorSelectionComposition selectionComposition(
    bool additive, bool toggle) noexcept {
  if (toggle) {
    return CreativeEditorSelectionComposition::Toggle;
  }
  return additive ? CreativeEditorSelectionComposition::Add
                  : CreativeEditorSelectionComposition::Replace;
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
  commands.push(CreativeDesktopCommandId::SelectObjects,
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

bool queuePlanSourceInteraction(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands,
    const CreativeEditorWorldLayoutPlanHit& hit,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (!hit.hit) {
    return false;
  }
  if (hit.table == cr::CreativeWorldLayoutTable::Object) {
    const CreativeEditorWorldLayoutEditReceipt begun =
        beginCreativeEditorWorldLayoutObjectManipulation(
            state, hit.sourceIndex, point);
    if (!begun.accepted) {
      return false;
    }
    // The manipulation owns the immediate 2D state. The queued semantic
    // selection synchronizes its generated 3D members without restarting the
    // already-active drag.
    queuePlanSourceSelection(commands, state, hit);
    return true;
  }
  queuePlanSourceSelection(commands, state, hit);
  switch (hit.table) {
    case cr::CreativeWorldLayoutTable::Room:
      if (state.source.roomBoundaries.empty()) {
        queueRoomManipulation(
            commands, CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
            point, toleranceCells);
      } else {
        const CreativeEditorWorldLayoutRoomCornerTarget corner =
            findCreativeEditorWorldLayoutRoomCornerTarget(
                state, hit.sourceIndex, point, toleranceCells);
        if (corner.topologyVertexIndex !=
            cr::kInvalidCreativeWorldLayoutIndex) {
          queueRoomCornerManipulation(
              commands,
              CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin,
              point, toleranceCells);
        } else {
          queueRoomBoundaryManipulation(
              commands,
              CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
              point, toleranceCells);
        }
      }
      break;
    case cr::CreativeWorldLayoutTable::TopologyEdge:
      queueRoomBoundaryManipulation(
          commands,
          CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
          point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      queueVerticalConnectorManipulation(
          commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin,
          point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::Box:
      queueBoxManipulation(
          commands, CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
          point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      queueWallManipulation(
          commands, CreativeEditorWorldLayoutWallManipulationPhase::Begin,
          point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::Opening:
      queueOpeningManipulation(
          commands, CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
          point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::RoofAperture:
      queueRoofApertureManipulation(
          commands,
          CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin,
          point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::Building:
    case cr::CreativeWorldLayoutTable::Level:
    case cr::CreativeWorldLayoutTable::Object:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      break;
  }
  return true;
}

void queueBuildingTemplatePlacement(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutBuildingTemplatePlacementPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    cr::CreativeWorldLayoutBuildingTransformOperation operation =
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90) {
  commands.push(
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
    commands.push(CreativeDesktopCommandId::WorldLayoutTransformBuilding,
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

bool selectedBuildingContains(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (state.selection.kind !=
      CreativeEditorWorldLayoutSelectionKind::Building) {
    return false;
  }
  CreativeEditorWorldLayoutBuildingBounds bounds;
  if (!readCreativeEditorWorldLayoutBuildingBounds(
          state, state.selection.index, bounds) ||
      !std::isfinite(point.x) || !std::isfinite(point.z)) {
    return false;
  }
  return point.x >= bounds.minimum.x - toleranceCells &&
         point.x <= bounds.maximum.x + toleranceCells &&
         point.z >= bounds.minimum.z - toleranceCells &&
         point.z <= bounds.maximum.z + toleranceCells;
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
      commands.push(
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
      commands.push(
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
      commands.push(
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
  ImGuiIO& io = ImGui::GetIO();

  CanvasTransform transform{
      {minimum.x + canvasSize.x * 0.5F + state.canvasPanX,
       minimum.y + canvasSize.y * 0.5F + state.canvasPanZ},
      state.canvasPixelsPerCell};

  if (hovered && io.MouseWheel != 0.0F) {
    const CreativeEditorWorldLayoutPoint before =
        toWorld(transform, io.MousePos);
    state.canvasPixelsPerCell = std::clamp(
        state.canvasPixelsPerCell * (io.MouseWheel > 0.0F ? 1.15F : 0.87F),
        12.0F, 80.0F);
    transform.pixelsPerCell = state.canvasPixelsPerCell;
    const ImVec2 anchored = toScreen(transform, before.x, before.z);
    state.canvasPanX += io.MousePos.x - anchored.x;
    state.canvasPanZ += io.MousePos.y - anchored.y;
    transform.origin.x += io.MousePos.x - anchored.x;
    transform.origin.y += io.MousePos.y - anchored.y;
  }
  if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    state.canvasPanX += io.MouseDelta.x;
    state.canvasPanZ += io.MouseDelta.y;
    transform.origin.x += io.MouseDelta.x;
    transform.origin.y += io.MouseDelta.y;
  }
  const ImVec2 boundedPointer{
      std::clamp(io.MousePos.x, minimum.x, maximum.x),
      std::clamp(io.MousePos.y, minimum.y, maximum.y)};
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
      semanticHoverEnabled
          ? hitCreativeEditorWorldLayoutPlanStack(
                editor.worldLayoutPlanView, state, displaySource,
                {semanticHoverPoint.x, semanticHoverPoint.z},
                semanticHitTolerance)
          : CreativeEditorWorldLayoutPlanHitStack{};
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
          document.measurementAnnotationStore(), measurement, io.MousePos,
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
        io.AppFocusLost ||
        (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape);
    if (cancel) {
      if (terrainRegion.manipulation.active) {
        if (cancelCreativeEditorWorldLayoutTerrainRegionManipulation(
                terrainRegion)) {
          commands.push(
              CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
        }
        return;
      }
      commands.push(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
      return;
    }
    const bool previewOwnedElsewhere =
        editor.terrainGeneration.previewActive &&
        !terrainRegion.ownsPreview;
    if (previewOwnedElsewhere) {
      return;
    }
    const bool leftClicked =
        hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (leftClicked && io.KeyShift) {
      const CreativeEditorWorldLayoutTerrainAnalysisEditPlan edit =
          planCreativeEditorWorldLayoutTerrainAnalysisEdit(
              topography.plan, semanticHoverPoint.x, semanticHoverPoint.z,
              semanticHitTolerance,
              cr::CreativeTerrainAnalysisHitMode::HeightHandleOnly);
      if (selectCreativeEditorWorldLayoutTerrainAnalysisEdit(terrainRegion,
                                                              edit)) {
        commands.push(
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
        commands.push(
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
    if (terrainRegion.manipulation.active &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      static_cast<void>(
          updateCreativeEditorWorldLayoutTerrainRegionManipulation(
              terrainRegion, hoveredPoint.x, hoveredPoint.z));
    } else if (terrainRegion.selecting &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      static_cast<void>(updateCreativeEditorWorldLayoutTerrainRegion(
          terrainRegion, hoveredPoint.x, hoveredPoint.z));
    }
    if (terrainRegion.manipulation.active &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      if (finishCreativeEditorWorldLayoutTerrainRegionManipulation(
              terrainRegion, hoveredPoint.x, hoveredPoint.z)) {
        commands.push(
            CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
      }
    } else if (terrainRegion.selecting &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      if (finishCreativeEditorWorldLayoutTerrainRegion(
              terrainRegion, hoveredPoint.x, hoveredPoint.z)) {
        commands.push(
            CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
      }
    }
    return;
  }

  if (state.buildingTemplatePlacement.active) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    const bool cancelPlacement =
        io.AppFocusLost ||
        (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape);
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
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
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
        io.AppFocusLost || ImGui::IsKeyPressed(ImGuiKey_Escape) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    if (cancelSelection) {
      state.planRegionSelection = {};
      return;
    }
    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
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
        commands.push(
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
        selectionComposition(gesture.additive, gesture.toggle), commands));
    return;
  }

  const CreativeEditorWorldLayoutOpeningTarget hoveredOpeningTarget =
      hovered && selectionToolActive &&
              hoveredPlanHit.table == cr::CreativeWorldLayoutTable::Opening
          ? findCreativeEditorWorldLayoutOpeningTarget(
                state, hoveredPoint, handleTolerance)
          : CreativeEditorWorldLayoutOpeningTarget{};
  const CreativeEditorWorldLayoutWallTarget hoveredWallTarget =
      hovered && selectionToolActive &&
              hoveredPlanHit.table == cr::CreativeWorldLayoutTable::Wall
          ? findCreativeEditorWorldLayoutWallTarget(state, hoveredPoint,
                                                    handleTolerance)
          : CreativeEditorWorldLayoutWallTarget{};
  const CreativeEditorWorldLayoutVerticalConnectorTarget
      hoveredVerticalConnectorTarget =
          hovered && selectionToolActive &&
                  hoveredPlanHit.table ==
                      cr::CreativeWorldLayoutTable::VerticalConnector
              ? findCreativeEditorWorldLayoutVerticalConnectorTarget(
                    state, hoveredPoint, handleTolerance)
              : CreativeEditorWorldLayoutVerticalConnectorTarget{};
  const CreativeEditorWorldLayoutRoomTarget hoveredRoomTarget =
      hovered && selectionToolActive &&
              hoveredPlanHit.table == cr::CreativeWorldLayoutTable::Room
          ? findCreativeEditorWorldLayoutRoomTarget(state, hoveredPoint,
                                                    handleTolerance)
          : CreativeEditorWorldLayoutRoomTarget{};
  const CreativeEditorWorldLayoutRoomBoundaryTarget
      hoveredRoomBoundaryTarget =
          hovered && selectionToolActive &&
                  hoveredPlanHit.table == cr::CreativeWorldLayoutTable::Room
              ? findCreativeEditorWorldLayoutRoomBoundaryTarget(
                    state, hoveredPoint, handleTolerance)
              : CreativeEditorWorldLayoutRoomBoundaryTarget{};
  const CreativeEditorWorldLayoutRoomCornerTarget hoveredRoomCornerTarget =
      hovered && selectionToolActive &&
              hoveredPlanHit.table == cr::CreativeWorldLayoutTable::Room
          ? findCreativeEditorWorldLayoutRoomCornerTarget(
                state, hoveredPlanHit.sourceIndex, hoveredPoint,
                handleTolerance)
          : CreativeEditorWorldLayoutRoomCornerTarget{};
  const CreativeEditorWorldLayoutBoxTarget hoveredBoxTarget =
      hovered && selectionToolActive &&
              hoveredPlanHit.table == cr::CreativeWorldLayoutTable::Box
          ? findCreativeEditorWorldLayoutBoxTarget(state, hoveredPoint,
                                                   handleTolerance)
          : CreativeEditorWorldLayoutBoxTarget{};
  const CreativeEditorWorldLayoutRoofApertureTarget
      hoveredRoofApertureTarget =
          hovered && selectionToolActive &&
                  hoveredPlanHit.table ==
                      cr::CreativeWorldLayoutTable::RoofAperture
              ? findCreativeEditorWorldLayoutRoofApertureTarget(
                    state, hoveredPoint, handleTolerance)
              : CreativeEditorWorldLayoutRoofApertureTarget{};
  const std::size_t roofLevelIndex =
      state.roofManipulation.active
          ? state.roofManipulation.target.levelIndex
          : state.selection.kind ==
                    CreativeEditorWorldLayoutSelectionKind::Level
                ? state.selection.index
                : cr::kInvalidCreativeWorldLayoutIndex;
  const CreativeEditorWorldLayoutRoofHandleFrame roofHandleFrame =
      buildCreativeEditorWorldLayoutRoofHandleFrame(state, grid,
                                                     roofLevelIndex);
  const CreativeEditorWorldLayoutRoofTarget hoveredRoofTarget =
      hovered && selectionToolActive
          ? findCreativeEditorWorldLayoutPlanRoofHandle(
                roofHandleFrame, hoveredPoint, handleTolerance)
          : CreativeEditorWorldLayoutRoofTarget{};
  const std::size_t hoveredObjectIndex =
      hoveredPlanHit.hit &&
              hoveredPlanHit.table == cr::CreativeWorldLayoutTable::Object
          ? hoveredPlanHit.sourceIndex
          : cr::kInvalidCreativeWorldLayoutIndex;
  const bool hoveredBuilding =
      hovered && selectionToolActive &&
      selectedBuildingContains(state, hoveredPoint, handleTolerance);
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
    if (hoveredOpeningTarget.handle !=
        CreativeEditorWorldLayoutOpeningHandle::None) {
      ImGui::SetMouseCursor(
          openingHandleCursor(state, hoveredOpeningTarget));
    } else if (hoveredWallTarget.handle !=
               CreativeEditorWorldLayoutWallHandle::None) {
      ImGui::SetMouseCursor(wallHandleCursor(state, hoveredWallTarget));
    } else if (hoveredObjectIndex != cr::kInvalidCreativeWorldLayoutIndex) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    } else if (hoveredBuilding) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    } else if (hoveredVerticalConnectorTarget.directionHandle) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    } else if (hoveredVerticalConnectorTarget.handle !=
               CreativeEditorWorldLayoutRectHandle::None) {
      ImGui::SetMouseCursor(
          rectHandleCursor(hoveredVerticalConnectorTarget.handle));
    } else if (hoveredRoomTarget.handle !=
               CreativeEditorWorldLayoutRoomHandle::None) {
      ImGui::SetMouseCursor(rectHandleCursor(hoveredRoomTarget.handle));
    } else if (hoveredRoomCornerTarget.topologyVertexIndex !=
               cr::kInvalidCreativeWorldLayoutIndex) {
      ImGui::SetMouseCursor(hoveredRoomCornerTarget.northWestSouthEast
                                ? ImGuiMouseCursor_ResizeNWSE
                                : ImGuiMouseCursor_ResizeNESW);
    } else if (hoveredRoomBoundaryTarget.topologyEdgeIndex !=
               cr::kInvalidCreativeWorldLayoutIndex) {
      ImGui::SetMouseCursor(hoveredRoomBoundaryTarget.horizontal
                                ? ImGuiMouseCursor_ResizeNS
                                : ImGuiMouseCursor_ResizeEW);
    } else if (hoveredBoxTarget.handle !=
               CreativeEditorWorldLayoutBoxHandle::None) {
      ImGui::SetMouseCursor(rectHandleCursor(hoveredBoxTarget.handle));
    } else if (hoveredRoofTarget.handle !=
               CreativeEditorWorldLayoutRoofHandleKind::None) {
      ImGui::SetMouseCursor(roofHandleCursor(hoveredRoofTarget.handle));
    } else if (hoveredRoofApertureTarget.handle !=
               CreativeEditorWorldLayoutRoofApertureHandle::None) {
      ImGui::SetMouseCursor(
          rectHandleCursor(hoveredRoofApertureTarget.handle));
    } else if (hoveredPlanHit.hit) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
  }

  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    if (selectionToolActive) {
      const bool additive = io.KeyShift;
      const bool toggle = io.KeySuper || io.KeyCtrl;
      if ((additive || toggle) && hoveredPlanHit.hit) {
        const cr::CreativeWorldLayoutSourceRef source{
            hoveredPlanHit.table, hoveredPlanHit.sourceIndex};
        static_cast<void>(queuePlanObjectSelection(
            state, document, selection,
            std::span<const cr::CreativeWorldLayoutSourceRef>{&source, 1U},
            selectionComposition(additive, toggle), commands));
      } else if ((additive || toggle) && hoveredBuilding &&
                 state.selection.kind ==
                     CreativeEditorWorldLayoutSelectionKind::Building) {
        const cr::CreativeWorldLayoutSourceRef source{
            cr::CreativeWorldLayoutTable::Building, state.selection.index};
        static_cast<void>(queuePlanObjectSelection(
            state, document, selection,
            std::span<const cr::CreativeWorldLayoutSourceRef>{&source, 1U},
            selectionComposition(additive, toggle), commands));
      } else if (hoveredRoofTarget.handle !=
                 CreativeEditorWorldLayoutRoofHandleKind::None) {
        queueRoofManipulation(
            commands, CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
            hoveredRoofTarget,
            planRoofHandleCoordinate(hoveredRoofTarget.handle, hoveredPoint));
      } else if (hoveredBuilding) {
        queueBuildingManipulation(
            commands,
            CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
            hoveredPoint, handleTolerance);
      } else if (hoveredPlanHit.hit) {
        const CreativeEditorWorldLayoutPlanHit cycledPlanHit =
            cycleCreativeEditorWorldLayoutPlanHit(hoveredPlanStack,
                                                  state.selection);
        static_cast<void>(queuePlanSourceInteraction(
            state, commands, cycledPlanHit, hoveredPoint, handleTolerance));
      } else {
        state.planRegionSelection = {
            true, pointerPoint, pointerPoint, additive, toggle};
      }
    } else if (dragTool(state.tool)) {
      queueGesture(commands, CreativeEditorWorldLayoutGesturePhase::Begin,
                   hoveredPoint);
    } else {
      commands.push(CreativeDesktopCommandId::WorldLayoutCanvasPoint,
                    CreativeDesktopWorldLayoutPointPayload{hoveredPoint});
    }
  }

  const bool cancelObjectManipulation =
      state.objectManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelObjectManipulation) {
    static_cast<void>(
        cancelCreativeEditorWorldLayoutObjectManipulation(state));
  } else if (state.objectManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    const CreativeEditorWorldLayoutEditReceipt updated =
        updateCreativeEditorWorldLayoutObjectManipulation(state, pointerPoint);
    if (updated.accepted && state.objectManipulation.active &&
        state.objectManipulation.previewValid) {
      const std::size_t objectIndex = state.objectManipulation.objectIndex;
      std::string stableKey = state.objectManipulation.stableKey;
      CreativeEditorWorldLayoutObjectSettings settings =
          std::move(state.objectManipulation.previewSettings);
      state.objectManipulation = {};
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSetObjectSettings,
          CreativeDesktopWorldLayoutObjectSettingsPayload{
              objectIndex, std::move(stableKey), std::move(settings)});
    } else {
      state.objectManipulation = {};
    }
  } else if (state.objectManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    static_cast<void>(updateCreativeEditorWorldLayoutObjectManipulation(
        state, pointerPoint));
  }

  const bool cancelRoofManipulation =
      state.roofManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelRoofManipulation) {
    queueRoofManipulation(
        commands, CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
        state.roofManipulation.target, 0.0);
  } else if (state.roofManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueRoofManipulation(
        commands, CreativeEditorWorldLayoutRoofManipulationPhase::Commit,
        state.roofManipulation.target,
        planRoofHandleCoordinate(state.roofManipulation.target.handle,
                                 pointerPoint));
  } else if (state.roofManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueRoofManipulation(
        commands, CreativeEditorWorldLayoutRoofManipulationPhase::Update,
        state.roofManipulation.target,
        planRoofHandleCoordinate(state.roofManipulation.target.handle,
                                 pointerPoint));
  }

  const bool cancelBuildingManipulation =
      state.buildingManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelBuildingManipulation) {
    queueBuildingManipulation(
        commands,
        CreativeEditorWorldLayoutBuildingManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.buildingManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueBuildingManipulation(
        commands,
        CreativeEditorWorldLayoutBuildingManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.buildingManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueBuildingManipulation(
        commands,
        CreativeEditorWorldLayoutBuildingManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelOpeningManipulation =
      state.openingManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelOpeningManipulation) {
    queueOpeningManipulation(
        commands, CreativeEditorWorldLayoutOpeningManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.openingManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueOpeningManipulation(
        commands, CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.openingManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueOpeningManipulation(
        commands, CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelWallManipulation =
      state.wallManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelWallManipulation) {
    queueWallManipulation(
        commands, CreativeEditorWorldLayoutWallManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.wallManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueWallManipulation(
        commands, CreativeEditorWorldLayoutWallManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.wallManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueWallManipulation(
        commands, CreativeEditorWorldLayoutWallManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelRoomManipulation =
      state.roomManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelRoomManipulation) {
    queueRoomManipulation(
        commands, CreativeEditorWorldLayoutRoomManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.roomManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueRoomManipulation(
        commands, CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.roomManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueRoomManipulation(
        commands, CreativeEditorWorldLayoutRoomManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelRoomBoundaryManipulation =
      state.roomBoundaryManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelRoomBoundaryManipulation) {
    queueRoomBoundaryManipulation(
        commands,
        CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.roomBoundaryManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueRoomBoundaryManipulation(
        commands,
        CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.roomBoundaryManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueRoomBoundaryManipulation(
        commands,
        CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelRoomCornerManipulation =
      state.roomCornerManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelRoomCornerManipulation) {
    queueRoomCornerManipulation(
        commands,
        CreativeEditorWorldLayoutRoomCornerManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.roomCornerManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueRoomCornerManipulation(
        commands,
        CreativeEditorWorldLayoutRoomCornerManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.roomCornerManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueRoomCornerManipulation(
        commands,
        CreativeEditorWorldLayoutRoomCornerManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelVerticalConnectorManipulation =
      state.verticalConnectorManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelVerticalConnectorManipulation) {
    queueVerticalConnectorManipulation(
        commands,
        CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.verticalConnectorManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueVerticalConnectorManipulation(
        commands,
        CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.verticalConnectorManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueVerticalConnectorManipulation(
        commands,
        CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelBoxManipulation =
      state.boxManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelBoxManipulation) {
    queueBoxManipulation(
        commands, CreativeEditorWorldLayoutBoxManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.boxManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueBoxManipulation(
        commands, CreativeEditorWorldLayoutBoxManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.boxManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueBoxManipulation(
        commands, CreativeEditorWorldLayoutBoxManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelRoofApertureManipulation =
      state.roofApertureManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelRoofApertureManipulation) {
    queueRoofApertureManipulation(
        commands,
        CreativeEditorWorldLayoutRoofApertureManipulationPhase::Cancel,
        pointerPoint, handleTolerance);
  } else if (state.roofApertureManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueRoofApertureManipulation(
        commands,
        CreativeEditorWorldLayoutRoofApertureManipulationPhase::Commit,
        pointerPoint, handleTolerance);
  } else if (state.roofApertureManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueRoofApertureManipulation(
        commands,
        CreativeEditorWorldLayoutRoofApertureManipulationPhase::Update,
        pointerPoint, handleTolerance);
  }

  const bool cancelGesture =
      !state.objectManipulation.active && !state.buildingManipulation.active &&
      !state.openingManipulation.active && !state.wallManipulation.active &&
      !state.verticalConnectorManipulation.active &&
      !state.roomManipulation.active &&
      !state.roomCornerManipulation.active &&
      !state.roomBoundaryManipulation.active &&
      !state.boxManipulation.active &&
      !state.roofApertureManipulation.active &&
      !state.roofManipulation.active &&
      state.anchorActive &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  const bool finishTerrainPathDraft =
      state.terrainPathDraft.active &&
      (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
       (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)));
  if (cancelGesture) {
    queueGesture(commands, CreativeEditorWorldLayoutGesturePhase::Cancel);
  } else if (finishTerrainPathDraft) {
    queueGesture(commands, CreativeEditorWorldLayoutGesturePhase::Commit);
  } else if (dragTool(state.tool) &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
             ImGui::IsItemDeactivated()) {
    queueGesture(commands, CreativeEditorWorldLayoutGesturePhase::Commit,
                 hoveredPoint);
  } else if (state.anchorActive && dragTool(state.tool) &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueGesture(commands, CreativeEditorWorldLayoutGesturePhase::Update,
                 hoveredPoint);
  }
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
    CreativeEditorWorldLayoutCanvasHoverStatus* hoverStatus) {
  drawLayoutCanvas(editor, document, selection, measurement, commands,
                   interactionEnabled, hoverStatus);
}

}  // namespace iggy3d_creative_app
