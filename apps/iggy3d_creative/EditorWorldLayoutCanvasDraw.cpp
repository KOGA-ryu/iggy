#include "EditorWorldLayoutCanvasInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorMeasurement.hpp"
#include "EditorWorldLayoutInternal.hpp"
#include "EditorWorldLayoutPlanDraw.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

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

ImU32 color(ImVec4 value) { return ImGui::ColorConvertFloat4ToU32(value); }

ImU32 draftingColor(CreativeEditorDraftingColor value) {
  return IM_COL32(value.r, value.g, value.b, value.a);
}

ImVec4 generatedScopeTint(cr::CreativeWorldLayoutTable table) {
  const CreativeDesktopGeneratedSourceScopeTint tint =
      creativeDesktopGeneratedSourceScopeTint(table);
  return {tint.r, tint.g, tint.b, tint.a};
}

bool selected(const CreativeEditorWorldLayoutState& state,
              CreativeEditorWorldLayoutSelectionKind kind, std::size_t index) {
  return state.selection.kind == kind && state.selection.index == index;
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

void drawLevelSelection(ImDrawList& drawList,
                        const CanvasTransform& transform,
                        const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (state.buildingTemplatePlacement.active ||
      state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::Level ||
      state.selection.index >= source.levels.size()) {
    return;
  }

  bool haveBounds = false;
  double minimumX = 0.0;
  double minimumZ = 0.0;
  double maximumX = 0.0;
  double maximumZ = 0.0;
  const cr::CreativeWorldLayoutLevel& level =
      source.levels[state.selection.index];
  for (const cr::CreativeWorldLayoutRoom& room : source.rooms) {
    if (room.levelIndex != state.selection.index ||
        room.buildingIndex != level.buildingIndex) {
      continue;
    }
    const auto [deltaX, deltaZ] =
        buildingPreviewOffset(state, room.buildingIndex);
    const double roomMinimumX = room.footprint.minimum.x + deltaX;
    const double roomMinimumZ = room.footprint.minimum.z + deltaZ;
    const double roomMaximumX = room.footprint.maximum.x + deltaX;
    const double roomMaximumZ = room.footprint.maximum.z + deltaZ;
    if (!haveBounds) {
      minimumX = roomMinimumX;
      minimumZ = roomMinimumZ;
      maximumX = roomMaximumX;
      maximumZ = roomMaximumZ;
      haveBounds = true;
      continue;
    }
    minimumX = std::min(minimumX, roomMinimumX);
    minimumZ = std::min(minimumZ, roomMinimumZ);
    maximumX = std::max(maximumX, roomMaximumX);
    maximumZ = std::max(maximumZ, roomMaximumZ);
  }
  if (!haveBounds) {
    return;
  }

  const ImVec2 minimum = toScreen(transform, minimumX, minimumZ);
  const ImVec2 maximum = toScreen(transform, maximumX, maximumZ);
  const ImVec4 tint =
      generatedScopeTint(cr::CreativeWorldLayoutTable::Level);
  ImVec4 fill = tint;
  fill.w = 0.05F;
  drawList.AddRectFilled(minimum, maximum, color(fill));
  drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0, 3.0F);
  drawList.AddText({minimum.x + 8.0F, minimum.y + 7.0F}, color(tint),
                   level.name.c_str());
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
      !detail::worldLayoutRoomOnActiveLevel(state, state.source,
                                            state.selection.index) ||
      !state.source.roomBoundaries.empty()) {
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

void drawRoomBoundaryManipulation(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Room ||
      state.source.roomBoundaries.empty()) {
    return;
  }
  const cr::CreativeWorldLayout& displayed =
      creativeEditorWorldLayoutDisplaySource(state);
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(displayed);
  if (!graph.accepted || state.selection.index >= graph.rooms.size()) {
    return;
  }

  std::size_t activeEdge = cr::kInvalidCreativeWorldLayoutIndex;
  if (state.roomBoundaryManipulation.active &&
      state.roomBoundaryManipulation.target.roomIndex ==
          state.selection.index) {
    activeEdge = state.roomBoundaryManipulation.target.topologyEdgeIndex;
    if (state.roomBoundaryManipulation.previewValid &&
        state.roomBoundaryManipulation.previewEdit.changed &&
        activeEdge < state.roomBoundaryManipulation.previewEdit
                         .sourceToEditedEdgeIndices.size()) {
      activeEdge = state.roomBoundaryManipulation.previewEdit
                       .sourceToEditedEdgeIndices[activeEdge];
    }
  }
  std::size_t activeVertex = cr::kInvalidCreativeWorldLayoutIndex;
  if (state.roomCornerManipulation.active &&
      state.roomCornerManipulation.target.roomIndex ==
          state.selection.index) {
    activeVertex =
        state.roomCornerManipulation.target.topologyVertexIndex;
    if (state.roomCornerManipulation.previewValid &&
        state.roomCornerManipulation.previewEdit.changed &&
        activeVertex < state.roomCornerManipulation.previewEdit
                           .sourceToEditedVertexIndices.size()) {
      activeVertex = state.roomCornerManipulation.previewEdit
                         .sourceToEditedVertexIndices[activeVertex];
    }
  }

  const ImU32 normalColor = color({0.96F, 0.82F, 0.22F, 0.90F});
  const ImU32 boundaryActiveColor =
      state.roomBoundaryManipulation.previewValid
          ? color({0.20F, 0.78F, 0.38F, 1.0F})
          : color({0.92F, 0.29F, 0.24F, 1.0F});
  const ImU32 cornerActiveColor =
      state.roomCornerManipulation.previewValid
          ? color({0.20F, 0.78F, 0.38F, 1.0F})
          : color({0.92F, 0.29F, 0.24F, 1.0F});
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       cr::creativeWorldLayoutRoomBoundaries(graph, state.selection.index)) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    const cr::CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const cr::CreativeTerrainCoord2 end =
        graph.vertices[edge.endVertexIndex].position;
    const ImVec2 startScreen = toScreen(transform, start.x, start.z);
    const ImVec2 endScreen = toScreen(transform, end.x, end.z);
    const ImU32 tint = boundary.topologyEdgeIndex == activeEdge
                           ? boundaryActiveColor
                           : normalColor;
    drawList.AddLine(startScreen, endScreen, tint,
                     boundary.topologyEdgeIndex == activeEdge ? 4.0F : 2.5F);
    for (const auto [vertexIndex, vertex] :
         {std::pair{edge.startVertexIndex, startScreen},
          std::pair{edge.endVertexIndex, endScreen}}) {
      const ImU32 vertexTint =
          vertexIndex == activeVertex ? cornerActiveColor : tint;
      drawList.AddRectFilled({vertex.x - 3.0F, vertex.y - 3.0F},
                             {vertex.x + 3.0F, vertex.y + 3.0F}, vertexTint);
    }
  }

  if (state.roomBoundaryManipulation.active &&
      !state.roomBoundaryManipulation.previewValid) {
    const cr::CreativeWorldLayoutRoomGraph sourceGraph =
        cr::buildCreativeWorldLayoutRoomGraph(state.source);
    const std::size_t sourceEdgeIndex =
        state.roomBoundaryManipulation.target.topologyEdgeIndex;
    if (sourceGraph.accepted && sourceEdgeIndex < sourceGraph.edges.size()) {
      const cr::CreativeWorldLayoutTopologyEdge& edge =
          sourceGraph.edges[sourceEdgeIndex];
      cr::CreativeTerrainCoord2 start =
          sourceGraph.vertices[edge.startVertexIndex].position;
      cr::CreativeTerrainCoord2 end =
          sourceGraph.vertices[edge.endVertexIndex].position;
      if (state.roomBoundaryManipulation.target.horizontal) {
        start.z = state.roomBoundaryManipulation.previewCoordinate;
        end.z = state.roomBoundaryManipulation.previewCoordinate;
      } else {
        start.x = state.roomBoundaryManipulation.previewCoordinate;
        end.x = state.roomBoundaryManipulation.previewCoordinate;
      }
      drawList.AddLine(toScreen(transform, start.x, start.z),
                       toScreen(transform, end.x, end.z), boundaryActiveColor,
                       4.0F);
    }
  }

  if (state.roomCornerManipulation.active &&
      !state.roomCornerManipulation.previewValid) {
    const cr::CreativeWorldLayoutRoomGraph sourceGraph =
        cr::buildCreativeWorldLayoutRoomGraph(state.source);
    const auto& target = state.roomCornerManipulation.target;
    if (sourceGraph.accepted && target.roomIndex < sourceGraph.rooms.size() &&
        target.topologyVertexIndex < sourceGraph.vertices.size()) {
      const ImVec2 attempted = toScreen(
          transform, state.roomCornerManipulation.previewPosition.x,
          state.roomCornerManipulation.previewPosition.z);
      for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
           cr::creativeWorldLayoutRoomBoundaries(sourceGraph,
                                                 target.roomIndex)) {
        const cr::CreativeWorldLayoutTopologyEdge& edge =
            sourceGraph.edges[boundary.topologyEdgeIndex];
        if (edge.startVertexIndex != target.topologyVertexIndex &&
            edge.endVertexIndex != target.topologyVertexIndex) {
          continue;
        }
        const std::size_t otherIndex =
            edge.startVertexIndex == target.topologyVertexIndex
                ? edge.endVertexIndex
                : edge.startVertexIndex;
        const cr::CreativeTerrainCoord2 other =
            sourceGraph.vertices[otherIndex].position;
        drawList.AddLine(attempted, toScreen(transform, other.x, other.z),
                         cornerActiveColor, 3.0F);
      }
      drawList.AddRectFilled({attempted.x - 4.0F, attempted.y - 4.0F},
                             {attempted.x + 4.0F, attempted.y + 4.0F},
                             cornerActiveColor);
    }
  }
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

void drawRoofApertureManipulation(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state) {
  if (state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::RoofAperture ||
      state.selection.index >= state.source.roofApertures.size()) {
    return;
  }

  const bool active = state.roofApertureManipulation.active &&
                      state.roofApertureManipulation.target.apertureIndex ==
                          state.selection.index;
  const cr::CreativeWorldLayoutRoofAperture& aperture =
      state.source.roofApertures[state.selection.index];
  const CreativeEditorWorldLayoutRoofApertureSettings settings =
      active ? state.roofApertureManipulation.previewSettings
             : CreativeEditorWorldLayoutRoofApertureSettings{
                   aperture.kind, aperture.minimumXCells,
                   aperture.maximumXCells, aperture.minimumZCells,
                   aperture.maximumZCells};
  const ImVec2 first =
      toScreen(transform, settings.minimumXCells, settings.minimumZCells);
  const ImVec2 second =
      toScreen(transform, settings.maximumXCells, settings.maximumZCells);
  const ImVec2 minimum{std::min(first.x, second.x),
                       std::min(first.y, second.y)};
  const ImVec2 maximum{std::max(first.x, second.x),
                       std::max(first.y, second.y)};
  const ImVec4 tint =
      active && !state.roofApertureManipulation.previewValid
          ? ImVec4{0.92F, 0.29F, 0.24F, 1.0F}
          : active ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
                   : ImVec4{0.96F, 0.82F, 0.22F, 1.0F};

  if (active) {
    ImVec4 fill = tint;
    fill.w = 0.24F;
    drawList.AddRectFilled(minimum, maximum, color(fill));
    drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0, 3.0F);
    char dimensions[64]{};
    std::snprintf(dimensions, sizeof(dimensions), "%.2f x %.2f",
                  std::fabs(settings.maximumXCells - settings.minimumXCells),
                  std::fabs(settings.maximumZCells - settings.minimumZCells));
    drawList.AddText({minimum.x + 7.0F, minimum.y + 7.0F}, color(tint),
                     dimensions);
  }

  const float centerX = (minimum.x + maximum.x) * 0.5F;
  const float centerY = (minimum.y + maximum.y) * 0.5F;
  const std::array<ImVec2, 8U> handles = {
      ImVec2{minimum.x, minimum.y}, ImVec2{centerX, minimum.y},
      ImVec2{maximum.x, minimum.y}, ImVec2{maximum.x, centerY},
      ImVec2{maximum.x, maximum.y}, ImVec2{centerX, maximum.y},
      ImVec2{minimum.x, maximum.y}, ImVec2{minimum.x, centerY},
  };
  for (const ImVec2 handle : handles) {
    drawList.AddRectFilled({handle.x - 4.0F, handle.y - 4.0F},
                           {handle.x + 4.0F, handle.y + 4.0F}, color(tint));
  }
}

void drawRoofManipulation(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid) {
  const std::size_t levelIndex =
      state.roofManipulation.active
          ? state.roofManipulation.target.levelIndex
          : state.selection.kind ==
                    CreativeEditorWorldLayoutSelectionKind::Level
                ? state.selection.index
                : cr::kInvalidCreativeWorldLayoutIndex;
  const CreativeEditorWorldLayoutRoofHandleFrame frame =
      buildCreativeEditorWorldLayoutRoofHandleFrame(state, grid, levelIndex);
  if (!frame.accepted) {
    return;
  }
  const bool active = state.roofManipulation.active;
  const ImVec4 tint =
      active && !state.roofManipulation.previewValid
          ? ImVec4{0.92F, 0.29F, 0.24F, 1.0F}
          : active ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
                   : ImVec4{0.96F, 0.82F, 0.22F, 1.0F};
  const ImU32 packed = color(tint);
  const auto planPoint = [&](cr::CreativeVec3 world) {
    return toScreen(
        transform, (world.x - grid.origin.x) / grid.cellSizeMeters,
        (world.z - grid.origin.z) / grid.cellSizeMeters);
  };
  for (std::size_t index = 0U;
       index < frame.roof.geometry.edgeCount; ++index) {
    const cr::CreativeStructuralRoofEdgePlan& edge =
        frame.roof.geometry.edges[index];
    drawList.AddLine(planPoint(edge.startMeters), planPoint(edge.endMeters),
                     packed, active ? 3.5F : 2.5F);
  }
  if (frame.roof.geometry.riseMeters > 0.0) {
    drawList.AddLine(planPoint(frame.roof.geometry.ridgeStart),
                     planPoint(frame.roof.geometry.ridgeEnd), packed, 1.5F);
  }
  for (std::size_t index = 0U; index < frame.handleCount; ++index) {
    const CreativeEditorWorldLayoutRoofHandle& handle = frame.handles[index];
    if (!handle.valid) {
      continue;
    }
    const ImVec2 point = toScreen(transform, handle.planPosition.x,
                                  handle.planPosition.z);
    if (handle.target.handle ==
        CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight) {
      drawList.AddQuadFilled({point.x, point.y - 5.0F},
                             {point.x + 5.0F, point.y},
                             {point.x, point.y + 5.0F},
                             {point.x - 5.0F, point.y}, packed);
    } else {
      drawList.AddCircleFilled(point, 5.0F, packed, 12);
    }
  }
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
             : generatedScopeTint(cr::CreativeWorldLayoutTable::Building);
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
    if (!detail::worldLayoutOpeningOnActiveLevel(state, source, index)) {
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
    if (!isSelected && !active && !hostWallActive) {
      continue;
    }
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
    if (active) {
      // The semantic wall is cut around the durable opening. During an
      // opening drag, restore the host line beneath the transient marker so
      // the old cut does not remain as a misleading second opening.
      drawList.AddLine(toScreen(transform, host.start.x, host.start.z),
                       toScreen(transform, host.end.x, host.end.z), markerColor,
                       3.0F);
    }
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

void drawMeasurementGeometry(
    ImDrawList& drawList, const CanvasTransform& transform,
    const cr::CreativeMeasurementGeometry& geometry,
    std::string_view label, bool transient) {
  if (!geometry.visible) {
    return;
  }
  const CreativeEditorDraftingStyle& style = creativeEditorDraftingStyle(
      CreativeEditorDraftingRole::MeasurementOverlay);
  CreativeEditorDraftingColor tintValue = style.tint;
  if (!transient) {
    tintValue.a = static_cast<std::uint8_t>(
        static_cast<float>(tintValue.a) * 0.72F);
  }
  const ImU32 tint = draftingColor(tintValue);
  const float thickness = creativeEditorDraftingStrokeThicknessPixels(
      style, transform.pixelsPerCell);
  for (std::size_t index = 0U; index < geometry.segmentCount; ++index) {
    const cr::CreativeMeasurementSegment& segment = geometry.segments[index];
    drawList.AddLine(toScreen(transform, segment.start.x, segment.start.z),
                     toScreen(transform, segment.end.x, segment.end.z), tint,
                     thickness);
  }
  for (std::size_t index = 0U; index < geometry.pointCount; ++index) {
    drawList.AddCircleFilled(
        toScreen(transform, geometry.points[index].x, geometry.points[index].z),
        index + 1U == geometry.pointCount ? 4.5F : 3.5F, tint);
  }
  if (!label.empty() && geometry.pointCount > 0U) {
    const cr::CreativeMeasurementPoint& finalPoint =
        geometry.points[geometry.pointCount - 1U];
    const ImVec2 anchor = toScreen(transform, finalPoint.x, finalPoint.z);
    drawList.AddText({anchor.x + 8.0F, anchor.y + 8.0F}, tint, label.data(),
                     label.data() + label.size());
  }
}

}  // namespace

CreativeEditorWorldLayoutCanvasPointerGeometry
drawCreativeEditorWorldLayoutCanvasScene(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorWorldLayoutPlanViewCache& planView,
    std::size_t hoveredPlanPrimitiveIndex,
    const cr::CreativeGridSettings& grid,
    const cr::CreativeMeasurementAnnotationStore& measurementAnnotations,
    const cr::CreativeMeasurementState& measurement,
    ImVec2 pointerPosition, bool hovered) {
  drawList.PushClipRect(minimum, maximum, true);
  drawList.AddRectFilled(minimum, maximum,
                         color({0.105F, 0.12F, 0.135F, 1.0F}));
  drawCreativeEditorWorldLayoutTerrainBackground(
      drawList, minimum, maximum, transform, topography);
  const cr::CreativeWorldLayout& displaySource =
      creativeEditorWorldLayoutDisplaySource(state);
  static_cast<void>(drawCreativeEditorWorldLayoutPlan(
      drawList, transform, state, displaySource, planView,
      hoveredPlanPrimitiveIndex));
  drawCreativeEditorWorldLayoutTerrainAnnotations(
      drawList, transform, topography, grid);
  const cr::CreativeWorldLayoutPlanProjection& projection =
      planView.projection;
  if (!projection.accepted) {
    const CreativeEditorDraftingStyle& invalidStyle =
        creativeEditorDraftingStyle(
            CreativeEditorDraftingRole::PreviewInvalidOverlay);
    constexpr std::string_view prefix = "Plan unavailable: ";
    drawList.AddText({minimum.x + 12.0F, minimum.y + 12.0F},
                     draftingColor(invalidStyle.tint), prefix.data(),
                     prefix.data() + prefix.size());
    drawList.AddText({minimum.x + 12.0F, minimum.y + 30.0F},
                     draftingColor(invalidStyle.tint),
                     projection.reasonCode.data(),
                     projection.reasonCode.data() +
                         projection.reasonCode.size());
  }
  for (std::size_t index = 0U; index < displaySource.verticalConnectors.size();
       ++index) {
    const bool selectedConnector =
        selected(state,
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
                 index);
    const bool activeConnector = state.verticalConnectorManipulation.active &&
                                 state.verticalConnectorManipulation.target
                                         .connectorIndex == index;
    if (selectedConnector || activeConnector) {
      drawVerticalConnector(drawList, transform, state, index);
    }
  }
  for (std::size_t index = 0U; index < displaySource.walls.size(); ++index) {
    const bool selectedWall =
        selected(state, CreativeEditorWorldLayoutSelectionKind::Wall, index);
    const bool activeWall = state.wallManipulation.active &&
                            state.wallManipulation.target.wallIndex == index;
    if (selectedWall || activeWall) {
      drawWall(drawList, transform, state, index);
    }
  }
  drawOpenings(drawList, transform, state);
  drawCreativeEditorWorldLayoutObjectSymbols(drawList, transform, state, grid);
  drawLevelSelection(drawList, transform, state);
  drawBuildingSelection(drawList, transform, state);
  drawBuildingTemplatePlacement(drawList, transform, state);
  drawRoomManipulation(drawList, transform, state);
  drawRoomBoundaryManipulation(drawList, transform, state);
  drawBoxManipulation(drawList, transform, state);
  drawRoofManipulation(drawList, transform, state, grid);
  drawRoofApertureManipulation(drawList, transform, state);
  for (const cr::CreativeMeasurementAnnotation& annotation :
       measurementAnnotations.annotations) {
    drawMeasurementGeometry(
        drawList, transform,
        projectCreativeEditorMeasurementGeometryToGrid(
            cr::buildCreativeMeasurementGeometry(annotation), grid),
        annotation.name, false);
  }
  const cr::CreativeMeasurementGeometry transientMeasurement =
      projectCreativeEditorMeasurementGeometryToGrid(
          cr::buildCreativeMeasurementGeometry(measurement), grid);
  drawMeasurementGeometry(
      drawList, transform, transientMeasurement,
      transientMeasurement.visible
          ? formatCreativeEditorMeasurementReadout(measurement)
          : std::string{},
      true);

  CreativeEditorWorldLayoutCanvasPointerGeometry geometry;
  geometry.pointerPoint = toWorld(transform, pointerPosition);
  const ImVec2 boundedPointer{
      std::clamp(pointerPosition.x, minimum.x, maximum.x),
      std::clamp(pointerPosition.y, minimum.y, maximum.y)};
  geometry.hoveredPoint = toWorld(transform, boundedPointer);
  geometry.handleToleranceCells = std::clamp(
      8.0 / static_cast<double>(transform.pixelsPerCell), 0.10, 0.45);
  if (hovered && !state.buildingTemplatePlacement.active) {
    drawCreativeEditorWorldLayoutPlacementPreviews(
        drawList, transform, state, geometry.hoveredPoint, grid);
  }
  drawList.PopClipRect();

  if (hovered) {
    drawCreativeEditorWorldLayoutTopographyHoverFacts(
        topography, geometry.pointerPoint, grid);
  }
  return geometry;
}

}  // namespace iggy3d_creative_app
