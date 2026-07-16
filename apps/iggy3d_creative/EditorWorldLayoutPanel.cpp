#include "EditorWorldLayoutPanel.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

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

void drawWall(ImDrawList& drawList, const CanvasTransform& transform,
              const cr::CreativeWorldLayoutWall& wall, bool isSelected) {
  const ImVec2 start = toScreen(transform, wall.start.x, wall.start.z);
  const ImVec2 end = toScreen(transform, wall.end.x, wall.end.z);
  drawList.AddLine(start, end,
                   isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : color({0.72F, 0.76F, 0.81F, 1.0F}),
                   isSelected ? 7.0F : 5.0F);
}

ImVec2 openingPosition(const CanvasTransform& transform,
                       const cr::CreativeWorldLayoutWall& wall,
                       const cr::CreativeWorldLayoutOpening& opening) {
  const double dx = static_cast<double>(wall.end.x) - wall.start.x;
  const double dz = static_cast<double>(wall.end.z) - wall.start.z;
  const double length = std::hypot(dx, dz);
  const double t =
      length > 0.0 ? std::clamp(opening.centerOffsetCells / length, 0.0, 1.0)
                   : 0.0;
  return toScreen(transform, wall.start.x + t * dx, wall.start.z + t * dz);
}

void drawOpenings(ImDrawList& drawList, const CanvasTransform& transform,
                  const CreativeEditorWorldLayoutState& state) {
  for (std::size_t index = 0U; index < state.source.openings.size(); ++index) {
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[index];
    if (opening.wallIndex >= state.source.walls.size()) {
      continue;
    }
    const ImVec2 center = openingPosition(
        transform, state.source.walls[opening.wallIndex], opening);
    const bool isDoor = opening.kind == cr::CreativeBuildingOpeningKind::Door;
    const bool isSelected =
        selected(state, CreativeEditorWorldLayoutSelectionKind::Opening, index);
    const ImU32 markerColor = isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : isDoor   ? color({0.31F, 0.82F, 0.43F, 1.0F})
                                         : color({0.27F, 0.72F, 0.91F, 1.0F});
    if (isDoor) {
      drawList.AddCircleFilled(center, isSelected ? 7.0F : 5.0F, markerColor);
    } else {
      const float half = isSelected ? 7.0F : 5.0F;
      drawList.AddRectFilled({center.x - half, center.y - half},
                             {center.x + half, center.y + half}, markerColor);
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
  if (state.tool == CreativeEditorWorldLayoutTool::Floor) {
    const ImVec2 end = toScreen(transform, snappedX, snappedZ);
    drawList.AddRect({std::min(start.x, end.x), std::min(start.y, end.y)},
                     {std::max(start.x, end.x), std::max(start.y, end.y)},
                     previewColor, 0.0F, 0, 2.0F);
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

void drawLayoutCanvas(CreativeEditorState& editor,
                      CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  const ImVec2 available = ImGui::GetContentRegionAvail();
  const ImVec2 canvasSize{std::max(available.x, 160.0F),
                          std::max(available.y, 160.0F)};
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  const ImVec2 maximum{minimum.x + canvasSize.x, minimum.y + canvasSize.y};
  ImGui::InvisibleButton(
      "##world_layout_canvas", canvasSize,
      ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
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
  const CreativeEditorWorldLayoutPoint hoveredPoint =
      toWorld(transform, io.MousePos);
  if (hovered) {
    drawAnchorPreview(*drawList, transform, state, hoveredPoint);
  }
  drawList->PopClipRect();

  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    commands.push(CreativeDesktopCommandId::WorldLayoutCanvasPoint,
                  CreativeDesktopWorldLayoutPointPayload{hoveredPoint});
  }
}

}  // namespace

void buildCreativeEditorWorldLayoutPanel(
    CreativeEditorDesktopUiState& desktopUi, CreativeEditorState& editor,
    bool playModeActive, CreativeDesktopCommandFrame& commands) {
  if (!desktopUi.showWorldLayout) {
    return;
  }
  if (!ImGui::Begin("World Layout", &desktopUi.showWorldLayout,
                    ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  ImGui::BeginDisabled(playModeActive || editor.assetEdit.active);
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Select);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Floor);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Wall);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Door);
  ImGui::SameLine();
  toolButton(commands, state.tool, CreativeEditorWorldLayoutTool::Window);

  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();
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

  ImGui::TextDisabled(
      "floors %llu  walls %llu  openings %llu  rev %llu%s",
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
  drawLayoutCanvas(editor, commands);
  ImGui::EndDisabled();
  ImGui::End();
}

}  // namespace iggy3d_creative_app
