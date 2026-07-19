#include "EditorWorldLayoutCanvasInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorWorldLayout.hpp"
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

void queueVerticalConnectorManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          phase, point, toleranceCells});
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
  if (state.elevationManipulation.active) {
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
  return tool == CreativeEditorWorldLayoutTool::BuildingShell ||
         tool == CreativeEditorWorldLayoutTool::Room ||
         tool == CreativeEditorWorldLayoutTool::Floor ||
         tool == CreativeEditorWorldLayoutTool::Wall ||
         creativeEditorWorldLayoutToolIsVerticalConnector(tool) ||
         tool == CreativeEditorWorldLayoutTool::Road ||
         tool == CreativeEditorWorldLayoutTool::Ditch ||
         tool == CreativeEditorWorldLayoutTool::Bridge;
}

// Left tools tab: mode toggle, operation, and selection shape only. The
// parameters live in the Properties window and the workflow status plus the
// sole Apply/Cancel controls live in the Build window, so the plan canvas
// keeps its space and each dock owns one concern.
void drawWorldLayoutViewControls(CreativeEditorWorldLayoutState& state,
                                 CreativeEditorWorldLayoutTopographyState&
                                     topography,
                                 CreativeDesktopCommandFrame& commands) {
  const bool plan = state.viewMode == CreativeEditorWorldLayoutViewMode::Plan;
  if (ImGui::RadioButton("Plan", plan) && !plan) {
    queueLayoutManipulationCancel(state, commands);
    state.viewMode = CreativeEditorWorldLayoutViewMode::Plan;
  }
  ImGui::SameLine();
  const bool elevation =
      state.viewMode == CreativeEditorWorldLayoutViewMode::Elevation;
  if (ImGui::RadioButton("Elevation", elevation) && !elevation) {
    queueLayoutManipulationCancel(state, commands);
    if (topography.region.editingEnabled) {
      topography.region.editingEnabled = false;
      commands.push(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
    state.viewMode = CreativeEditorWorldLayoutViewMode::Elevation;
  }
  if (state.viewMode == CreativeEditorWorldLayoutViewMode::Plan) {
    ImGui::SameLine();
    ImGui::Checkbox("Topography", &topography.visible);
    if (topography.visible) {
      ImGui::SameLine();
      ImGui::Checkbox("Bands", &topography.elevationBandsVisible);
      int interval = static_cast<int>(topography.intervalCells);
      ImGui::SameLine();
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
                      CreativeDesktopCommandFrame& commands,
                      bool interactionEnabled,
                      CreativeEditorWorldLayoutCanvasHoverStatus* hoverStatus) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  CreativeEditorWorldLayoutTopographyState& topography =
      editor.worldLayoutTopography;
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
          : previewActive ? state.previewLayoutRevision : 0U;
  const cr::CreativeGridSettings grid = renderDocument.gridSettings();
  static_cast<void>(refreshCreativeEditorWorldLayoutTopography(
      topography, renderDocument, topographySourceOverride,
      topographySourceKey, terrainHeightOverride));
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
  if (hovered && hoverStatus != nullptr) {
    const CreativeEditorWorldLayoutPoint hoveredWorld =
        toWorld(transform, io.MousePos);
    hoverStatus->present = true;
    hoverStatus->cellX = hoveredWorld.x;
    hoverStatus->cellZ = hoveredWorld.z;
  }

  const CreativeEditorWorldLayoutCanvasPointerGeometry pointerGeometry =
      drawCreativeEditorWorldLayoutCanvasScene(
          *ImGui::GetWindowDrawList(), minimum, maximum, transform, state,
          topography, grid, io.MousePos, hovered);
  const CreativeEditorWorldLayoutPoint pointerPoint =
      pointerGeometry.pointerPoint;
  const CreativeEditorWorldLayoutPoint hoveredPoint =
      pointerGeometry.hoveredPoint;
  const double handleTolerance = pointerGeometry.handleToleranceCells;

  if (!interactionEnabled) {
    return;
  }

  CreativeEditorWorldLayoutTerrainRegionState& terrainRegion =
      topography.region;
  if (terrainRegion.editingEnabled) {
    if (hovered) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    const bool cancel =
        io.AppFocusLost ||
        (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape);
    if (cancel) {
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
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      static_cast<void>(beginCreativeEditorWorldLayoutTerrainRegion(
          terrainRegion, hoveredPoint.x, hoveredPoint.z));
    }
    if (terrainRegion.selecting &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      static_cast<void>(updateCreativeEditorWorldLayoutTerrainRegion(
          terrainRegion, hoveredPoint.x, hoveredPoint.z));
    }
    if (terrainRegion.selecting &&
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

  const CreativeEditorWorldLayoutOpeningTarget hoveredOpeningTarget =
      hovered && state.tool == CreativeEditorWorldLayoutTool::Select
          ? findCreativeEditorWorldLayoutOpeningTarget(
                state, hoveredPoint, handleTolerance)
          : CreativeEditorWorldLayoutOpeningTarget{};
  const CreativeEditorWorldLayoutWallTarget hoveredWallTarget =
      hovered && state.tool == CreativeEditorWorldLayoutTool::Select
          ? findCreativeEditorWorldLayoutWallTarget(state, hoveredPoint,
                                                    handleTolerance)
          : CreativeEditorWorldLayoutWallTarget{};
  const CreativeEditorWorldLayoutVerticalConnectorTarget
      hoveredVerticalConnectorTarget =
          hovered && state.tool == CreativeEditorWorldLayoutTool::Select
              ? findCreativeEditorWorldLayoutVerticalConnectorTarget(
                    state, hoveredPoint, handleTolerance)
              : CreativeEditorWorldLayoutVerticalConnectorTarget{};
  const CreativeEditorWorldLayoutRoomTarget hoveredRoomTarget =
      hovered && state.tool == CreativeEditorWorldLayoutTool::Select
          ? findCreativeEditorWorldLayoutRoomTarget(state, hoveredPoint,
                                                    handleTolerance)
          : CreativeEditorWorldLayoutRoomTarget{};
  const CreativeEditorWorldLayoutBoxTarget hoveredBoxTarget =
      hovered && state.tool == CreativeEditorWorldLayoutTool::Select
          ? findCreativeEditorWorldLayoutBoxTarget(state, hoveredPoint,
                                                   handleTolerance)
          : CreativeEditorWorldLayoutBoxTarget{};
  const std::size_t hoveredObjectIndex =
      hovered && state.tool == CreativeEditorWorldLayoutTool::Select
          ? findCreativeEditorWorldLayoutObjectAt(state, hoveredPoint, grid)
          : cr::kInvalidCreativeWorldLayoutIndex;
  const bool hoveredBuilding =
      hovered && state.tool == CreativeEditorWorldLayoutTool::Select &&
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
  } else if (state.boxManipulation.active) {
    ImGui::SetMouseCursor(
        rectHandleCursor(state.boxManipulation.target.handle));
  } else if (hovered && state.tool == CreativeEditorWorldLayoutTool::Select) {
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
    } else if (hoveredBoxTarget.handle !=
               CreativeEditorWorldLayoutBoxHandle::None) {
      ImGui::SetMouseCursor(rectHandleCursor(hoveredBoxTarget.handle));
    }
  }

  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    if (state.tool == CreativeEditorWorldLayoutTool::Select) {
      if (hoveredOpeningTarget.handle !=
          CreativeEditorWorldLayoutOpeningHandle::None) {
        queueOpeningManipulation(
            commands,
            CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
            hoveredPoint, handleTolerance);
      } else if (hoveredWallTarget.handle !=
                 CreativeEditorWorldLayoutWallHandle::None) {
        queueWallManipulation(
            commands, CreativeEditorWorldLayoutWallManipulationPhase::Begin,
            hoveredPoint, handleTolerance);
      } else if (hoveredObjectIndex != cr::kInvalidCreativeWorldLayoutIndex) {
        static_cast<void>(beginCreativeEditorWorldLayoutObjectManipulation(
            state, hoveredObjectIndex, hoveredPoint));
      } else if (hoveredBuilding) {
        queueBuildingManipulation(
            commands,
            CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
            hoveredPoint, handleTolerance);
      } else if (hoveredVerticalConnectorTarget.directionHandle ||
                 hoveredVerticalConnectorTarget.handle !=
                     CreativeEditorWorldLayoutRectHandle::None) {
        queueVerticalConnectorManipulation(
            commands,
            CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin,
            hoveredPoint, handleTolerance);
      } else if (hoveredRoomTarget.handle !=
                 CreativeEditorWorldLayoutRoomHandle::None) {
        queueRoomManipulation(
            commands, CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
            hoveredPoint, handleTolerance);
      } else if (hoveredBoxTarget.handle !=
                 CreativeEditorWorldLayoutBoxHandle::None) {
        queueBoxManipulation(
            commands, CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
            hoveredPoint, handleTolerance);
      } else {
        queueRoomManipulation(
            commands, CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
            hoveredPoint, handleTolerance);
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

  const bool cancelGesture =
      !state.objectManipulation.active && !state.buildingManipulation.active &&
      !state.openingManipulation.active && !state.wallManipulation.active &&
      !state.verticalConnectorManipulation.active &&
      !state.roomManipulation.active && !state.boxManipulation.active &&
      state.anchorActive &&
      ((hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelGesture) {
    queueGesture(commands, CreativeEditorWorldLayoutGesturePhase::Cancel);
  } else if (dragTool(state.tool) &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
             ImGui::IsItemDeactivated()) {
    queueGesture(commands, CreativeEditorWorldLayoutGesturePhase::Commit,
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
    CreativeDesktopCommandFrame& commands, bool interactionEnabled,
    CreativeEditorWorldLayoutCanvasHoverStatus* hoverStatus) {
  drawLayoutCanvas(editor, document, commands, interactionEnabled,
                   hoverStatus);
}

}  // namespace iggy3d_creative_app
