#include "EditorWorldLayoutPanel.hpp"

#include "EditorDesktopWorldLayoutInspector.hpp"

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

struct CanvasTransform {
  ImVec2 origin;
  float pixelsPerCell = 28.0F;
};

ImVec2 toScreen(const CanvasTransform& transform, double x, double z) {
  return {transform.origin.x + static_cast<float>(x) * transform.pixelsPerCell,
          transform.origin.y + static_cast<float>(z) * transform.pixelsPerCell};
}

CreativeEditorWorldLayoutPoint toWorld(const CanvasTransform& transform,
                                       ImVec2 screen) {
  return {(screen.x - transform.origin.x) / transform.pixelsPerCell,
          (screen.y - transform.origin.y) / transform.pixelsPerCell};
}

ImU32 color(ImVec4 value) { return ImGui::ColorConvertFloat4ToU32(value); }

bool selected(const CreativeEditorWorldLayoutState& state,
              CreativeEditorWorldLayoutSelectionKind kind, std::size_t index) {
  return state.selection.kind == kind && state.selection.index == index;
}

void drawGrid(ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
              const CanvasTransform& transform) {
  const CreativeEditorWorldLayoutPoint worldMinimum =
      toWorld(transform, minimum);
  const CreativeEditorWorldLayoutPoint worldMaximum =
      toWorld(transform, maximum);
  const int firstX = static_cast<int>(std::floor(worldMinimum.x));
  const int lastX = static_cast<int>(std::ceil(worldMaximum.x));
  const int firstZ = static_cast<int>(std::floor(worldMinimum.z));
  const int lastZ = static_cast<int>(std::ceil(worldMaximum.z));
  for (int x = firstX; x <= lastX; ++x) {
    const float screenX = toScreen(transform, static_cast<double>(x), 0.0).x;
    const bool major = x % 5 == 0;
    drawList.AddLine({screenX, minimum.y}, {screenX, maximum.y},
                     major ? color({0.30F, 0.34F, 0.38F, 1.0F})
                           : color({0.20F, 0.23F, 0.26F, 1.0F}),
                     major ? 1.4F : 1.0F);
  }
  for (int z = firstZ; z <= lastZ; ++z) {
    const float screenZ = toScreen(transform, 0.0, static_cast<double>(z)).y;
    const bool major = z % 5 == 0;
    drawList.AddLine({minimum.x, screenZ}, {maximum.x, screenZ},
                     major ? color({0.30F, 0.34F, 0.38F, 1.0F})
                           : color({0.20F, 0.23F, 0.26F, 1.0F}),
                     major ? 1.4F : 1.0F);
  }
  const ImVec2 zero = toScreen(transform, 0.0, 0.0);
  drawList.AddLine({zero.x, minimum.y}, {zero.x, maximum.y},
                   color({0.40F, 0.63F, 0.86F, 1.0F}), 1.8F);
  drawList.AddLine({minimum.x, zero.y}, {maximum.x, zero.y},
                   color({0.86F, 0.42F, 0.36F, 1.0F}), 1.8F);
}

void drawFloor(ImDrawList& drawList, const CanvasTransform& transform,
               const cr::CreativeWorldLayoutBox& box, bool isSelected) {
  const ImVec2 minimum =
      toScreen(transform, box.footprint.minimum.x, box.footprint.minimum.z);
  const ImVec2 maximum =
      toScreen(transform, box.footprint.maximum.x, box.footprint.maximum.z);
  drawList.AddRectFilled(minimum, maximum, color({0.32F, 0.42F, 0.37F, 0.72F}));
  drawList.AddRect(minimum, maximum,
                   isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : color({0.53F, 0.70F, 0.59F, 1.0F}),
                   0.0F, 0, isSelected ? 3.0F : 1.5F);
}

void drawRoom(ImDrawList& drawList, const CanvasTransform& transform,
              const cr::CreativeWorldLayoutRoom& room, bool isSelected) {
  const ImVec2 minimum =
      toScreen(transform, room.footprint.minimum.x, room.footprint.minimum.z);
  const ImVec2 maximum =
      toScreen(transform, room.footprint.maximum.x, room.footprint.maximum.z);
  drawList.AddRectFilled(minimum, maximum,
                         color({0.22F, 0.34F, 0.42F, 0.38F}));
  drawList.AddRect(minimum, maximum,
                   isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : color({0.70F, 0.78F, 0.86F, 1.0F}),
                   0.0F, 0, isSelected ? 4.0F : 3.0F);
}

std::pair<ImVec2, ImVec2> screenRect(
    const CanvasTransform& transform, cr::CreativeWorldLayoutRect rect) {
  const ImVec2 first =
      toScreen(transform, rect.minimum.x, rect.minimum.z);
  const ImVec2 second =
      toScreen(transform, rect.maximum.x, rect.maximum.z);
  return {{std::min(first.x, second.x), std::min(first.y, second.y)},
          {std::max(first.x, second.x), std::max(first.y, second.y)}};
}

void drawRectManipulation(ImDrawList& drawList,
                          const CanvasTransform& transform,
                          cr::CreativeWorldLayoutRect footprint, bool active,
                          bool previewValid) {
  const auto [minimum, maximum] = screenRect(transform, footprint);
  if (active) {
    const ImVec4 tint = previewValid
                            ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
                            : ImVec4{0.92F, 0.29F, 0.24F, 1.0F};
    ImVec4 fill = tint;
    fill.w = 0.24F;
    drawList.AddRectFilled(minimum, maximum, color(fill));
    drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0, 3.0F);
    const std::int64_t width =
        static_cast<std::int64_t>(footprint.maximum.x) - footprint.minimum.x;
    const std::int64_t depth =
        static_cast<std::int64_t>(footprint.maximum.z) - footprint.minimum.z;
    const std::string dimensions =
        std::to_string(width) + " x " + std::to_string(depth);
    drawList.AddText({minimum.x + 7.0F, minimum.y + 7.0F}, color(tint),
                     dimensions.c_str());
  }

  const float centerX = (minimum.x + maximum.x) * 0.5F;
  const float centerY = (minimum.y + maximum.y) * 0.5F;
  const std::array<ImVec2, 8U> handles = {
      ImVec2{minimum.x, minimum.y}, ImVec2{centerX, minimum.y},
      ImVec2{maximum.x, minimum.y}, ImVec2{maximum.x, centerY},
      ImVec2{maximum.x, maximum.y}, ImVec2{centerX, maximum.y},
      ImVec2{minimum.x, maximum.y}, ImVec2{minimum.x, centerY},
  };
  const ImU32 handleColor =
      active ? (previewValid
                    ? color({0.20F, 0.78F, 0.38F, 1.0F})
                    : color({0.92F, 0.29F, 0.24F, 1.0F}))
             : color({0.96F, 0.82F, 0.22F, 1.0F});
  for (const ImVec2 handle : handles) {
    drawList.AddRectFilled({handle.x - 4.0F, handle.y - 4.0F},
                           {handle.x + 4.0F, handle.y + 4.0F}, handleColor);
  }
}

void drawRoomManipulation(ImDrawList& drawList,
                          const CanvasTransform& transform,
                          const CreativeEditorWorldLayoutState& state) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Room ||
      state.selection.index >= state.source.rooms.size()) {
    return;
  }
  const bool active = state.roomManipulation.active &&
                      state.roomManipulation.target.roomIndex ==
                          state.selection.index;
  drawRectManipulation(
      drawList, transform,
      active ? state.roomManipulation.previewFootprint
             : state.source.rooms[state.selection.index].footprint,
      active, state.roomManipulation.previewValid);
}

void drawBoxManipulation(ImDrawList& drawList,
                         const CanvasTransform& transform,
                         const CreativeEditorWorldLayoutState& state) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Box ||
      state.selection.index >= state.source.boxes.size()) {
    return;
  }
  const bool active = state.boxManipulation.active &&
                      state.boxManipulation.target.boxIndex ==
                          state.selection.index;
  drawRectManipulation(
      drawList, transform,
      active ? state.boxManipulation.previewFootprint
             : state.source.boxes[state.selection.index].footprint,
      active, state.boxManipulation.previewValid);
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

void drawWall(ImDrawList& drawList, const CanvasTransform& transform,
              const CreativeEditorWorldLayoutState& state,
              std::size_t wallIndex) {
  const cr::CreativeWorldLayoutWall& wall = state.source.walls[wallIndex];
  const bool isSelected =
      selected(state, CreativeEditorWorldLayoutSelectionKind::Wall, wallIndex);
  const bool active = state.wallManipulation.active &&
                      state.wallManipulation.target.wallIndex == wallIndex;
  const cr::CreativeTerrainCoord2 startPoint =
      active ? state.wallManipulation.previewStart : wall.start;
  const cr::CreativeTerrainCoord2 endPoint =
      active ? state.wallManipulation.previewEnd : wall.end;
  const ImVec2 start = toScreen(transform, startPoint.x, startPoint.z);
  const ImVec2 end = toScreen(transform, endPoint.x, endPoint.z);
  const ImU32 wallColor =
      active ? (state.wallManipulation.previewValid
                    ? color({0.20F, 0.78F, 0.38F, 1.0F})
                    : color({0.92F, 0.29F, 0.24F, 1.0F}))
      : isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                   : color({0.72F, 0.76F, 0.81F, 1.0F});
  drawList.AddLine(start, end, wallColor,
                   active || isSelected ? 7.0F : 5.0F);
  if (active || isSelected) {
    drawList.AddRectFilled({start.x - 4.0F, start.y - 4.0F},
                           {start.x + 4.0F, start.y + 4.0F}, wallColor);
    drawList.AddRectFilled({end.x - 4.0F, end.y - 4.0F},
                           {end.x + 4.0F, end.y + 4.0F}, wallColor);
  }
  if (active) {
    char dimensions[48]{};
    const double length =
        std::hypot(static_cast<double>(endPoint.x) - startPoint.x,
                   static_cast<double>(endPoint.z) - startPoint.z);
    std::snprintf(dimensions, sizeof(dimensions), "%.0f long", length);
    drawList.AddText({(start.x + end.x) * 0.5F + 7.0F,
                      (start.y + end.y) * 0.5F + 7.0F},
                     wallColor, dimensions);
  }
}

CreativeEditorWorldLayoutPoint openingPoint(
    CreativeEditorWorldLayoutOpeningHost host, double offsetCells) {
  const double t = offsetCells / host.lengthCells;
  return {host.start.x + (host.end.x - host.start.x) * t,
          host.start.z + (host.end.z - host.start.z) * t};
}

void drawOpenings(ImDrawList& drawList, const CanvasTransform& transform,
                  const CreativeEditorWorldLayoutState& state) {
  for (std::size_t index = 0U; index < state.source.openings.size(); ++index) {
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[index];
    const CreativeEditorWorldLayoutOpeningHost host =
        resolveCreativeEditorWorldLayoutOpeningHost(state, index);
    if (!host.valid) {
      continue;
    }
    const bool isDoor = opening.kind == cr::CreativeBuildingOpeningKind::Door;
    const bool isSelected =
        selected(state, CreativeEditorWorldLayoutSelectionKind::Opening, index);
    const bool active = state.openingManipulation.active &&
                        state.openingManipulation.target.openingIndex == index;
    const bool hostWallActive =
        state.wallManipulation.active &&
        opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
        opening.wallIndex == state.wallManipulation.target.wallIndex;
    const double centerOffset = active
                                    ? state.openingManipulation
                                          .previewCenterOffsetCells
                                    : hostWallActive
                                          ? opening.centerOffsetCells +
                                                state.wallManipulation
                                                    .previewOpeningOffsetDeltaCells
                                          : opening.centerOffsetCells;
    const double width = active ? state.openingManipulation.previewWidthCells
                                : opening.widthCells;
    const CreativeEditorWorldLayoutPoint centerPoint =
        openingPoint(host, centerOffset);
    const CreativeEditorWorldLayoutPoint startPoint =
        openingPoint(host, centerOffset - width * 0.5);
    const CreativeEditorWorldLayoutPoint endPoint =
        openingPoint(host, centerOffset + width * 0.5);
    const ImVec2 center =
        toScreen(transform, centerPoint.x, centerPoint.z);
    const ImVec2 start = toScreen(transform, startPoint.x, startPoint.z);
    const ImVec2 end = toScreen(transform, endPoint.x, endPoint.z);
    const bool previewing = active || hostWallActive;
    const bool previewValid =
        active ? state.openingManipulation.previewValid
               : state.wallManipulation.previewValid;
    const ImU32 markerColor =
        previewing ? (previewValid
                          ? color({0.20F, 0.78F, 0.38F, 1.0F})
                          : color({0.92F, 0.29F, 0.24F, 1.0F}))
        : isSelected  ? color({0.96F, 0.82F, 0.22F, 1.0F})
        : isDoor      ? color({0.31F, 0.82F, 0.43F, 1.0F})
                      : color({0.27F, 0.72F, 0.91F, 1.0F});
    drawList.AddLine(start, end, markerColor,
                     previewing || isSelected ? 7.0F : 5.0F);
    if (isDoor) {
      drawList.AddCircleFilled(center, previewing || isSelected ? 7.0F : 5.0F,
                               markerColor);
    } else {
      const float half = previewing || isSelected ? 7.0F : 5.0F;
      drawList.AddRectFilled({center.x - half, center.y - half},
                             {center.x + half, center.y + half}, markerColor);
    }
    if (active || isSelected) {
      drawList.AddRectFilled({start.x - 4.0F, start.y - 4.0F},
                             {start.x + 4.0F, start.y + 4.0F}, markerColor);
      drawList.AddRectFilled({end.x - 4.0F, end.y - 4.0F},
                             {end.x + 4.0F, end.y + 4.0F}, markerColor);
    }
    if (active) {
      char dimensions[48]{};
      std::snprintf(dimensions, sizeof(dimensions), "%.2f wide", width);
      drawList.AddText({center.x + 8.0F, center.y + 8.0F}, markerColor,
                       dimensions);
    }
  }
}

void drawAnchorPreview(ImDrawList& drawList, const CanvasTransform& transform,
                       const CreativeEditorWorldLayoutState& state,
                       CreativeEditorWorldLayoutPoint hovered) {
  if (!state.anchorActive) {
    return;
  }
  const double snappedX = std::round(hovered.x);
  const double snappedZ = std::round(hovered.z);
  const ImU32 previewColor = color({0.96F, 0.82F, 0.22F, 0.95F});
  const ImVec2 start = toScreen(transform, state.anchor.x, state.anchor.z);
  if (state.tool == CreativeEditorWorldLayoutTool::Room ||
      state.tool == CreativeEditorWorldLayoutTool::Floor) {
    const ImVec2 end = toScreen(transform, snappedX, snappedZ);
    if (state.tool == CreativeEditorWorldLayoutTool::Room) {
      drawList.AddRectFilled(
          {std::min(start.x, end.x), std::min(start.y, end.y)},
          {std::max(start.x, end.x), std::max(start.y, end.y)},
          color({0.22F, 0.58F, 0.38F, 0.22F}));
    }
    drawList.AddRect({std::min(start.x, end.x), std::min(start.y, end.y)},
                     {std::max(start.x, end.x), std::max(start.y, end.y)},
                     previewColor, 0.0F, 0, 2.0F);
    const int width = static_cast<int>(std::fabs(snappedX - state.anchor.x));
    const int depth = static_cast<int>(std::fabs(snappedZ - state.anchor.z));
    const std::string dimensions =
        std::to_string(width) + " x " + std::to_string(depth);
    drawList.AddText({std::min(start.x, end.x) + 6.0F,
                      std::min(start.y, end.y) + 6.0F},
                     previewColor, dimensions.c_str());
  } else if (state.tool == CreativeEditorWorldLayoutTool::Wall) {
    const double deltaX = std::fabs(snappedX - state.anchor.x);
    const double deltaZ = std::fabs(snappedZ - state.anchor.z);
    const ImVec2 end = deltaX >= deltaZ
                           ? toScreen(transform, snappedX, state.anchor.z)
                           : toScreen(transform, state.anchor.x, snappedZ);
    drawList.AddLine(start, end, previewColor, 4.0F);
  }
}

void queueTool(CreativeDesktopCommandFrame& commands,
               CreativeEditorWorldLayoutTool tool) {
  commands.push(CreativeDesktopCommandId::WorldLayoutSetTool,
                CreativeDesktopWorldLayoutToolPayload{tool});
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

void queueLayoutManipulationCancel(
    const CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  if (state.openingManipulation.active) {
    queueOpeningManipulation(
        commands, CreativeEditorWorldLayoutOpeningManipulationPhase::Cancel, {},
        0.25);
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

bool dragTool(CreativeEditorWorldLayoutTool tool) noexcept {
  return tool == CreativeEditorWorldLayoutTool::Room ||
         tool == CreativeEditorWorldLayoutTool::Floor ||
         tool == CreativeEditorWorldLayoutTool::Wall;
}

void toolButton(CreativeDesktopCommandFrame& commands,
                CreativeEditorWorldLayoutTool current,
                CreativeEditorWorldLayoutTool tool) {
  const bool active = current == tool;
  if (active) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.16F, 0.47F, 0.25F, 1.0F});
  }
  if (ImGui::Button(creativeEditorWorldLayoutToolLabel(tool))) {
    queueTool(commands, tool);
  }
  if (active) {
    ImGui::PopStyleColor();
  }
}

void drawSelectedRoomSettings(CreativeEditorWorldLayoutState& state,
                              CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Room ||
      state.selection.index >= state.source.rooms.size()) {
    return;
  }

  const cr::CreativeWorldLayoutRoom& room =
      state.source.rooms[state.selection.index];
  const std::int64_t widthCells =
      static_cast<std::int64_t>(room.footprint.maximum.x) -
      room.footprint.minimum.x;
  const std::int64_t depthCells =
      static_cast<std::int64_t>(room.footprint.maximum.z) -
      room.footprint.minimum.z;
  int width = static_cast<int>(std::min<std::int64_t>(
      widthCells, std::numeric_limits<int>::max()));
  int depth = static_cast<int>(std::min<std::int64_t>(
      depthCells, std::numeric_limits<int>::max()));
  int baseLayer = room.baseLayer;
  int wallHeight = room.wallHeightCells;
  double wallThickness = room.wallThicknessCells;
  int floorLayers = room.floorThicknessCells;

  ImGui::SetNextItemWidth(88.0F);
  bool changed = ImGui::InputInt("Width##room_shell", &width, 1, 4);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(88.0F);
  changed = ImGui::InputInt("Depth##room_shell", &depth, 1, 4) || changed;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(88.0F);
  changed = ImGui::InputInt("Base##room_shell", &baseLayer, 1, 4) || changed;

  ImGui::SetNextItemWidth(88.0F);
  changed =
      ImGui::InputInt("Wall height##room_shell", &wallHeight, 1, 4) || changed;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(88.0F);
  changed = ImGui::InputDouble("Wall thickness##room_shell", &wallThickness,
                               0.05, 0.25, "%.3f") ||
            changed;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(88.0F);
  changed =
      ImGui::InputInt("Floor layers##room_shell", &floorLayers, 1, 2) ||
      changed;

  if (!changed) {
    return;
  }

  const std::int64_t maximumX =
      static_cast<std::int64_t>(room.footprint.minimum.x) + width;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(room.footprint.minimum.z) + depth;
  const int maximumLayerCount = std::numeric_limits<std::uint16_t>::max();
  const bool valid =
      width > 0 && depth > 0 &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() &&
      wallHeight > 0 && wallHeight <= maximumLayerCount &&
      floorLayers > 0 && floorLayers <= maximumLayerCount &&
      std::isfinite(wallThickness) && wallThickness > 0.0 &&
      static_cast<double>(width) > wallThickness * 2.0 &&
      static_cast<double>(depth) > wallThickness * 2.0;
  if (!valid) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "room shell settings are outside valid bounds");
    return;
  }

  CreativeEditorWorldLayoutRoomSettings settings;
  settings.footprint = room.footprint;
  settings.footprint.maximum.x = static_cast<std::int32_t>(maximumX);
  settings.footprint.maximum.z = static_cast<std::int32_t>(maximumZ);
  settings.baseLayer = static_cast<std::int32_t>(baseLayer);
  settings.wallHeightCells = static_cast<std::uint16_t>(wallHeight);
  settings.wallThicknessCells = wallThickness;
  settings.floorThicknessCells = static_cast<std::uint16_t>(floorLayers);
  commands.push(
      CreativeDesktopCommandId::WorldLayoutSetRoomSettings,
      CreativeDesktopWorldLayoutRoomSettingsPayload{state.selection.index,
                                                    settings});
}

bool sameOpeningSettings(
    const CreativeEditorWorldLayoutOpeningSettings& lhs,
    const CreativeEditorWorldLayoutOpeningSettings& rhs) noexcept {
  return lhs.centerOffsetCells == rhs.centerOffsetCells &&
         lhs.widthCells == rhs.widthCells &&
         lhs.sillHeightCells == rhs.sillHeightCells &&
         lhs.heightCells == rhs.heightCells && lhs.pose == rhs.pose &&
         lhs.includeInsert == rhs.includeInsert;
}

void drawSelectedOpeningSettings(CreativeEditorWorldLayoutState& state,
                                 CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::Opening ||
      state.selection.index >= state.source.openings.size()) {
    state.openingSettingsDraft = {};
    return;
  }
  const std::size_t openingIndex = state.selection.index;
  CreativeEditorWorldLayoutOpeningSettings current;
  if (!readCreativeEditorWorldLayoutOpeningSettings(state, openingIndex,
                                                    current)) {
    state.openingSettingsDraft = {};
    return;
  }
  if (!state.openingSettingsDraft.active ||
      state.openingSettingsDraft.openingIndex != openingIndex ||
      state.openingSettingsDraft.sourceRevision != state.revision) {
    state.openingSettingsDraft = {true, openingIndex, state.revision, current};
  }

  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  CreativeEditorWorldLayoutOpeningSettings& settings =
      state.openingSettingsDraft.settings;
  const bool isDoor = opening.kind == cr::CreativeBuildingOpeningKind::Door;
  ImGui::TextUnformatted(isDoor ? "Door settings" : "Window settings");
  ImGui::SetNextItemWidth(105.0F);
  ImGui::InputDouble("Offset##opening", &settings.centerOffsetCells, 0.25, 1.0,
                     "%.2f");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(105.0F);
  ImGui::InputDouble("Width##opening", &settings.widthCells, 0.25, 1.0,
                     "%.2f");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(105.0F);
  ImGui::InputDouble("Height##opening", &settings.heightCells, 0.25, 1.0,
                     "%.2f");

  if (isDoor) {
    settings.sillHeightCells = 0.0;
    constexpr std::array<const char*, 5U> kPoseLabels = {
        "Closed", "Start hinge / side A", "Start hinge / side B",
        "End hinge / side A", "End hinge / side B"};
    int pose = static_cast<int>(settings.pose);
    ImGui::SetNextItemWidth(190.0F);
    if (ImGui::Combo("Pose##opening", &pose, kPoseLabels.data(),
                     static_cast<int>(kPoseLabels.size()))) {
      settings.pose = static_cast<cr::CreativeBuildingOpeningPose>(pose);
    }
  } else {
    settings.pose = cr::CreativeBuildingOpeningPose::Closed;
    ImGui::SetNextItemWidth(105.0F);
    ImGui::InputDouble("Sill##opening", &settings.sillHeightCells, 0.25, 1.0,
                       "%.2f");
  }
  ImGui::SameLine();
  ImGui::Checkbox("Insert##opening", &settings.includeInsert);

  const bool dirty = !sameOpeningSettings(current, settings);
  ImGui::BeginDisabled(!dirty || state.openingManipulation.active);
  if (ImGui::Button("Apply opening")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetOpeningSettings,
        CreativeDesktopWorldLayoutOpeningSettingsPayload{openingIndex,
                                                         settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Reset opening")) {
    state.openingSettingsDraft.settings = current;
  }
  ImGui::EndDisabled();
}

void drawLayoutCanvas(CreativeEditorState& editor,
                      CreativeDesktopCommandFrame& commands,
                      bool interactionEnabled) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
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

  ImDrawList* drawList = ImGui::GetWindowDrawList();
  drawList->PushClipRect(minimum, maximum, true);
  drawList->AddRectFilled(minimum, maximum,
                          color({0.105F, 0.12F, 0.135F, 1.0F}));
  drawGrid(*drawList, minimum, maximum, transform);
  for (std::size_t index = 0U; index < state.source.rooms.size(); ++index) {
    drawRoom(
        *drawList, transform, state.source.rooms[index],
        selected(state, CreativeEditorWorldLayoutSelectionKind::Room, index));
  }
  for (std::size_t index = 0U; index < state.source.boxes.size(); ++index) {
    drawFloor(
        *drawList, transform, state.source.boxes[index],
        selected(state, CreativeEditorWorldLayoutSelectionKind::Box, index));
  }
  for (std::size_t index = 0U; index < state.source.walls.size(); ++index) {
    drawWall(*drawList, transform, state, index);
  }
  drawOpenings(*drawList, transform, state);
  drawRoomManipulation(*drawList, transform, state);
  drawBoxManipulation(*drawList, transform, state);
  const CreativeEditorWorldLayoutPoint pointerPoint =
      toWorld(transform, io.MousePos);
  const ImVec2 boundedPointer{
      std::clamp(io.MousePos.x, minimum.x, maximum.x),
      std::clamp(io.MousePos.y, minimum.y, maximum.y)};
  const CreativeEditorWorldLayoutPoint hoveredPoint =
      toWorld(transform, boundedPointer);
  const double handleTolerance = std::clamp(
      8.0 / static_cast<double>(transform.pixelsPerCell), 0.10, 0.45);
  if (hovered) {
    drawAnchorPreview(*drawList, transform, state, hoveredPoint);
  }
  drawList->PopClipRect();

  if (!interactionEnabled) {
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
  if (state.openingManipulation.active) {
    ImGui::SetMouseCursor(openingHandleCursor(
        state, state.openingManipulation.target));
  } else if (state.wallManipulation.active) {
    ImGui::SetMouseCursor(
        wallHandleCursor(state, state.wallManipulation.target));
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
      !state.openingManipulation.active && !state.wallManipulation.active &&
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

void buildCreativeEditorWorldLayoutPanel(
    CreativeEditorDesktopUiState& desktopUi, CreativeEditorState& editor,
    bool playModeActive, CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  if (!desktopUi.showWorldLayout) {
    queueLayoutManipulationCancel(state, commands);
    return;
  }
  if (!ImGui::Begin("World Layout", &desktopUi.showWorldLayout,
                    ImGuiWindowFlags_NoCollapse)) {
    queueLayoutManipulationCancel(state, commands);
    ImGui::End();
    return;
  }

  if (playModeActive || editor.assetEdit.active) {
    queueLayoutManipulationCancel(state, commands);
  }
  ImGui::BeginDisabled(playModeActive || editor.assetEdit.active);
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Select);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Room);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Floor);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Wall);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Door);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Window);

  if (ImGui::Button("Preview 3D")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutPreview);
  }
  ImGui::SameLine();
  if (ImGui::Button("Generate")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutConfirm);
  }
  ImGui::SameLine();
  const bool hasSelection =
      state.selection.kind != CreativeEditorWorldLayoutSelectionKind::None;
  ImGui::BeginDisabled(!hasSelection);
  if (ImGui::Button("Delete")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutDeleteSelection);
  }
  ImGui::EndDisabled();
  ImGui::EndDisabled();

  ImGui::BeginDisabled(playModeActive || editor.assetEdit.active);
  drawSelectedRoomSettings(state, commands);
  drawCreativeEditorWorldLayoutStructureInspector(state, commands);
  drawSelectedOpeningSettings(state, commands);
  ImGui::EndDisabled();

  ImGui::TextDisabled(
      "rooms %llu  floors %llu  partitions %llu  openings %llu  rev %llu%s",
      static_cast<unsigned long long>(state.source.rooms.size()),
      static_cast<unsigned long long>(state.source.boxes.size()),
      static_cast<unsigned long long>(state.source.walls.size()),
      static_cast<unsigned long long>(state.source.openings.size()),
      static_cast<unsigned long long>(state.revision),
      creativeEditorWorldLayoutDirty(state) ? " *" : "");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4{0.32F, 0.95F, 0.43F, 1.0F}, "%s",
                     state.statusMessage.c_str());
  ImGui::Separator();

  ImGui::BeginDisabled(playModeActive || editor.assetEdit.active);
  drawLayoutCanvas(editor, commands,
                   !playModeActive && !editor.assetEdit.active);
  ImGui::EndDisabled();
  ImGui::End();
}

}  // namespace iggy3d_creative_app
