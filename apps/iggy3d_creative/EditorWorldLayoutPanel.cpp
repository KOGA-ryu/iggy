#include "EditorWorldLayoutPanel.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
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
  const cr::CreativeWorldLayoutRect footprint =
      active ? state.roomManipulation.previewFootprint
             : state.source.rooms[state.selection.index].footprint;
  const auto [minimum, maximum] = screenRect(transform, footprint);
  if (active) {
    const ImVec4 tint = state.roomManipulation.previewValid
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
      active ? (state.roomManipulation.previewValid
                    ? color({0.20F, 0.78F, 0.38F, 1.0F})
                    : color({0.92F, 0.29F, 0.24F, 1.0F}))
             : color({0.96F, 0.82F, 0.22F, 1.0F});
  for (const ImVec2 handle : handles) {
    drawList.AddRectFilled({handle.x - 4.0F, handle.y - 4.0F},
                           {handle.x + 4.0F, handle.y + 4.0F}, handleColor);
  }
}

ImGuiMouseCursor roomHandleCursor(
    CreativeEditorWorldLayoutRoomHandle handle) noexcept {
  switch (handle) {
    case CreativeEditorWorldLayoutRoomHandle::Move:
      return ImGuiMouseCursor_ResizeAll;
    case CreativeEditorWorldLayoutRoomHandle::North:
    case CreativeEditorWorldLayoutRoomHandle::South:
      return ImGuiMouseCursor_ResizeNS;
    case CreativeEditorWorldLayoutRoomHandle::East:
    case CreativeEditorWorldLayoutRoomHandle::West:
      return ImGuiMouseCursor_ResizeEW;
    case CreativeEditorWorldLayoutRoomHandle::NorthWest:
    case CreativeEditorWorldLayoutRoomHandle::SouthEast:
      return ImGuiMouseCursor_ResizeNWSE;
    case CreativeEditorWorldLayoutRoomHandle::NorthEast:
    case CreativeEditorWorldLayoutRoomHandle::SouthWest:
      return ImGuiMouseCursor_ResizeNESW;
    case CreativeEditorWorldLayoutRoomHandle::None:
    case CreativeEditorWorldLayoutRoomHandle::Count:
      break;
  }
  return ImGuiMouseCursor_Arrow;
}

void drawWall(ImDrawList& drawList, const CanvasTransform& transform,
              const cr::CreativeWorldLayoutWall& wall, bool isSelected) {
  const ImVec2 start = toScreen(transform, wall.start.x, wall.start.z);
  const ImVec2 end = toScreen(transform, wall.end.x, wall.end.z);
  drawList.AddLine(start, end,
                   isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : color({0.72F, 0.76F, 0.81F, 1.0F}),
                   isSelected ? 7.0F : 5.0F);
}

std::optional<ImVec2> openingPosition(
    const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayoutOpening& opening) {
  cr::CreativeTerrainCoord2 start{};
  cr::CreativeTerrainCoord2 end{};
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall) {
    if (opening.wallIndex >= state.source.walls.size()) {
      return std::nullopt;
    }
    start = state.source.walls[opening.wallIndex].start;
    end = state.source.walls[opening.wallIndex].end;
  } else {
    if (opening.roomIndex >= state.source.rooms.size()) {
      return std::nullopt;
    }
    const cr::CreativeWorldLayoutRect rect =
        state.source.rooms[opening.roomIndex].footprint;
    switch (opening.roomEdge) {
      case cr::CreativeWorldLayoutRoomEdge::North:
        start = {rect.minimum.x, rect.minimum.z};
        end = {rect.maximum.x, rect.minimum.z};
        break;
      case cr::CreativeWorldLayoutRoomEdge::East:
        start = {rect.maximum.x, rect.minimum.z};
        end = {rect.maximum.x, rect.maximum.z};
        break;
      case cr::CreativeWorldLayoutRoomEdge::South:
        start = {rect.minimum.x, rect.maximum.z};
        end = {rect.maximum.x, rect.maximum.z};
        break;
      case cr::CreativeWorldLayoutRoomEdge::West:
        start = {rect.minimum.x, rect.minimum.z};
        end = {rect.minimum.x, rect.maximum.z};
        break;
      case cr::CreativeWorldLayoutRoomEdge::Count:
        return std::nullopt;
    }
  }
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  const double length = std::hypot(dx, dz);
  const double t =
      length > 0.0 ? std::clamp(opening.centerOffsetCells / length, 0.0, 1.0)
                   : 0.0;
  return toScreen(transform, start.x + t * dx, start.z + t * dz);
}

void drawOpenings(ImDrawList& drawList, const CanvasTransform& transform,
                  const CreativeEditorWorldLayoutState& state) {
  for (std::size_t index = 0U; index < state.source.openings.size(); ++index) {
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[index];
    const std::optional<ImVec2> center =
        openingPosition(transform, state, opening);
    if (!center.has_value()) {
      continue;
    }
    const bool isDoor = opening.kind == cr::CreativeBuildingOpeningKind::Door;
    const bool isSelected =
        selected(state, CreativeEditorWorldLayoutSelectionKind::Opening, index);
    const ImU32 markerColor = isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : isDoor   ? color({0.31F, 0.82F, 0.43F, 1.0F})
                                         : color({0.27F, 0.72F, 0.91F, 1.0F});
    if (isDoor) {
      drawList.AddCircleFilled(*center, isSelected ? 7.0F : 5.0F,
                               markerColor);
    } else {
      const float half = isSelected ? 7.0F : 5.0F;
      drawList.AddRectFilled({center->x - half, center->y - half},
                             {center->x + half, center->y + half}, markerColor);
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
    drawWall(
        *drawList, transform, state.source.walls[index],
        selected(state, CreativeEditorWorldLayoutSelectionKind::Wall, index));
  }
  drawOpenings(*drawList, transform, state);
  drawRoomManipulation(*drawList, transform, state);
  const CreativeEditorWorldLayoutPoint pointerPoint =
      toWorld(transform, io.MousePos);
  const ImVec2 boundedPointer{
      std::clamp(io.MousePos.x, minimum.x, maximum.x),
      std::clamp(io.MousePos.y, minimum.y, maximum.y)};
  const CreativeEditorWorldLayoutPoint hoveredPoint =
      toWorld(transform, boundedPointer);
  const double roomHandleTolerance = std::clamp(
      8.0 / static_cast<double>(transform.pixelsPerCell), 0.10, 0.45);
  if (hovered) {
    drawAnchorPreview(*drawList, transform, state, hoveredPoint);
  }
  drawList->PopClipRect();

  if (!interactionEnabled) {
    return;
  }

  if (state.roomManipulation.active) {
    ImGui::SetMouseCursor(
        roomHandleCursor(state.roomManipulation.target.handle));
  } else if (hovered && state.tool == CreativeEditorWorldLayoutTool::Select) {
    const CreativeEditorWorldLayoutRoomTarget target =
        findCreativeEditorWorldLayoutRoomTarget(
            state, hoveredPoint, roomHandleTolerance);
    if (target.handle != CreativeEditorWorldLayoutRoomHandle::None) {
      ImGui::SetMouseCursor(roomHandleCursor(target.handle));
    }
  }

  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    if (state.tool == CreativeEditorWorldLayoutTool::Select) {
      queueRoomManipulation(
          commands, CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
          hoveredPoint, roomHandleTolerance);
    } else if (dragTool(state.tool)) {
      queueGesture(commands, CreativeEditorWorldLayoutGesturePhase::Begin,
                   hoveredPoint);
    } else {
      commands.push(CreativeDesktopCommandId::WorldLayoutCanvasPoint,
                    CreativeDesktopWorldLayoutPointPayload{hoveredPoint});
    }
  }

  const bool cancelRoomManipulation =
      state.roomManipulation.active &&
      (io.AppFocusLost ||
       (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
       ImGui::IsKeyPressed(ImGuiKey_Escape));
  if (cancelRoomManipulation) {
    queueRoomManipulation(
        commands, CreativeEditorWorldLayoutRoomManipulationPhase::Cancel,
        pointerPoint, roomHandleTolerance);
  } else if (state.roomManipulation.active &&
             ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    queueRoomManipulation(
        commands, CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
        pointerPoint, roomHandleTolerance);
  } else if (state.roomManipulation.active &&
             ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    queueRoomManipulation(
        commands, CreativeEditorWorldLayoutRoomManipulationPhase::Update,
        pointerPoint, roomHandleTolerance);
  }

  const bool cancelGesture =
      !state.roomManipulation.active && state.anchorActive &&
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
    if (state.roomManipulation.active) {
      queueRoomManipulation(
          commands, CreativeEditorWorldLayoutRoomManipulationPhase::Cancel, {},
          0.25);
    }
    return;
  }
  if (!ImGui::Begin("World Layout", &desktopUi.showWorldLayout,
                    ImGuiWindowFlags_NoCollapse)) {
    if (state.roomManipulation.active) {
      queueRoomManipulation(
          commands, CreativeEditorWorldLayoutRoomManipulationPhase::Cancel, {},
          0.25);
    }
    ImGui::End();
    return;
  }

  if (state.roomManipulation.active &&
      (playModeActive || editor.assetEdit.active)) {
    queueRoomManipulation(
        commands, CreativeEditorWorldLayoutRoomManipulationPhase::Cancel, {},
        0.25);
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
