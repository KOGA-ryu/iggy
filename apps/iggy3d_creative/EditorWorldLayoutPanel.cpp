#include "EditorWorldLayoutPanel.hpp"

#include "EditorDesktopWorldLayoutInspector.hpp"
#include "EditorWorldLayoutElevationPanel.hpp"
#include "EditorWorldLayoutHierarchyPanel.hpp"

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

struct CanvasTransform {
  ImVec2 origin;
  float pixelsPerCell = 28.0F;
};

int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto* value = static_cast<std::string*>(data->UserData);
    value->resize(static_cast<std::size_t>(data->BufTextLen));
    data->Buf = value->data();
  }
  return 0;
}

bool inputText(const char* label, std::string& value) {
  return ImGui::InputText(label, value.data(), value.capacity() + 1U,
                          ImGuiInputTextFlags_CallbackResize,
                          inputTextResizeCallback, &value);
}

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

bool roomOnActiveLevel(const CreativeEditorWorldLayoutState& state,
                       const cr::CreativeWorldLayout& source,
                       std::size_t roomIndex) noexcept {
  return roomIndex < source.rooms.size() &&
         (state.activeLevelIndex >= source.levels.size() ||
          source.rooms[roomIndex].levelIndex == state.activeLevelIndex);
}

bool openingOnActiveLevel(const CreativeEditorWorldLayoutState& state,
                          const cr::CreativeWorldLayout& source,
                          std::size_t openingIndex) noexcept {
  if (openingIndex >= source.openings.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutOpening& opening = source.openings[openingIndex];
  return opening.hostKind !=
             cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
         roomOnActiveLevel(state, source, opening.roomIndex);
}

std::pair<double, double> buildingPreviewOffset(
    const CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex) noexcept {
  if (!state.buildingManipulation.active ||
      state.buildingManipulation.buildingIndex != buildingIndex) {
    return {0.0, 0.0};
  }
  return {
      static_cast<double>(state.buildingManipulation.previewDeltaXCells),
      static_cast<double>(state.buildingManipulation.previewDeltaZCells),
  };
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

void drawTerrainSymbols(ImDrawList& drawList,
                        const CanvasTransform& transform,
                        const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  for (std::size_t index = 0U; index < source.terrainProfiles.size(); ++index) {
    const cr::CreativeWorldLayoutTerrainProfile& profile =
        source.terrainProfiles[index];
    const ImVec2 center =
        toScreen(transform, profile.center.x, profile.center.z);
    const float radius = std::max(
        5.0F, static_cast<float>(profile.radiusCells) * transform.pixelsPerCell);
    const bool isSelected = selected(
        state, CreativeEditorWorldLayoutSelectionKind::TerrainProfile, index);
    drawList.AddCircleFilled(center, radius,
                             color({0.25F, 0.46F, 0.28F, 0.12F}), 48);
    drawList.AddCircle(center, radius,
                       isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                                  : color({0.38F, 0.68F, 0.42F, 0.78F}),
                       48, isSelected ? 3.0F : 1.5F);
    drawList.AddCircleFilled(center, isSelected ? 6.0F : 4.0F,
                             isSelected
                                 ? color({0.96F, 0.82F, 0.22F, 1.0F})
                                 : color({0.38F, 0.68F, 0.42F, 1.0F}));
  }
  for (std::size_t index = 0U; index < source.terrainPaths.size(); ++index) {
    const cr::CreativeWorldLayoutTerrainPath& path = source.terrainPaths[index];
    if (path.pointCount < 2U ||
        path.firstPointIndex > source.terrainPathPoints.size() ||
        path.pointCount >
            source.terrainPathPoints.size() - path.firstPointIndex) {
      continue;
    }
    const bool isSelected = selected(
        state, CreativeEditorWorldLayoutSelectionKind::TerrainPath, index);
    const ImVec4 tint = path.kind == cr::CreativeTerrainRecipeKind::Ditch
                            ? ImVec4{0.25F, 0.47F, 0.68F, 1.0F}
                            : ImVec4{0.72F, 0.56F, 0.28F, 1.0F};
    ImVec4 fill = tint;
    fill.w = 0.24F;
    const float width = std::max(
        3.0F, static_cast<float>(path.halfWidthCells * 2U + 1U) *
                  transform.pixelsPerCell);
    for (std::size_t pointIndex = path.firstPointIndex + 1U;
         pointIndex < path.firstPointIndex + path.pointCount; ++pointIndex) {
      const cr::CreativeTerrainCoord2 startCoord =
          source.terrainPathPoints[pointIndex - 1U].coord;
      const cr::CreativeTerrainCoord2 endCoord =
          source.terrainPathPoints[pointIndex].coord;
      const ImVec2 start = toScreen(transform, startCoord.x, startCoord.z);
      const ImVec2 end = toScreen(transform, endCoord.x, endCoord.z);
      drawList.AddLine(start, end, color(fill), width);
      drawList.AddLine(start, end,
                       isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                                  : color(tint),
                       isSelected ? 4.0F : 2.0F);
    }
  }
}

void drawObjectSymbols(ImDrawList& drawList,
                       const CanvasTransform& transform,
                       const CreativeEditorWorldLayoutState& state,
                       cr::CreativeGridSettings grid) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  for (std::size_t index = 0U; index < source.objects.size(); ++index) {
    const cr::CreativeWorldLayoutObject& object = source.objects[index];
    const bool isSelected = selected(
        state, CreativeEditorWorldLayoutSelectionKind::Object, index);
    const bool previewing = state.objectManipulation.active &&
                            state.objectManipulation.objectIndex == index &&
                            state.objectManipulation.sourceRevision ==
                                state.revision;
    const CreativeEditorWorldLayoutObjectSettings* preview =
        previewing ? &state.objectManipulation.previewSettings : nullptr;
    const ImU32 outline =
        previewing ? (state.objectManipulation.previewValid
                          ? color({0.20F, 0.78F, 0.38F, 1.0F})
                          : color({0.92F, 0.29F, 0.24F, 1.0F}))
        : isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
        : object.kind == cr::CreativeObjectKind::Rock
            ? color({0.66F, 0.69F, 0.72F, 1.0F})
        : object.kind == cr::CreativeObjectKind::SpawnPoint
            ? color({0.24F, 0.86F, 0.43F, 1.0F})
        : object.kind == cr::CreativeObjectKind::NpcSpawn
            ? color({0.72F, 0.42F, 0.88F, 1.0F})
            : color({0.90F, 0.58F, 0.25F, 1.0F});
    cr::CreativeWorldLayoutObject displayed = object;
    if (preview != nullptr) {
      displayed.mode = preview->mode;
      displayed.boundsCells = preview->boundsCells;
      displayed.pointCells = preview->pointCells;
      displayed.assetSourceBoundsMeters = preview->assetSourceBoundsMeters;
      displayed.hasAssetSourceBounds = preview->hasAssetSourceBounds;
      displayed.yawRadians = preview->yawRadians;
      displayed.scale = preview->scale;
    }
    ImVec2 labelAnchor;
    const CreativeEditorWorldLayoutObjectFootprint footprint =
        planCreativeEditorWorldLayoutObjectFootprint(displayed, grid);
    if (footprint.valid) {
      std::array<ImVec2, 4U> points;
      for (std::size_t pointIndex = 0U; pointIndex < points.size(); ++pointIndex) {
        points[pointIndex] = toScreen(transform, footprint.corners[pointIndex].x,
                                     footprint.corners[pointIndex].z);
      }
      drawList.AddConvexPolyFilled(
          points.data(), static_cast<int>(points.size()),
          previewing
              ? (state.objectManipulation.previewValid
                     ? color({0.20F, 0.78F, 0.38F, 0.22F})
                     : color({0.92F, 0.29F, 0.24F, 0.22F}))
              : color({0.54F, 0.48F, 0.40F, 0.28F}));
      drawList.AddPolyline(points.data(), static_cast<int>(points.size()),
                           outline, ImDrawFlags_Closed,
                           previewing || isSelected ? 3.0F : 1.8F);
      labelAnchor = points.front();
    } else {
      const ImVec2 center =
          toScreen(transform, displayed.pointCells.x, displayed.pointCells.z);
      drawList.AddCircleFilled(center, previewing || isSelected ? 7.0F : 5.0F,
                               outline);
      drawList.AddCircle(center, previewing || isSelected ? 11.0F : 8.0F,
                         outline, 16,
                         previewing || isSelected ? 3.0F : 1.5F);
      labelAnchor = center;
    }
    if (previewing || isSelected) {
      drawList.AddText({labelAnchor.x + 7.0F, labelAnchor.y + 7.0F}, outline,
                       object.name.c_str());
    }
  }
}

void drawFloor(ImDrawList& drawList, const CanvasTransform& transform,
               const CreativeEditorWorldLayoutState& state,
               std::size_t boxIndex) {
  const cr::CreativeWorldLayoutBox& box =
      creativeEditorWorldLayoutDisplaySource(state).boxes[boxIndex];
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, box.buildingIndex);
  const ImVec2 minimum =
      toScreen(transform, box.footprint.minimum.x + deltaX,
               box.footprint.minimum.z + deltaZ);
  const ImVec2 maximum =
      toScreen(transform, box.footprint.maximum.x + deltaX,
               box.footprint.maximum.z + deltaZ);
  const bool isSelected = selected(
      state, CreativeEditorWorldLayoutSelectionKind::Box, boxIndex);
  drawList.AddRectFilled(minimum, maximum, color({0.32F, 0.42F, 0.37F, 0.72F}));
  drawList.AddRect(minimum, maximum,
                   isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : color({0.53F, 0.70F, 0.59F, 1.0F}),
                   0.0F, 0, isSelected ? 3.0F : 1.5F);
}

void drawRoom(ImDrawList& drawList, const CanvasTransform& transform,
              const CreativeEditorWorldLayoutState& state,
              std::size_t roomIndex) {
  const cr::CreativeWorldLayoutRoom& room =
      creativeEditorWorldLayoutDisplaySource(state).rooms[roomIndex];
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, room.buildingIndex);
  const ImVec2 minimum =
      toScreen(transform, room.footprint.minimum.x + deltaX,
               room.footprint.minimum.z + deltaZ);
  const ImVec2 maximum =
      toScreen(transform, room.footprint.maximum.x + deltaX,
               room.footprint.maximum.z + deltaZ);
  const bool isSelected = selected(
      state, CreativeEditorWorldLayoutSelectionKind::Room, roomIndex);
  drawList.AddRectFilled(minimum, maximum,
                         color({0.22F, 0.34F, 0.42F, 0.38F}));
  drawList.AddRect(minimum, maximum,
                   isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : color({0.70F, 0.78F, 0.86F, 1.0F}),
                   0.0F, 0, isSelected ? 4.0F : 3.0F);
}

void drawActiveLevelRoof(ImDrawList& drawList,
                         const CanvasTransform& transform,
                         const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (state.activeLevelIndex >= source.levels.size()) {
    return;
  }
  const cr::CreativeWorldLayoutLevel& level =
      source.levels[state.activeLevelIndex];
  const bool drawsAuthoredFootprint =
      level.roofStyle == cr::CreativeStructuralRoofStyle::Gable ||
      level.roofOverhangCells > 0.0;
  if (!drawsAuthoredFootprint ||
      !cr::creativeWorldLayoutLevelIsTopmostOccupied(
          source, state.activeLevelIndex)) {
    return;
  }
  cr::CreativeWorldLayoutRect footprint;
  if (!cr::creativeWorldLayoutLevelRoofFootprint(
          source, state.activeLevelIndex, footprint)) {
    return;
  }
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, level.buildingIndex);
  const double minimumX = footprint.minimum.x - level.roofOverhangCells +
                          deltaX;
  const double maximumX = footprint.maximum.x + level.roofOverhangCells +
                          deltaX;
  const double minimumZ = footprint.minimum.z - level.roofOverhangCells +
                          deltaZ;
  const double maximumZ = footprint.maximum.z + level.roofOverhangCells +
                          deltaZ;
  const ImVec2 first = toScreen(transform, minimumX, minimumZ);
  const ImVec2 second = toScreen(transform, maximumX, maximumZ);
  const ImVec2 minimum{std::min(first.x, second.x),
                       std::min(first.y, second.y)};
  const ImVec2 maximum{std::max(first.x, second.x),
                       std::max(first.y, second.y)};
  const ImU32 outline = color({0.34F, 0.86F, 0.56F, 0.92F});
  drawList.AddRectFilled(minimum, maximum,
                         color({0.20F, 0.54F, 0.34F, 0.10F}));
  drawList.AddRect(minimum, maximum, outline, 0.0F, 0, 2.0F);
  if (level.roofStyle != cr::CreativeStructuralRoofStyle::Gable) {
    return;
  }
  if (level.roofRidgeAxis == cr::CreativeStructuralRoofRidgeAxis::X) {
    const double centerZ = (minimumZ + maximumZ) * 0.5;
    drawList.AddLine(toScreen(transform, minimumX, centerZ),
                     toScreen(transform, maximumX, centerZ), outline, 3.0F);
  } else {
    const double centerX = (minimumX + maximumX) * 0.5;
    drawList.AddLine(toScreen(transform, centerX, minimumZ),
                     toScreen(transform, centerX, maximumZ), outline, 3.0F);
  }
}

void drawSharedRoomEdges(ImDrawList& drawList,
                         const CanvasTransform& transform,
                         const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  const auto spans = cr::inspectCreativeWorldLayoutSharedRoomEdges(source);
  const ImU32 sharedColor = color({0.26F, 0.84F, 0.58F, 1.0F});
  for (const cr::CreativeWorldLayoutSharedRoomEdgeSpan& span : spans) {
    if (!roomOnActiveLevel(state, source, span.firstRoomIndex)) {
      continue;
    }
    const auto [deltaX, deltaZ] =
        buildingPreviewOffset(state,
                              source.rooms[span.firstRoomIndex].buildingIndex);
    drawList.AddLine(
        toScreen(transform, span.start.x + deltaX, span.start.z + deltaZ),
        toScreen(transform, span.end.x + deltaX, span.end.z + deltaZ),
        sharedColor, 4.0F);
  }
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
                          bool previewValid);

void drawVerticalConnector(ImDrawList& drawList,
                           const CanvasTransform& transform,
                           const CreativeEditorWorldLayoutState& state,
                           std::size_t connectorIndex) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (!creativeEditorWorldLayoutVerticalConnectorOnActiveLevel(
          state, source, connectorIndex)) {
    return;
  }
  const cr::CreativeWorldLayoutVerticalConnector& connector =
      source.verticalConnectors[connectorIndex];
  const bool active = state.verticalConnectorManipulation.active &&
                      state.verticalConnectorManipulation.target
                              .connectorIndex == connectorIndex;
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, connector.buildingIndex);
  const cr::CreativeWorldLayoutRect footprint =
      active ? state.verticalConnectorManipulation.previewFootprint
             : connector.footprint;
  const cr::CreativeWorldLayoutVerticalDirection direction =
      active ? state.verticalConnectorManipulation.previewDirection
             : connector.direction;
  const ImVec2 first = toScreen(transform, footprint.minimum.x + deltaX,
                                footprint.minimum.z + deltaZ);
  const ImVec2 second = toScreen(transform, footprint.maximum.x + deltaX,
                                 footprint.maximum.z + deltaZ);
  const ImVec2 minimum{std::min(first.x, second.x),
                       std::min(first.y, second.y)};
  const ImVec2 maximum{std::max(first.x, second.x),
                       std::max(first.y, second.y)};
  const bool isSelected =
      selected(state, CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
               connectorIndex);
  const cr::CreativeWorldLayoutVerticalConnectorKind kind =
      active ? state.verticalConnectorManipulation.previewKind
             : connector.kind;
  const bool isRamp =
      kind == cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  const ImU32 outline = active && !state.verticalConnectorManipulation.previewValid
                            ? color({0.92F, 0.29F, 0.24F, 1.0F})
                        : isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                        : isRamp   ? color({0.92F, 0.58F, 0.20F, 1.0F})
                                   : color({0.24F, 0.72F, 0.88F, 1.0F});
  drawList.AddRectFilled(minimum, maximum,
                         isRamp ? color({0.72F, 0.38F, 0.12F, 0.24F})
                                : color({0.18F, 0.55F, 0.72F, 0.24F}));
  drawList.AddRect(minimum, maximum, outline, 0.0F, 0,
                   isSelected ? 3.0F : 2.0F);

  CreativeEditorWorldLayoutPoint low;
  CreativeEditorWorldLayoutPoint high;
  if (!resolveCreativeEditorWorldLayoutVerticalConnectorAxis(
          footprint, direction, low, high)) {
    return;
  }
  low.x += deltaX;
  low.z += deltaZ;
  high.x += deltaX;
  high.z += deltaZ;
  const ImVec2 lowScreen = toScreen(transform, low.x, low.z);
  const ImVec2 highScreen = toScreen(transform, high.x, high.z);
  drawList.AddLine(lowScreen, highScreen, outline, 2.5F);
  const float dx = highScreen.x - lowScreen.x;
  const float dy = highScreen.y - lowScreen.y;
  const float length = std::hypot(dx, dy);
  if (length > 0.0F) {
    const float ux = dx / length;
    const float uy = dy / length;
    const ImVec2 base{highScreen.x - ux * 11.0F, highScreen.y - uy * 11.0F};
    drawList.AddTriangleFilled(
        highScreen, {base.x - uy * 5.5F, base.y + ux * 5.5F},
        {base.x + uy * 5.5F, base.y - ux * 5.5F}, outline);
  }
  if (!isRamp) {
    for (int tread = 1; tread < 6; ++tread) {
      const float t = static_cast<float>(tread) / 6.0F;
      if (std::fabs(dx) >= std::fabs(dy)) {
        const float x = lowScreen.x + dx * t;
        drawList.AddLine({x, minimum.y + 3.0F}, {x, maximum.y - 3.0F}, outline,
                         1.0F);
      } else {
        const float y = lowScreen.y + dy * t;
        drawList.AddLine({minimum.x + 3.0F, y}, {maximum.x - 3.0F, y}, outline,
                         1.0F);
      }
    }
  }
  if (isSelected) {
    drawRectManipulation(drawList, transform, footprint, active,
                         !active ||
                             state.verticalConnectorManipulation.previewValid);
    CreativeEditorWorldLayoutPoint directionHandle;
    if (resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
            footprint, direction, directionHandle)) {
      directionHandle.x += deltaX;
      directionHandle.z += deltaZ;
      const ImVec2 handleScreen =
          toScreen(transform, directionHandle.x, directionHandle.z);
      drawList.AddLine(highScreen, handleScreen, outline, 2.0F);
      drawList.AddCircleFilled(handleScreen, 5.0F, outline);
      drawList.AddCircle(handleScreen, 8.0F, outline, 0, 1.5F);
    }
  }
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
      !roomOnActiveLevel(state, state.source, state.selection.index)) {
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
  const cr::CreativeWorldLayoutWall& wall =
      creativeEditorWorldLayoutDisplaySource(state).walls[wallIndex];
  const bool isSelected =
      selected(state, CreativeEditorWorldLayoutSelectionKind::Wall, wallIndex);
  const bool active = state.wallManipulation.active &&
                      state.wallManipulation.target.wallIndex == wallIndex;
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, wall.buildingIndex);
  const double startX =
      (active ? state.wallManipulation.previewStart.x : wall.start.x) + deltaX;
  const double startZ =
      (active ? state.wallManipulation.previewStart.z : wall.start.z) + deltaZ;
  const double endX =
      (active ? state.wallManipulation.previewEnd.x : wall.end.x) + deltaX;
  const double endZ =
      (active ? state.wallManipulation.previewEnd.z : wall.end.z) + deltaZ;
  const ImVec2 start = toScreen(transform, startX, startZ);
  const ImVec2 end = toScreen(transform, endX, endZ);
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
    const double length = std::hypot(endX - startX, endZ - startZ);
    std::snprintf(dimensions, sizeof(dimensions), "%.0f long", length);
    drawList.AddText({(start.x + end.x) * 0.5F + 7.0F,
                      (start.y + end.y) * 0.5F + 7.0F},
                     wallColor, dimensions);
  }
}

void drawBuildingSelection(ImDrawList& drawList,
                           const CanvasTransform& transform,
                           const CreativeEditorWorldLayoutState& state) {
  if (state.buildingTemplatePlacement.active ||
      state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::Building ||
      state.selection.index >= state.source.buildings.size()) {
    return;
  }
  CreativeEditorWorldLayoutBuildingBounds bounds;
  if (!readCreativeEditorWorldLayoutBuildingBounds(
          state, state.selection.index, bounds)) {
    return;
  }
  const bool moveActive = state.buildingManipulation.active &&
                          state.buildingManipulation.buildingIndex ==
                              state.selection.index;
  const bool transformActive =
      state.buildingTransform.active &&
      state.buildingTransform.buildingIndex == state.selection.index;
  const bool active = moveActive || transformActive;
  const double deltaX = moveActive ? static_cast<double>(
                                         state.buildingManipulation
                                             .previewDeltaXCells)
                                   : 0.0;
  const double deltaZ = moveActive ? static_cast<double>(
                                         state.buildingManipulation
                                             .previewDeltaZCells)
                                   : 0.0;
  const ImVec2 minimum =
      toScreen(transform, bounds.minimum.x + deltaX,
               bounds.minimum.z + deltaZ);
  const ImVec2 maximum =
      toScreen(transform, bounds.maximum.x + deltaX,
               bounds.maximum.z + deltaZ);
  const ImVec4 tint =
      active ? ((transformActive || state.buildingManipulation.previewValid)
                    ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
                    : ImVec4{0.92F, 0.29F, 0.24F, 1.0F})
             : ImVec4{0.96F, 0.82F, 0.22F, 1.0F};
  ImVec4 fill = tint;
  fill.w = active ? 0.12F : 0.05F;
  drawList.AddRectFilled(minimum, maximum, color(fill));
  drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0,
                   active ? 4.0F : 3.0F);
  drawList.AddRectFilled({minimum.x - 5.0F, minimum.y - 5.0F},
                         {minimum.x + 5.0F, minimum.y + 5.0F}, color(tint));
  const std::string& name =
      state.source.buildings[state.selection.index].name;
  drawList.AddText({minimum.x + 8.0F, minimum.y + 7.0F}, color(tint),
                   name.c_str());
}

void drawBuildingTemplatePlacement(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state) {
  const auto& placement = state.buildingTemplatePlacement;
  if (!placement.active || !placement.previewBounds.valid) {
    return;
  }
  const ImVec2 minimum =
      toScreen(transform, placement.previewBounds.minimum.x,
               placement.previewBounds.minimum.z);
  const ImVec2 maximum =
      toScreen(transform, placement.previewBounds.maximum.x,
               placement.previewBounds.maximum.z);
  const ImVec4 tint = placement.previewValid
                          ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
                          : ImVec4{0.92F, 0.29F, 0.24F, 1.0F};
  ImVec4 fill = tint;
  fill.w = 0.10F;
  drawList.AddRectFilled(minimum, maximum, color(fill));
  drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0, 4.0F);
  drawList.AddRectFilled({minimum.x - 5.0F, minimum.y - 5.0F},
                         {minimum.x + 5.0F, minimum.y + 5.0F}, color(tint));
  const std::string& label = placement.orientedTemplate.label;
  drawList.AddText({minimum.x + 8.0F, minimum.y + 7.0F}, color(tint),
                   label.c_str());
}

CreativeEditorWorldLayoutPoint openingPoint(
    CreativeEditorWorldLayoutOpeningHost host, double offsetCells) {
  const double t = offsetCells / host.lengthCells;
  return {host.start.x + (host.end.x - host.start.x) * t,
          host.start.z + (host.end.z - host.start.z) * t};
}

void drawOpenings(ImDrawList& drawList, const CanvasTransform& transform,
                  const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  for (std::size_t index = 0U; index < source.openings.size(); ++index) {
    if (!openingOnActiveLevel(state, source, index)) {
      continue;
    }
    const cr::CreativeWorldLayoutOpening& opening =
        source.openings[index];
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

void drawOpeningPlacementPlan(
    ImDrawList& drawList, const CanvasTransform& transform,
    CreativeEditorWorldLayoutPoint hovered,
    const CreativeEditorWorldLayoutOpeningPlacementPlan& plan,
    cr::CreativeBuildingOpeningKind kind, std::string_view label) {
  const ImU32 previewColor =
      !plan.accepted
          ? color({0.92F, 0.29F, 0.24F, 1.0F})
          : kind == cr::CreativeBuildingOpeningKind::Door
                ? color({0.20F, 0.82F, 0.38F, 1.0F})
                : color({0.27F, 0.72F, 0.91F, 1.0F});
  if (!plan.accepted) {
    const ImVec2 center = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine({center.x - 7.0F, center.y - 7.0F},
                     {center.x + 7.0F, center.y + 7.0F}, previewColor, 3.0F);
    drawList.AddLine({center.x - 7.0F, center.y + 7.0F},
                     {center.x + 7.0F, center.y - 7.0F}, previewColor, 3.0F);
    drawList.AddText({center.x + 11.0F, center.y + 9.0F}, previewColor,
                     plan.message.data());
    return;
  }

  const ImVec2 start =
      toScreen(transform, plan.startPoint.x, plan.startPoint.z);
  const ImVec2 center =
      toScreen(transform, plan.centerPoint.x, plan.centerPoint.z);
  const ImVec2 end = toScreen(transform, plan.endPoint.x, plan.endPoint.z);
  if (plan.pointerDistanceCells > 0.01) {
    const ImVec2 pointer = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine(pointer, center, previewColor, 1.0F);
  }
  drawList.AddLine(start, end, previewColor, 8.0F);
  drawList.AddRectFilled({start.x - 3.5F, start.y - 3.5F},
                         {start.x + 3.5F, start.y + 3.5F}, previewColor);
  drawList.AddRectFilled({end.x - 3.5F, end.y - 3.5F},
                         {end.x + 3.5F, end.y + 3.5F}, previewColor);
  if (kind == cr::CreativeBuildingOpeningKind::Door) {
    drawList.AddCircleFilled(center, 6.0F, previewColor);
  } else {
    drawList.AddRectFilled({center.x - 6.0F, center.y - 6.0F},
                           {center.x + 6.0F, center.y + 6.0F}, previewColor);
  }
  char placementLabel[96]{};
  const std::size_t visibleLabelSize =
      std::min(label.size(), std::size_t{48U});
  std::snprintf(placementLabel, sizeof(placementLabel),
                "%.*s | %.2f wide x %.2f high",
                static_cast<int>(visibleLabelSize), label.data(),
                plan.opening.widthCells, plan.opening.cutoutHeightCells);
  drawList.AddText({center.x + 9.0F, center.y + 9.0F}, previewColor,
                   placementLabel);
}

void drawOpeningPlacementPreview(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint hovered) {
  cr::CreativeBuildingOpeningKind kind;
  if (state.tool == CreativeEditorWorldLayoutTool::Door) {
    kind = cr::CreativeBuildingOpeningKind::Door;
  } else if (state.tool == CreativeEditorWorldLayoutTool::Window) {
    kind = cr::CreativeBuildingOpeningKind::Window;
  } else {
    return;
  }
  const CreativeEditorWorldLayoutOpeningPlacementPlan plan =
      planCreativeEditorWorldLayoutOpeningPlacement(state, hovered, kind);
  drawOpeningPlacementPlan(
      drawList, transform, hovered, plan, kind,
      kind == cr::CreativeBuildingOpeningKind::Door ? "Door" : "Window");
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
  if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell ||
      state.tool == CreativeEditorWorldLayoutTool::Room ||
      state.tool == CreativeEditorWorldLayoutTool::Floor ||
      creativeEditorWorldLayoutToolIsVerticalConnector(state.tool) ||
      state.tool == CreativeEditorWorldLayoutTool::Bridge) {
    const ImVec2 end = toScreen(transform, snappedX, snappedZ);
    if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell ||
        state.tool == CreativeEditorWorldLayoutTool::Room ||
        creativeEditorWorldLayoutToolIsVerticalConnector(state.tool) ||
        state.tool == CreativeEditorWorldLayoutTool::Bridge) {
      drawList.AddRectFilled(
          {std::min(start.x, end.x), std::min(start.y, end.y)},
          {std::max(start.x, end.x), std::max(start.y, end.y)},
          state.tool == CreativeEditorWorldLayoutTool::Ramp
              ? color({0.72F, 0.38F, 0.12F, 0.28F})
              : creativeEditorWorldLayoutToolIsVerticalConnector(state.tool)
                    ? color({0.18F, 0.55F, 0.72F, 0.28F})
                    : color({0.22F, 0.58F, 0.38F, 0.22F}));
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
  } else if (state.tool == CreativeEditorWorldLayoutTool::Road ||
             state.tool == CreativeEditorWorldLayoutTool::Ditch) {
    const ImVec2 end = toScreen(transform, snappedX, snappedZ);
    const float width = state.tool == CreativeEditorWorldLayoutTool::Ditch
                            ? transform.pixelsPerCell * 3.0F
                            : transform.pixelsPerCell * 3.0F;
    drawList.AddLine(start, end,
                     state.tool == CreativeEditorWorldLayoutTool::Ditch
                         ? color({0.25F, 0.47F, 0.68F, 0.32F})
                         : color({0.72F, 0.56F, 0.28F, 0.32F}),
                     width);
    drawList.AddLine(start, end, previewColor, 2.0F);
  }
}

void drawCatalogPlacementPreview(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint hovered,
    cr::CreativeGridSettings grid) {
  if (state.tool != CreativeEditorWorldLayoutTool::CatalogAsset ||
      !state.catalogPlacement.active) {
    return;
  }
  const CreativeEditorWorldLayoutCatalogPlacementPlan plan =
      planCreativeEditorWorldLayoutCatalogPlacement(state, hovered, grid);
  if (plan.hostedOpening) {
    drawOpeningPlacementPlan(
        drawList, transform, hovered, plan.openingPlacement,
        state.catalogPlacement.categoryId == "window"
            ? cr::CreativeBuildingOpeningKind::Window
            : cr::CreativeBuildingOpeningKind::Door,
        state.catalogPlacement.label);
    return;
  }
  const ImU32 outline = plan.accepted
                            ? color({0.20F, 0.82F, 0.38F, 1.0F})
                            : color({0.92F, 0.29F, 0.24F, 1.0F});
  if (!plan.accepted || !plan.footprint.valid) {
    const ImVec2 center = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine({center.x - 8.0F, center.y - 8.0F},
                     {center.x + 8.0F, center.y + 8.0F}, outline, 3.0F);
    drawList.AddLine({center.x - 8.0F, center.y + 8.0F},
                     {center.x + 8.0F, center.y - 8.0F}, outline, 3.0F);
    drawList.AddText({center.x + 12.0F, center.y + 10.0F}, outline,
                     plan.message.c_str());
    return;
  }
  std::array<ImVec2, 4U> points;
  for (std::size_t index = 0U; index < points.size(); ++index) {
    points[index] = toScreen(transform, plan.footprint.corners[index].x,
                             plan.footprint.corners[index].z);
  }
  drawList.AddConvexPolyFilled(points.data(), static_cast<int>(points.size()),
                               color({0.20F, 0.82F, 0.38F, 0.22F}));
  drawList.AddPolyline(points.data(), static_cast<int>(points.size()), outline,
                       ImDrawFlags_Closed, 2.5F);
  const ImVec2 pivot = toScreen(transform, plan.object.pointCells.x,
                                plan.object.pointCells.z);
  if (plan.snapMode == CreativeEditorWorldLayoutCatalogSnapMode::Wall) {
    const CreativeEditorWorldLayoutPoint centerline{
        plan.snapSurfacePoint.x -
            plan.snapNormal.x * plan.snapWallThicknessCells * 0.5,
        plan.snapSurfacePoint.z -
            plan.snapNormal.z * plan.snapWallThicknessCells * 0.5};
    const CreativeEditorWorldLayoutPoint tangent{-plan.snapNormal.z,
                                                  plan.snapNormal.x};
    const CreativeEditorWorldLayoutPoint faceStart{
        plan.snapSurfacePoint.x - tangent.x * 0.45,
        plan.snapSurfacePoint.z - tangent.z * 0.45};
    const CreativeEditorWorldLayoutPoint faceEnd{
        plan.snapSurfacePoint.x + tangent.x * 0.45,
        plan.snapSurfacePoint.z + tangent.z * 0.45};
    const CreativeEditorWorldLayoutPoint normalTip{
        plan.snapSurfacePoint.x + plan.snapNormal.x * 0.65,
        plan.snapSurfacePoint.z + plan.snapNormal.z * 0.65};
    const ImVec2 centerlineScreen =
        toScreen(transform, centerline.x, centerline.z);
    const ImVec2 surfaceScreen = toScreen(
        transform, plan.snapSurfacePoint.x, plan.snapSurfacePoint.z);
    const ImVec2 normalTipScreen =
        toScreen(transform, normalTip.x, normalTip.z);
    const ImU32 faceColor = color({0.98F, 0.78F, 0.20F, 1.0F});
    if (plan.snapDistanceCells > 0.01) {
      const ImVec2 pointer = toScreen(transform, hovered.x, hovered.z);
      drawList.AddLine(pointer, centerlineScreen, outline, 1.0F);
    }
    drawList.AddLine(centerlineScreen, surfaceScreen, faceColor, 2.0F);
    drawList.AddLine(toScreen(transform, faceStart.x, faceStart.z),
                     toScreen(transform, faceEnd.x, faceEnd.z), faceColor,
                     3.0F);
    drawList.AddCircleFilled(surfaceScreen, 3.5F, faceColor);
    drawList.AddLine(surfaceScreen, normalTipScreen, outline, 2.0F);
    const float arrowDx = normalTipScreen.x - surfaceScreen.x;
    const float arrowDy = normalTipScreen.y - surfaceScreen.y;
    const float arrowLength = std::hypot(arrowDx, arrowDy);
    if (arrowLength > 0.0F) {
      const float unitX = arrowDx / arrowLength;
      const float unitY = arrowDy / arrowLength;
      const ImVec2 arrowBase{normalTipScreen.x - unitX * 7.0F,
                             normalTipScreen.y - unitY * 7.0F};
      const ImVec2 arrowSide{-unitY * 3.5F, unitX * 3.5F};
      drawList.AddTriangleFilled(
          normalTipScreen,
          {arrowBase.x + arrowSide.x, arrowBase.y + arrowSide.y},
          {arrowBase.x - arrowSide.x, arrowBase.y - arrowSide.y}, outline);
    }
  }
  drawList.AddCircleFilled(pivot, 3.5F, outline);
  const std::string previewLabel =
      state.catalogPlacement.label + " | " + plan.message;
  drawList.AddText({points.front().x + 6.0F, points.front().y + 6.0F}, outline,
                   previewLabel.c_str());
}

void queueTool(CreativeDesktopCommandFrame& commands,
               CreativeEditorWorldLayoutTool tool) {
  commands.push(CreativeDesktopCommandId::WorldLayoutSetTool,
                CreativeDesktopWorldLayoutToolPayload{tool});
}

void queueLevelOperation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutLevelOperation operation,
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex,
    std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutLevelOperation,
      CreativeDesktopWorldLayoutLevelOperationPayload{operation,
                                                      buildingIndex,
                                                      levelIndex});
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

std::size_t findBuildingTemplate(
    const CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    std::string_view templateId) noexcept {
  const auto found = std::find_if(
      library.templates.begin(), library.templates.end(),
      [templateId](const cr::CreativeWorldLayoutBuildingTemplate& value) {
        return value.templateId == templateId;
      });
  return found == library.templates.end()
             ? cr::kInvalidCreativeWorldLayoutIndex
             : static_cast<std::size_t>(found - library.templates.begin());
}

void drawWorldLayoutPalette(CreativeEditorWorldLayoutState& state,
                            CreativeDesktopCommandFrame& commands) {
  if (!ImGui::BeginTabBar("##world_layout_palette")) {
    return;
  }
  for (std::uint8_t categoryValue = 0U;
       categoryValue < static_cast<std::uint8_t>(
                           CreativeEditorWorldLayoutPaletteCategory::Count);
       ++categoryValue) {
    const auto category =
        static_cast<CreativeEditorWorldLayoutPaletteCategory>(categoryValue);
    if (!ImGui::BeginTabItem(
            creativeEditorWorldLayoutPaletteCategoryLabel(category))) {
      continue;
    }
    bool first = true;
    for (const CreativeEditorWorldLayoutPaletteEntry& entry :
         creativeEditorWorldLayoutPaletteEntries()) {
      if (entry.category != category) {
        continue;
      }
      if (!first) {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float buttonWidth = ImGui::CalcTextSize(entry.label.data()).x +
                                  (2.0F * style.FramePadding.x);
        const float nextButtonRight = ImGui::GetItemRectMax().x +
                                      style.ItemSpacing.x + buttonWidth;
        const float contentRight = ImGui::GetWindowPos().x +
                                   ImGui::GetWindowContentRegionMax().x;
        if (nextButtonRight <= contentRight) {
          ImGui::SameLine();
        }
      }
      first = false;
      const bool toolActive =
          entry.activation == CreativeEditorWorldLayoutPaletteActivation::Tool &&
          state.tool == entry.tool && !state.buildingTemplatePlacement.active;
      if (toolActive) {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4{0.16F, 0.47F, 0.25F, 1.0F});
      }
      const std::size_t templateIndex =
          entry.activation ==
                  CreativeEditorWorldLayoutPaletteActivation::BuildingTemplate
              ? findBuildingTemplate(state.buildingTemplates,
                                     entry.buildingTemplateId)
              : cr::kInvalidCreativeWorldLayoutIndex;
      const bool unavailable =
          entry.activation ==
              CreativeEditorWorldLayoutPaletteActivation::BuildingTemplate &&
          templateIndex == cr::kInvalidCreativeWorldLayoutIndex;
      ImGui::BeginDisabled(unavailable);
      if (ImGui::Button(entry.label.data())) {
        if (entry.activation ==
            CreativeEditorWorldLayoutPaletteActivation::Tool) {
          queueTool(commands, entry.tool);
        } else {
          if (state.tool != CreativeEditorWorldLayoutTool::Select) {
            queueTool(commands, CreativeEditorWorldLayoutTool::Select);
          }
          commands.push(
              CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate,
              CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload{
                  templateIndex});
          commands.push(
              CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
              CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
                  CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
                  {0.0, 0.0},
                  cr::CreativeWorldLayoutBuildingTransformOperation::
                      RotateRight90});
        }
      }
      ImGui::EndDisabled();
      if (toolActive) {
        ImGui::PopStyleColor();
      }
    }
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
}

void drawWorldLayoutAssetPlacementControls(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutCatalogPlacementState& placement =
      state.catalogPlacement;
  if (!placement.active) {
    return;
  }
  ImGui::SeparatorText("Placement");
  ImGui::TextUnformatted(placement.label.c_str());
  ImGui::TextUnformatted("Snap");
  const bool hostedOpening =
      creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
          placement.categoryId);
  if (hostedOpening) {
    ImGui::RadioButton("Wall", true);
  } else {
    constexpr std::array kSnapModes{
        CreativeEditorWorldLayoutCatalogSnapMode::Grid,
        CreativeEditorWorldLayoutCatalogSnapMode::Floor,
        CreativeEditorWorldLayoutCatalogSnapMode::Wall,
    };
    for (std::size_t index = 0U; index < kSnapModes.size(); ++index) {
      const CreativeEditorWorldLayoutCatalogSnapMode mode = kSnapModes[index];
      if (index > 0U) {
        ImGui::SameLine();
      }
      const bool wallUnsupported =
          mode == CreativeEditorWorldLayoutCatalogSnapMode::Wall &&
          !creativeEditorWorldLayoutCatalogAssetSupportsWallSnap(
              placement.categoryId);
      ImGui::BeginDisabled(wallUnsupported);
      const bool selected = placement.snapMode == mode;
      if (ImGui::RadioButton(
              creativeEditorWorldLayoutCatalogSnapModeLabel(mode), selected) &&
          !selected) {
        placement.snapMode = mode;
      }
      ImGui::EndDisabled();
      if (wallUnsupported &&
          ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Architecture assets only");
      }
    }
    ImGui::BeginDisabled(
        placement.snapMode != CreativeEditorWorldLayoutCatalogSnapMode::Grid);
    ImGui::InputDouble("Grid elevation##layout_asset",
                       &placement.elevationCells, 0.25, 1.0, "%.3f");
    ImGui::EndDisabled();
    const char* yawLabel =
        placement.snapMode == CreativeEditorWorldLayoutCatalogSnapMode::Wall
            ? "Yaw offset##layout_asset"
            : "Yaw##layout_asset";
    ImGui::InputDouble(yawLabel, &placement.yawDegrees, 15.0, 90.0,
                       "%.1f deg");
    if (placement.snapMode == CreativeEditorWorldLayoutCatalogSnapMode::Wall) {
      ImGui::Checkbox("Flip wall side##layout_asset",
                      &placement.wallSideFlipped);
    }
  }
  ImGui::InputDouble("Scale X##layout_asset", &placement.scale.x, 0.1, 1.0,
                     "%.3f");
  ImGui::InputDouble("Scale Y##layout_asset", &placement.scale.y, 0.1, 1.0,
                     "%.3f");
  ImGui::InputDouble("Scale Z##layout_asset", &placement.scale.z, 0.1, 1.0,
                     "%.3f");
  if (hostedOpening) {
    ImGui::SeparatorText("Selected opening");
    const bool hasOpeningSelection =
        state.selection.kind ==
            CreativeEditorWorldLayoutSelectionKind::Opening &&
        state.selection.index < state.source.openings.size();
    const bool compatible =
        hasOpeningSelection &&
        creativeEditorWorldLayoutCatalogAssetMatchesOpening(
            placement.categoryId,
            state.source.openings[state.selection.index].kind);
    if (!hasOpeningSelection) {
      ImGui::TextDisabled("Select an opening to replace");
    } else if (!compatible) {
      ImGui::TextDisabled("Selected opening requires a matching asset kind");
    }
    ImGui::BeginDisabled(!compatible);
    if (ImGui::Button("Fit asset to opening")) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSetOpeningInsert,
          CreativeDesktopWorldLayoutOpeningInsertPayload{
              state.selection.index,
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  FitAssetToOpening,
              placement.assetId,
              placement.scale});
    }
    if (ImGui::Button("Resize opening to asset")) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSetOpeningInsert,
          CreativeDesktopWorldLayoutOpeningInsertPayload{
              state.selection.index,
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  ResizeOpeningToAsset,
              placement.assetId,
              placement.scale});
    }
    ImGui::EndDisabled();
  }
  if (!hostedOpening) {
    if (ImGui::Button("Rotate left##layout_asset")) {
      placement.yawDegrees -= 90.0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Rotate right##layout_asset")) {
      placement.yawDegrees += 90.0;
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Reset##layout_asset")) {
    placement.elevationCells = 0.0;
    placement.yawDegrees = 0.0;
    placement.scale = {1.0, 1.0, 1.0};
    placement.wallSideFlipped = false;
  }
}

void drawWorldLayoutAssetPalette(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands) {
  ImGui::SeparatorText("Asset library");
  inputText("Search##world_layout_assets", state.assetQuery);
  if (!ImGui::BeginTabBar("##world_layout_asset_categories")) {
    return;
  }
  for (std::uint8_t categoryValue = 0U;
       categoryValue < static_cast<std::uint8_t>(
                           CreativeEditorWorldLayoutAssetCategory::Count);
       ++categoryValue) {
    const auto category =
        static_cast<CreativeEditorWorldLayoutAssetCategory>(categoryValue);
    if (!ImGui::BeginTabItem(
            creativeEditorWorldLayoutAssetCategoryLabel(category))) {
      continue;
    }
    state.assetCategory = category;
    std::size_t visibleCount = 0U;
    if (ImGui::BeginChild("##world_layout_asset_list", {0.0F, 210.0F}, true)) {
      for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
        if (entry.category != cr::CreativeCatalogEntryCategory::Asset ||
            classifyCreativeEditorWorldLayoutAsset(entry) != category ||
            !creativeEditorWorldLayoutAssetMatchesQuery(entry,
                                                        state.assetQuery)) {
          continue;
        }
        ++visibleCount;
        const std::string_view assetId =
            cr::creativeHotbarAssetId(entry.hotbarEntry);
        const bool selected =
            state.tool == CreativeEditorWorldLayoutTool::CatalogAsset &&
            state.catalogPlacement.active &&
            state.catalogPlacement.assetId == assetId;
        ImGui::PushID(assetId.data(), assetId.data() + assetId.size());
        if (ImGui::Selectable(entry.label.c_str(), selected)) {
          commands.push(
              CreativeDesktopCommandId::WorldLayoutSelectCatalogAsset,
              CreativeDesktopWorldLayoutCatalogAssetPayload{
                  std::string(assetId)});
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%.*s\n%s", static_cast<int>(assetId.size()),
                            assetId.data(),
                            entry.assetAuthoringMetadata.categoryId.c_str());
        }
        ImGui::PopID();
      }
      if (visibleCount == 0U) {
        ImGui::TextDisabled("No matching assets");
      }
    }
    ImGui::EndChild();
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
  drawWorldLayoutAssetPlacementControls(state, commands);
}

void drawWorldLayoutLevels(CreativeEditorDesktopUiState& desktopUi,
                           CreativeEditorWorldLayoutState& state,
                           CreativeDesktopCommandFrame& commands) {
  std::size_t buildingIndex = creativeEditorWorldLayoutSelectedBuilding(state);
  if (buildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      state.activeLevelIndex < state.source.levels.size()) {
    buildingIndex = state.source.levels[state.activeLevelIndex].buildingIndex;
  }
  if (buildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      state.source.buildings.size() == 1U) {
    buildingIndex = 0U;
  }

  ImGui::TextUnformatted("Levels");
  ImGui::Separator();
  bool anyLevel = false;
  for (std::size_t index = 0U; index < state.source.levels.size(); ++index) {
    const cr::CreativeWorldLayoutLevel& level = state.source.levels[index];
    if (level.buildingIndex != buildingIndex) {
      continue;
    }
    anyLevel = true;
    const bool active = index == state.activeLevelIndex;
    const std::string label = level.name + "##layout_level_" +
                              std::to_string(index);
    if (ImGui::Selectable(label.c_str(), active)) {
      queueLevelOperation(commands,
                          CreativeEditorWorldLayoutLevelOperation::Select,
                          buildingIndex, index);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Floor top %.3f", level.floorTopLayer);
    }
  }
  if (!anyLevel && buildingIndex != cr::kInvalidCreativeWorldLayoutIndex) {
    ImGui::TextDisabled("No levels");
  }

  const bool hasBuilding = buildingIndex < state.source.buildings.size();
  const bool hasLevel = state.activeLevelIndex < state.source.levels.size() &&
                        state.source.levels[state.activeLevelIndex]
                                .buildingIndex == buildingIndex;
  ImGui::BeginDisabled(!hasBuilding);
  if (ImGui::SmallButton("+##layout_level_add")) {
    queueLevelOperation(commands,
                        CreativeEditorWorldLayoutLevelOperation::Add,
                        buildingIndex);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("Add level above this building");
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!hasLevel);
  if (ImGui::SmallButton("Copy##layout_level_copy")) {
    queueLevelOperation(commands,
                        CreativeEditorWorldLayoutLevelOperation::Duplicate,
                        buildingIndex, state.activeLevelIndex);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("Duplicate this level and its rooms above the building");
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Up##layout_level_earlier")) {
    queueLevelOperation(commands,
                        CreativeEditorWorldLayoutLevelOperation::MoveEarlier,
                        buildingIndex, state.activeLevelIndex);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("Move level earlier in the tab order");
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Down##layout_level_later")) {
    queueLevelOperation(commands,
                        CreativeEditorWorldLayoutLevelOperation::MoveLater,
                        buildingIndex, state.activeLevelIndex);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("Move level later in the tab order");
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Delete##layout_level_delete")) {
    desktopUi.worldLayoutDeleteLevelIndex = state.activeLevelIndex;
    ImGui::OpenPopup("Delete building level");
  }
  ImGui::EndDisabled();
}

void drawWorldLayoutLevelDeleteModal(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  if (ImGui::BeginPopupModal("Delete building level", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    const std::size_t levelIndex = desktopUi.worldLayoutDeleteLevelIndex;
    std::size_t roomCount = 0U;
    std::size_t openingCount = 0U;
    std::array<std::size_t,
               static_cast<std::size_t>(
                   cr::CreativeWorldLayoutVerticalConnectorKind::Count)>
        connectorCounts{};
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
    if (levelIndex < state.source.levels.size()) {
      buildingIndex = state.source.levels[levelIndex].buildingIndex;
      roomCount = static_cast<std::size_t>(std::count_if(
          state.source.rooms.begin(), state.source.rooms.end(),
          [levelIndex](const cr::CreativeWorldLayoutRoom& room) {
            return room.levelIndex == levelIndex;
          }));
      openingCount = static_cast<std::size_t>(std::count_if(
          state.source.openings.begin(), state.source.openings.end(),
          [&state, levelIndex](const cr::CreativeWorldLayoutOpening& opening) {
            return opening.hostKind ==
                       cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                   opening.roomIndex < state.source.rooms.size() &&
                   state.source.rooms[opening.roomIndex].levelIndex ==
                       levelIndex;
          }));
      for (const cr::CreativeWorldLayoutVerticalConnector& connector :
           state.source.verticalConnectors) {
        const bool touchesLevel =
            (connector.lowerRoomIndex < state.source.rooms.size() &&
             state.source.rooms[connector.lowerRoomIndex].levelIndex ==
                 levelIndex) ||
            (connector.upperRoomIndex < state.source.rooms.size() &&
             state.source.rooms[connector.upperRoomIndex].levelIndex ==
                 levelIndex);
        const std::size_t kind = static_cast<std::size_t>(connector.kind);
        if (touchesLevel && kind < connectorCounts.size()) {
          ++connectorCounts[kind];
        }
      }
    }
    ImGui::Text("Delete this level, %llu room(s), %llu opening(s), %llu "
                "stair(s), and %llu ramp(s)?",
                static_cast<unsigned long long>(roomCount),
                static_cast<unsigned long long>(openingCount),
                static_cast<unsigned long long>(
                    connectorCounts[static_cast<std::size_t>(
                        cr::CreativeWorldLayoutVerticalConnectorKind::Stair)]),
                static_cast<unsigned long long>(
                    connectorCounts[static_cast<std::size_t>(
                        cr::CreativeWorldLayoutVerticalConnectorKind::Ramp)]));
    ImGui::BeginDisabled(levelIndex >= state.source.levels.size());
    if (ImGui::Button("Delete level")) {
      queueLevelOperation(commands,
                          CreativeEditorWorldLayoutLevelOperation::Delete,
                          buildingIndex, levelIndex);
      desktopUi.worldLayoutDeleteLevelIndex =
          std::numeric_limits<std::size_t>::max();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      desktopUi.worldLayoutDeleteLevelIndex =
          std::numeric_limits<std::size_t>::max();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
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
  const cr::CreativeWorldLayoutLevel* level =
      cr::creativeWorldLayoutLevelForRoom(state.source,
                                          state.selection.index);
  if (level == nullptr) {
    ImGui::TextDisabled("Room level is unavailable");
    return;
  }
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
  double floorTopLayer = level->floorTopLayer;
  int wallHeight = level->wallHeightCells;
  double wallThickness = room.wallThicknessCells;
  int floorLayers = level->floorThicknessLayers;
  int roofLayers = level->roofThicknessLayers;
  int roofStyle = static_cast<int>(level->roofStyle);
  int roofRidgeAxis = static_cast<int>(level->roofRidgeAxis);
  double roofPitch = level->roofPitchDegrees;
  double roofOverhang = level->roofOverhangCells;

  ImGui::SetNextItemWidth(128.0F);
  bool changed = ImGui::InputInt("Width##room_shell", &width, 1, 4);
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputInt("Depth##room_shell", &depth, 1, 4) || changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputDouble("Level floor##room_shell", &floorTopLayer, 0.5,
                               1.0, "%.3f") ||
            changed;

  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputInt("Level wall height##room_shell", &wallHeight, 1,
                            4) ||
            changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputDouble("Room wall##room_shell", &wallThickness,
                               0.05, 0.25, "%.3f") ||
            changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputInt("Level floor layers##room_shell", &floorLayers, 1,
                            2) ||
            changed;

  ImGui::SeparatorText("Level roof");
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::Combo("Style##room_roof", &roofStyle,
                         "Flat\0Gable\0") ||
            changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputInt("Layers##room_roof", &roofLayers, 1, 2) ||
            changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputDouble("Overhang##room_roof", &roofOverhang, 0.25,
                               1.0, "%.2f") ||
            changed;
  if (roofStyle == static_cast<int>(cr::CreativeStructuralRoofStyle::Gable)) {
    ImGui::SetNextItemWidth(128.0F);
    changed = ImGui::Combo("Ridge##room_roof", &roofRidgeAxis,
                           "X axis\0Z axis\0") ||
              changed;
    ImGui::SetNextItemWidth(128.0F);
    changed = ImGui::InputDouble("Pitch##room_roof", &roofPitch, 1.0, 5.0,
                                 "%.1f deg") ||
              changed;
  }

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
      roofLayers > 0 && roofLayers <= maximumLayerCount &&
      std::isfinite(floorTopLayer) && std::isfinite(wallThickness) &&
      roofStyle >= 0 &&
      roofStyle < static_cast<int>(cr::CreativeStructuralRoofStyle::Count) &&
      roofRidgeAxis >= 0 &&
      roofRidgeAxis <
          static_cast<int>(cr::CreativeStructuralRoofRidgeAxis::Count) &&
      cr::validCreativeStructuralRoofSettings(
          static_cast<cr::CreativeStructuralRoofStyle>(roofStyle),
          static_cast<cr::CreativeStructuralRoofRidgeAxis>(roofRidgeAxis),
          roofPitch, roofOverhang) &&
      roofOverhang <= cr::kMaximumCreativeWorldLayoutRoofOverhangCells &&
      wallThickness > 0.0 &&
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
  settings.floorTopLayer = floorTopLayer;
  settings.wallHeightCells = static_cast<std::uint16_t>(wallHeight);
  settings.wallThicknessCells = wallThickness;
  settings.floorThicknessLayers = static_cast<std::uint16_t>(floorLayers);
  settings.roofThicknessLayers = static_cast<std::uint16_t>(roofLayers);
  settings.roofStyle =
      static_cast<cr::CreativeStructuralRoofStyle>(roofStyle);
  settings.roofRidgeAxis =
      static_cast<cr::CreativeStructuralRoofRidgeAxis>(roofRidgeAxis);
  settings.roofPitchDegrees = roofPitch;
  settings.roofOverhangCells = roofOverhang;
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

bool catalogContainsAsset(const cr::CreativeCatalogState& catalog,
                          std::string_view assetId) {
  return std::any_of(
      catalog.entries.begin(), catalog.entries.end(),
      [assetId](const cr::CreativeCatalogEntry& entry) {
        return entry.category == cr::CreativeCatalogEntryCategory::Asset &&
               cr::creativeHotbarAssetId(entry.hotbarEntry) == assetId;
      });
}

void drawSelectedOpeningSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
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
  if (!opening.insertAssetId.empty()) {
    ImGui::TextDisabled("Asset: %s", opening.insertAssetId.c_str());
    if (!catalogContainsAsset(catalog, opening.insertAssetId)) {
      ImGui::TextColored({1.0F, 0.72F, 0.20F, 1.0F},
                         "Asset unavailable: procedural preview");
    }
  }
  ImGui::SetNextItemWidth(128.0F);
  ImGui::InputDouble("Offset##opening", &settings.centerOffsetCells, 0.25, 1.0,
                     "%.2f");
  ImGui::SetNextItemWidth(128.0F);
  ImGui::InputDouble("Width##opening", &settings.widthCells, 0.25, 1.0,
                     "%.2f");
  ImGui::SetNextItemWidth(128.0F);
  ImGui::InputDouble("Height##opening", &settings.heightCells, 0.25, 1.0,
                     "%.2f");

  if (isDoor) {
    settings.sillHeightCells = 0.0;
    constexpr std::array<const char*, 5U> kPoseLabels = {
        "Closed", "Start hinge / side A", "Start hinge / side B",
        "End hinge / side A", "End hinge / side B"};
    int pose = static_cast<int>(settings.pose);
    ImGui::SetNextItemWidth(188.0F);
    if (ImGui::Combo("Pose##opening", &pose, kPoseLabels.data(),
                     static_cast<int>(kPoseLabels.size()))) {
      settings.pose = static_cast<cr::CreativeBuildingOpeningPose>(pose);
    }
  } else {
    settings.pose = cr::CreativeBuildingOpeningPose::Closed;
    ImGui::SetNextItemWidth(128.0F);
    ImGui::InputDouble("Sill##opening", &settings.sillHeightCells, 0.25, 1.0,
                       "%.2f");
  }
  ImGui::Checkbox("Insert##opening", &settings.includeInsert);

  const bool dirty = !sameOpeningSettings(current, settings);
  if (!opening.insertAssetId.empty()) {
    ImGui::BeginDisabled(dirty || state.openingManipulation.active);
    if (ImGui::Button("Use procedural insert")) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSetOpeningInsert,
          CreativeDesktopWorldLayoutOpeningInsertPayload{
              openingIndex,
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  UseProceduralInsert,
              {},
              {1.0, 1.0, 1.0}});
    }
    ImGui::EndDisabled();
    if (dirty && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      ImGui::SetTooltip("Apply or reset opening edits first");
    }
  }
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

void drawWorldLayoutViewControls(CreativeEditorWorldLayoutState& state,
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
    state.viewMode = CreativeEditorWorldLayoutViewMode::Elevation;
  }
  if (state.viewMode != CreativeEditorWorldLayoutViewMode::Elevation) {
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
                      cr::CreativeGridSettings grid,
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
  const cr::CreativeWorldLayout &displaySource =
      creativeEditorWorldLayoutDisplaySource(state);
  drawTerrainSymbols(*drawList, transform, state);
  for (std::size_t index = 0U; index < displaySource.rooms.size(); ++index) {
    if (roomOnActiveLevel(state, displaySource, index)) {
      drawRoom(*drawList, transform, state, index);
    }
  }
  drawActiveLevelRoof(*drawList, transform, state);
  for (std::size_t index = 0U; index < displaySource.boxes.size(); ++index) {
    drawFloor(*drawList, transform, state, index);
  }
  for (std::size_t index = 0U; index < displaySource.verticalConnectors.size();
       ++index) {
    drawVerticalConnector(*drawList, transform, state, index);
  }
  for (std::size_t index = 0U; index < displaySource.walls.size(); ++index) {
    drawWall(*drawList, transform, state, index);
  }
  drawSharedRoomEdges(*drawList, transform, state);
  drawOpenings(*drawList, transform, state);
  drawObjectSymbols(*drawList, transform, state, grid);
  drawBuildingSelection(*drawList, transform, state);
  drawBuildingTemplatePlacement(*drawList, transform, state);
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
  if (hovered && !state.buildingTemplatePlacement.active) {
    drawOpeningPlacementPreview(*drawList, transform, state, hoveredPoint);
    drawCatalogPlacementPreview(*drawList, transform, state, hoveredPoint,
                                grid);
    drawAnchorPreview(*drawList, transform, state, hoveredPoint);
  }
  drawList->PopClipRect();

  if (!interactionEnabled) {
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

[[nodiscard]] bool worldLayoutRepairAssetCompatible(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogEntry& entry) noexcept {
  const cr::CreativeBoundsMetrics bounds =
      cr::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
  if (entry.category != cr::CreativeCatalogEntryCategory::Asset ||
      !entry.hotbarEntry.hasAssetBounds || !bounds.valid ||
      !cr::isPositiveCreativeVec3(bounds.size)) {
    return false;
  }
  if (issue.table == cr::CreativeWorldLayoutTable::Opening) {
    return issue.index < state.source.openings.size() &&
           creativeEditorWorldLayoutCatalogAssetMatchesOpening(
               entry.assetAuthoringMetadata.categoryId,
               state.source.openings[issue.index].kind);
  }
  if (issue.table == cr::CreativeWorldLayoutTable::Object) {
    return issue.index < state.source.objects.size() &&
           !creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
               entry.assetAuthoringMetadata.categoryId) &&
           entry.hotbarEntry.objectKind ==
               state.source.objects[issue.index].kind;
  }
  return false;
}

void queueWorldLayoutAssetRepair(
    CreativeDesktopCommandFrame& commands,
    const CreativeEditorWorldLayoutDiagnostic& issue,
    CreativeDesktopWorldLayoutAssetRepairOperation operation,
    std::string replacementAssetId = {}) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutRepairAsset,
      CreativeDesktopWorldLayoutAssetRepairPayload{
          operation, issue.table, issue.index, issue.stableKey,
          issue.assetId, std::move(replacementAssetId)});
}

void drawWorldLayoutAssetRepair(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands, bool editingDisabled,
    std::size_t issueIndex) {
  if (issue.assetIssue == CreativeEditorWorldLayoutAssetIssue::None) {
    return;
  }

  ImGui::PushID(static_cast<int>(issueIndex));
  ImGui::Indent();
  ImGui::BeginDisabled(editingDisabled);
  if (ImGui::SmallButton("Repair...")) {
    ImGui::OpenPopup("Asset repair");
  }
  ImGui::EndDisabled();
  ImGui::Unindent();

  if (ImGui::BeginPopup("Asset repair")) {
    ImGui::TextDisabled("Current: %s", issue.assetId.c_str());
    if (issue.assetIssue ==
        CreativeEditorWorldLayoutAssetIssue::StaleBounds) {
      if (ImGui::Button("Refresh Bounds")) {
        queueWorldLayoutAssetRepair(
            commands, issue,
            CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Adopt current catalog dimensions; preserve placement and fit");
      }
    }
    if (issue.table == cr::CreativeWorldLayoutTable::Opening) {
      if (issue.assetIssue ==
          CreativeEditorWorldLayoutAssetIssue::StaleBounds) {
        ImGui::SameLine();
      }
      if (ImGui::Button("Use Procedural")) {
        queueWorldLayoutAssetRepair(
            commands, issue,
            CreativeDesktopWorldLayoutAssetRepairOperation::
                UseProceduralInsert);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Replace the catalog insert; preserve the opening");
      }
    }

    std::size_t compatibleCount = 0U;
    for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
      if (worldLayoutRepairAssetCompatible(issue, state, entry) &&
          cr::creativeHotbarAssetId(entry.hotbarEntry) != issue.assetId) {
        ++compatibleCount;
      }
    }
    ImGui::BeginDisabled(compatibleCount == 0U);
    if (ImGui::BeginCombo("Replace Asset", "Choose compatible asset")) {
      for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
        const std::string_view assetId =
            cr::creativeHotbarAssetId(entry.hotbarEntry);
        if (!worldLayoutRepairAssetCompatible(issue, state, entry) ||
            assetId == issue.assetId) {
          continue;
        }
        ImGui::PushID(entry.hotbarEntry.assetId.data());
        if (ImGui::Selectable(entry.label.c_str())) {
          queueWorldLayoutAssetRepair(
              commands, issue,
              CreativeDesktopWorldLayoutAssetRepairOperation::ReplaceAsset,
              std::string(assetId));
          ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemHovered()) {
          const cr::CreativeBoundsMetrics bounds =
              cr::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
          ImGui::SetTooltip("%s\n%.2f x %.2f x %.2f m", assetId.data(),
                            bounds.size.x, bounds.size.y, bounds.size.z);
        }
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    if (compatibleCount == 0U) {
      ImGui::TextDisabled("No compatible replacement assets");
    }
    ImGui::EndPopup();
  }
  ImGui::PopID();
}


}  // namespace

void buildCreativeEditorWorldLayoutPanel(
    CreativeEditorDesktopUiState& desktopUi, CreativeEditorState& editor,
    const cr::CreativeDocument& document, bool playModeActive,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  if (!desktopUi.showWorldLayout) {
    queueLayoutManipulationCancel(state, commands);
    return;
  }

  const bool editingDisabled = playModeActive || editor.assetEdit.active;
  const CreativeEditorWorldLayoutDiagnosticReport& diagnostics =
      refreshCreativeEditorWorldLayoutDiagnostics(
          state.diagnosticCache, document, state.source, state.revision,
          &editor.catalog.model);
  if (editingDisabled) {
    queueLayoutManipulationCancel(state, commands);
  }
  if (state.buildingTransform.active && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    commands.push(CreativeDesktopCommandId::WorldLayoutTransformBuilding,
                  CreativeDesktopWorldLayoutBuildingTransformPayload{
                      CreativeEditorWorldLayoutBuildingTransformPhase::Cancel,
                      state.buildingTransform.operation});
  }

  // The World Layout workspace reuses the shell's existing dock identities.
  // The ### suffix keeps the persisted ImGui IDs stable while presenting
  // task-specific titles instead of leaving unrelated panels beside a
  // crowded center canvas.
  if (ImGui::Begin("World Layout Tools###Project", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    if (ImGui::BeginTabBar("##world_layout_left_tabs")) {
      if (ImGui::BeginTabItem("Source")) {
        drawCreativeEditorWorldLayoutHierarchy(desktopUi, state, commands,
                                               editingDisabled);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Create")) {
        ImGui::BeginDisabled(
            editingDisabled || state.buildingTransform.active ||
            state.buildingTemplatePlacement.active);
        drawWorldLayoutPalette(state, commands);
        ImGui::Spacing();
        drawWorldLayoutAssetPalette(state, editor.catalog.model, commands);
        ImGui::Spacing();
        drawWorldLayoutLevels(desktopUi, state, commands);
        ImGui::EndDisabled();
        drawWorldLayoutLevelDeleteModal(desktopUi, state, commands);
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }
  ImGui::End();

  if (ImGui::Begin("World Layout Properties###Inspector", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    ImGui::BeginDisabled(editingDisabled);
    drawCreativeEditorWorldLayoutSourceInspector(state, commands);
    drawCreativeEditorWorldLayoutStructureInspector(state, commands);
    drawSelectedRoomSettings(state, commands);
    drawSelectedOpeningSettings(state, editor.catalog.model, commands);
    if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::None) {
      ImGui::TextDisabled("Select a World Layout source to edit it.");
    }
    ImGui::EndDisabled();
  }
  ImGui::End();

  const bool exactPreviewActive =
      creativeEditorWorldLayoutPreviewActive(state);
  const bool hasSelection =
      state.selection.kind != CreativeEditorWorldLayoutSelectionKind::None;
  const bool buildingSelected =
      state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Building;
  if (ImGui::Begin("World Layout Build###Diagnostics##bottom", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    ImGui::BeginDisabled(editingDisabled || state.buildingTransform.active ||
                         state.buildingTemplatePlacement.active);
    ImGui::BeginDisabled(!diagnostics.ready);
    if (ImGui::Button(exactPreviewActive ? "Refresh 3D Preview"
                                         : "Preview 3D")) {
      commands.push(CreativeDesktopCommandId::WorldLayoutPreview);
    }
    ImGui::SameLine();
    if (ImGui::Button(exactPreviewActive ? "Confirm Preview"
                                         : "Confirm & Generate")) {
      commands.push(CreativeDesktopCommandId::WorldLayoutConfirm);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button(buildingSelected ? "Delete building" : "Delete")) {
      if (buildingSelected) {
        ImGui::OpenPopup("Delete building group");
      } else {
        commands.push(CreativeDesktopCommandId::WorldLayoutDeleteSelection);
      }
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    if (ImGui::BeginPopupModal("Delete building group", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      const char* buildingName =
          buildingSelected &&
                  state.selection.index < state.source.buildings.size()
              ? state.source.buildings[state.selection.index].name.c_str()
              : "selected building";
      ImGui::Text("Delete %s and all owned layout symbols?", buildingName);
      if (ImGui::Button("Delete building")) {
        commands.push(CreativeDesktopCommandId::WorldLayoutDeleteSelection);
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::TextColored(
        diagnostics.ready ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                          : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
        "%s", diagnostics.ready ? "READY" : "BLOCKED");
    ImGui::SameLine();
    if (diagnostics.ready) {
      ImGui::TextDisabled("%s", diagnostics.hasChanges
                                    ? "Changes are ready to generate"
                                    : "Generated output already matches");
      const cr::CreativeWorldLayoutReceipt& generation =
          diagnostics.compileReceipt;
      ImGui::TextDisabled(
          "Recipe groups: +%llu  replace %llu  keep %llu  |  remove %llu objects",
          static_cast<unsigned long long>(
              generation.objectRecipeCreateCount),
          static_cast<unsigned long long>(
              generation.objectRecipeReplaceCount),
          static_cast<unsigned long long>(generation.objectRecipeKeepCount),
          static_cast<unsigned long long>(generation.objectRemoveCount));
    }
    for (std::size_t issueIndex = 0U;
         issueIndex < diagnostics.issueCount; ++issueIndex) {
      const CreativeEditorWorldLayoutDiagnostic& issue =
          diagnostics.issues[issueIndex];
      const bool navigable =
          issue.table != cr::CreativeWorldLayoutTable::None &&
          issue.index != cr::kInvalidCreativeWorldLayoutIndex;
      const ImVec4 issueColor =
          issue.severity == CreativeEditorWorldLayoutDiagnosticSeverity::Warning
              ? ImVec4{1.0F, 0.72F, 0.20F, 1.0F}
              : issue.severity ==
                        CreativeEditorWorldLayoutDiagnosticSeverity::Info
                    ? ImVec4{0.38F, 0.72F, 1.0F, 1.0F}
                    : ImVec4{1.0F, 0.34F, 0.30F, 1.0F};
      const std::string label = issue.message + "##world_layout_issue_" +
                                std::to_string(issueIndex);
      ImGui::PushStyleColor(ImGuiCol_Text, issueColor);
      if (navigable) {
        if (ImGui::Selectable(label.c_str(), false,
                              ImGuiSelectableFlags_None,
                              ImVec2(0.0F, ImGui::GetFrameHeight()))) {
          commands.push(
              CreativeDesktopCommandId::WorldLayoutFocusSource,
              CreativeDesktopWorldLayoutSourcePayload{issue.table,
                                                       issue.index, {}});
        }
      } else {
        ImGui::TextUnformatted(issue.message.c_str());
      }
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
        if (!issue.kernelReasonCode.empty() &&
            issue.kernelReasonCode !=
                "creative_world_layout_kernel_not_requested") {
          ImGui::SetTooltip("%s\n%s", issue.reasonCode.c_str(),
                            issue.kernelReasonCode.c_str());
        } else {
          ImGui::SetTooltip("%s", issue.reasonCode.c_str());
        }
      }
      drawWorldLayoutAssetRepair(issue, state, editor.catalog.model, commands,
                                 editingDisabled, issueIndex);
    }

    ImGui::Separator();
    if (ImGui::BeginTable("##world_layout_counts", 4,
                          ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_BordersInnerV)) {
      ImGui::TableNextColumn();
      ImGui::Text("Buildings  %llu", static_cast<unsigned long long>(
                                        state.source.buildings.size()));
      ImGui::TableNextColumn();
      ImGui::Text("Levels  %llu", static_cast<unsigned long long>(
                                     state.source.levels.size()));
      ImGui::TableNextColumn();
      ImGui::Text("Rooms  %llu", static_cast<unsigned long long>(
                                    state.source.rooms.size()));
      ImGui::TableNextColumn();
      ImGui::Text("Floors  %llu", static_cast<unsigned long long>(
                                     state.source.boxes.size()));
      ImGui::TableNextColumn();
      ImGui::Text("Partitions  %llu", static_cast<unsigned long long>(
                                         state.source.walls.size()));
      ImGui::TableNextColumn();
      ImGui::Text("Openings  %llu", static_cast<unsigned long long>(
                                       state.source.openings.size()));
      ImGui::TableNextColumn();
      ImGui::Text(
          "Terrain  %llu",
          static_cast<unsigned long long>(
              state.source.terrainProfiles.size() +
              state.source.terrainPaths.size()));
      ImGui::TableNextColumn();
      ImGui::Text("Objects  %llu", static_cast<unsigned long long>(
                                      state.source.objects.size()));
      ImGui::EndTable();
    }

    ImGui::TextDisabled("Revision %llu%s",
                        static_cast<unsigned long long>(state.revision),
                        creativeEditorWorldLayoutDirty(state) ? " *" : "");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4{0.32F, 0.95F, 0.43F, 1.0F}, "%s",
                       state.statusMessage.c_str());
  }
  ImGui::End();

  if (ImGui::Begin("World Layout", &desktopUi.showWorldLayout,
                   ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse)) {
    drawWorldLayoutViewControls(state, commands);
    ImGui::Separator();
    ImGui::BeginDisabled(editingDisabled);
    const bool canvasInteractionEnabled =
        !editingDisabled && !state.buildingTransform.active;
    if (state.viewMode == CreativeEditorWorldLayoutViewMode::Elevation) {
      drawCreativeEditorWorldLayoutElevationCanvas(
          editor, document.gridSettings(), commands,
          canvasInteractionEnabled);
    } else {
      drawLayoutCanvas(editor, document.gridSettings(), commands,
                       canvasInteractionEnabled);
    }
    ImGui::EndDisabled();
  } else {
    queueLayoutManipulationCancel(state, commands);
  }
  ImGui::End();
}

}  // namespace iggy3d_creative_app
