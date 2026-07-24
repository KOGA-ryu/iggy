#include "EditorWorldLayoutPanelInternal.hpp"

#include "EditorDesktopWidgets.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutWallOperations.hpp"

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

std::vector<std::size_t> adjacentRoomIndices(
    const cr::CreativeWorldLayout& layout, std::size_t roomIndex) {
  std::vector<std::size_t> adjacent;
  for (const cr::CreativeWorldLayoutSharedRoomEdgeSpan& shared :
       cr::inspectCreativeWorldLayoutSharedRoomEdges(layout)) {
    std::size_t other = cr::kInvalidCreativeWorldLayoutIndex;
    if (shared.firstRoomIndex == roomIndex) {
      other = shared.secondRoomIndex;
    } else if (shared.secondRoomIndex == roomIndex) {
      other = shared.firstRoomIndex;
    }
    if (other < layout.rooms.size() &&
        std::find(adjacent.begin(), adjacent.end(), other) == adjacent.end()) {
      adjacent.push_back(other);
    }
  }
  return adjacent;
}

std::int32_t roomSplitMidpoint(
    cr::CreativeWorldLayoutRect bounds,
    cr::CreativeWorldLayoutRoomSplitAxis axis) noexcept {
  const std::int32_t minimum =
      axis == cr::CreativeWorldLayoutRoomSplitAxis::X ? bounds.minimum.x
                                                      : bounds.minimum.z;
  const std::int32_t maximum =
      axis == cr::CreativeWorldLayoutRoomSplitAxis::X ? bounds.maximum.x
                                                      : bounds.maximum.z;
  return static_cast<std::int32_t>(
      static_cast<std::int64_t>(minimum) +
      (static_cast<std::int64_t>(maximum) - minimum) / 2);
}

std::uint32_t roomTopologyEdgeLength(
    const cr::CreativeWorldLayoutRoomGraph& graph,
    std::size_t edgeIndex) noexcept {
  if (edgeIndex >= graph.edges.size()) {
    return 0U;
  }
  const cr::CreativeWorldLayoutTopologyEdge& edge = graph.edges[edgeIndex];
  const cr::CreativeTerrainCoord2 start =
      graph.vertices[edge.startVertexIndex].position;
  const cr::CreativeTerrainCoord2 end =
      graph.vertices[edge.endVertexIndex].position;
  const std::int64_t length = start.x == end.x
                                  ? static_cast<std::int64_t>(end.z) - start.z
                                  : static_cast<std::int64_t>(end.x) - start.x;
  return static_cast<std::uint32_t>(length);
}

std::string topologyEdgeLabel(
    const cr::CreativeWorldLayoutRoomGraph& graph,
    std::size_t topologyEdgeIndex) {
  const cr::CreativeWorldLayoutTopologyEdge& edge =
      graph.edges[topologyEdgeIndex];
  const cr::CreativeTerrainCoord2 start =
      graph.vertices[edge.startVertexIndex].position;
  const cr::CreativeTerrainCoord2 end =
      graph.vertices[edge.endVertexIndex].position;
  return "Wall  (" +
         std::to_string(start.x) + ", " + std::to_string(start.z) + ") to (" +
         std::to_string(end.x) + ", " + std::to_string(end.z) + ")";
}

void drawSelectedRoomTopology(CreativeEditorWorldLayoutState& state,
                              std::size_t roomIndex,
                              CreativeDesktopCommandFrame& commands) {
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  if (!graph.accepted || roomIndex >= graph.rooms.size()) {
    ImGui::SeparatorText("Floor plan");
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "Room topology is invalid");
    return;
  }

  const std::vector<std::size_t> adjacent =
      adjacentRoomIndices(state.source, roomIndex);
  CreativeEditorWorldLayoutRoomTopologyDraft& draft =
      state.roomTopologyDraft;
  if (draft.roomIndex != roomIndex ||
      draft.sourceRevision != state.revision) {
    draft = {};
    draft.roomIndex = roomIndex;
    draft.sourceRevision = state.revision;
    draft.splitAxis = cr::CreativeWorldLayoutRoomSplitAxis::X;
    draft.splitCoordinate =
        roomSplitMidpoint(graph.roomBounds[roomIndex], draft.splitAxis);
    draft.mergeRoomIndex =
        adjacent.empty() ? cr::kInvalidCreativeWorldLayoutIndex
                         : adjacent.front();
  }

  std::int64_t areaCells = 0;
  for (const cr::CreativeWorldLayoutRect surface :
       cr::creativeWorldLayoutRoomSurfaceRects(graph, roomIndex)) {
    areaCells +=
        (static_cast<std::int64_t>(surface.maximum.x) - surface.minimum.x) *
        (static_cast<std::int64_t>(surface.maximum.z) - surface.minimum.z);
  }

  ImGui::SeparatorText("Floor plan");
  ImGui::TextDisabled("%zu edges | %lld cells | %zu adjacent",
                      graph.rooms[roomIndex].boundaryCount,
                      static_cast<long long>(areaCells), adjacent.size());
  for (const std::size_t adjacentRoomIndex : adjacent) {
    ImGui::BulletText("Adjacent: %s",
                      state.source.rooms[adjacentRoomIndex].name.c_str());
  }

  ImGui::SeparatorText("Room operations");

  int axis = static_cast<int>(draft.splitAxis);
  if (ImGui::RadioButton("Split X", axis == 0)) {
    draft.splitAxis = cr::CreativeWorldLayoutRoomSplitAxis::X;
    draft.splitCoordinate =
        roomSplitMidpoint(graph.roomBounds[roomIndex], draft.splitAxis);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Split Z", axis == 1)) {
    draft.splitAxis = cr::CreativeWorldLayoutRoomSplitAxis::Z;
    draft.splitCoordinate =
        roomSplitMidpoint(graph.roomBounds[roomIndex], draft.splitAxis);
  }
  int splitCoordinate = draft.splitCoordinate;
  ImGui::SetNextItemWidth(128.0F);
  if (ImGui::InputInt("Grid line##room_split", &splitCoordinate)) {
    draft.splitCoordinate = splitCoordinate;
  }
  const bool roomEditActive = state.roomManipulation.active ||
                              state.roomCornerManipulation.active ||
                              state.roomBoundaryManipulation.active;
  ImGui::BeginDisabled(roomEditActive);
  if (ImGui::Button("Split room")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutSplitRoom,
        CreativeDesktopWorldLayoutRoomSplitPayload{
            roomIndex, draft.splitAxis, draft.splitCoordinate});
  }
  ImGui::EndDisabled();

  if (std::find(adjacent.begin(), adjacent.end(), draft.mergeRoomIndex) ==
      adjacent.end()) {
    draft.mergeRoomIndex =
        adjacent.empty() ? cr::kInvalidCreativeWorldLayoutIndex
                         : adjacent.front();
  }
  const char* mergePreview = "No adjacent room";
  if (draft.mergeRoomIndex < state.source.rooms.size()) {
    mergePreview = state.source.rooms[draft.mergeRoomIndex].name.c_str();
  }
  ImGui::SetNextItemWidth(180.0F);
  ImGui::BeginDisabled(adjacent.empty() || roomEditActive);
  if (ImGui::BeginCombo("Merge with##room_merge", mergePreview)) {
    for (const std::size_t adjacentRoomIndex : adjacent) {
      const bool selected = adjacentRoomIndex == draft.mergeRoomIndex;
      const std::string label =
          state.source.rooms[adjacentRoomIndex].name + "##room_merge_" +
          std::to_string(adjacentRoomIndex);
      if (ImGui::Selectable(label.c_str(), selected)) {
        draft.mergeRoomIndex = adjacentRoomIndex;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::Button("Merge rooms")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutMergeRooms,
        CreativeDesktopWorldLayoutRoomMergePayload{
            roomIndex, draft.mergeRoomIndex});
  }
  ImGui::EndDisabled();
}

void drawSelectedRoomSettings(CreativeEditorWorldLayoutState& state,
                              CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Room ||
      state.selection.index >= state.source.rooms.size()) {
    state.roomSettingsDraft = {};
    state.roomMetadataDraft = {};
    state.roomTopologyDraft = {};
    return;
  }

  const std::size_t roomIndex = state.selection.index;
  CreativeEditorWorldLayoutRoomSettings current;
  CreativeEditorWorldLayoutRoomMetadata currentMetadata;
  if (!readCreativeEditorWorldLayoutRoomSettings(state, roomIndex, current)) {
    state.roomSettingsDraft = {};
    state.roomTopologyDraft = {};
    ImGui::TextDisabled("Room level is unavailable");
    return;
  }
  if (!readCreativeEditorWorldLayoutRoomMetadata(state, roomIndex,
                                                 currentMetadata)) {
    state.roomMetadataDraft = {};
    return;
  }
  if (!state.roomSettingsDraft.active ||
      state.roomSettingsDraft.roomIndex != roomIndex ||
      state.roomSettingsDraft.sourceRevision != state.revision) {
    state.roomSettingsDraft = {true, roomIndex, state.revision, current};
  }
  if (!state.roomMetadataDraft.active ||
      state.roomMetadataDraft.roomIndex != roomIndex ||
      state.roomMetadataDraft.sourceRevision != state.revision) {
    state.roomMetadataDraft =
        {true, roomIndex, state.revision, currentMetadata};
  }

  const cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  CreativeEditorWorldLayoutRoomMetadata& metadata =
      state.roomMetadataDraft.metadata;
  CreativeDesktopPropertyEditActivity metadataActivity;
  ImGui::SeparatorText("Room");
  ImGui::SetNextItemWidth(220.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      metadataActivity,
      creativeDesktopInputTextStdString("Name##room", &metadata.name));
  constexpr std::array<const char*, 8U> kRoomTypeLabels = {
      "Generic", "Living",  "Kitchen", "Bedroom",
      "Bathroom", "Corridor", "Storage", "Utility"};
  int roomType = static_cast<int>(metadata.type);
  ImGui::SetNextItemWidth(160.0F);
  if (ImGui::Combo("Type##room", &roomType, kRoomTypeLabels.data(),
                   static_cast<int>(kRoomTypeLabels.size()))) {
    metadata.type = static_cast<cr::CreativeWorldLayoutRoomType>(roomType);
    observeCreativeDesktopDiscretePropertyWidget(metadataActivity, true);
  }
  const bool metadataValid =
      detail::hasVisibleWorldLayoutName(metadata.name) &&
      metadata.name.size() <= 127U &&
      metadata.type < cr::CreativeWorldLayoutRoomType::Count;
  if (!metadataValid) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "room needs a visible name and valid type");
  }
  finishCreativeDesktopWorldLayoutPropertyEdit(
      metadataActivity, currentMetadata, metadata, metadataValid,
      "Reset identity", state, roomIndex, room.stableKey, commands);

  drawSelectedRoomTopology(state, roomIndex, commands);
  CreativeEditorWorldLayoutRoomSettings& settings =
      state.roomSettingsDraft.settings;
  const bool explicitTopology = !state.source.roomBoundaries.empty();
  const std::size_t levelIndex = room.levelIndex;
  ImGui::SeparatorText("Level");
  if (levelIndex < state.source.levels.size()) {
    const cr::CreativeWorldLayoutLevel& level = state.source.levels[levelIndex];
    ImGui::TextUnformatted(level.name.c_str());
    ImGui::TextDisabled("Floor %.3f | ceiling %.3f grid layers",
                        level.floorTopLayer,
                        level.floorTopLayer +
                            static_cast<double>(level.wallHeightCells));
    ImGui::TextDisabled("Floor slab %u | ceiling %u | roof %u layers",
                        level.floorThicknessLayers,
                        level.ceilingThicknessLayers,
                        level.roofThicknessLayers);
    if (ImGui::Button("Edit level settings")) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutLevelOperation,
          CreativeDesktopWorldLayoutLevelOperationPayload{
              CreativeEditorWorldLayoutLevelOperation::Select,
              level.buildingIndex, levelIndex});
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
          "Level settings own storey height, slabs, ceiling, and roof");
    }
  } else {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "Room level is unavailable");
  }

  const std::int64_t widthCells =
      static_cast<std::int64_t>(settings.footprint.maximum.x) -
      settings.footprint.minimum.x;
  const std::int64_t depthCells =
      static_cast<std::int64_t>(settings.footprint.maximum.z) -
      settings.footprint.minimum.z;
  int width = static_cast<int>(std::min<std::int64_t>(
      widthCells, std::numeric_limits<int>::max()));
  int depth = static_cast<int>(std::min<std::int64_t>(
      depthCells, std::numeric_limits<int>::max()));
  double wallThickness = settings.wallThicknessCells;
  CreativeDesktopPropertyEditActivity activity;
  bool continuousChanged = false;
  const auto observe = [&](bool changed) {
    continuousChanged = continuousChanged || changed;
    observeCreativeDesktopContinuousPropertyWidget(activity, changed);
  };

  ImGui::SeparatorText("Room shape");
  ImGui::BeginDisabled(state.roomManipulation.active ||
                       state.roomCornerManipulation.active ||
                       state.roomBoundaryManipulation.active);
  if (explicitTopology) {
    ImGui::TextDisabled("Bounds: %d x %d cells", width, depth);
    ImGui::TextDisabled("Shape is owned by the floor-plan walls above");
  } else {
    ImGui::SetNextItemWidth(128.0F);
    observe(ImGui::InputInt("Width##room_shell", &width, 1, 4));
    ImGui::SetNextItemWidth(128.0F);
    observe(ImGui::InputInt("Depth##room_shell", &depth, 1, 4));
    ImGui::SetNextItemWidth(128.0F);
    observe(ImGui::InputDouble("Room wall##room_shell", &wallThickness, 0.05,
                               0.25, "%.3f"));
  }
  if (explicitTopology) {
    ImGui::TextDisabled("Wall thickness is edited per selected wall");
  }
  ImGui::EndDisabled();

  const std::int64_t maximumX =
      static_cast<std::int64_t>(settings.footprint.minimum.x) + width;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(settings.footprint.minimum.z) + depth;
  const int maximumLayerCount = std::numeric_limits<std::uint16_t>::max();
  const bool valid =
      width > 0 && depth > 0 &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() &&
      settings.wallHeightCells > 0U &&
      settings.wallHeightCells <= maximumLayerCount &&
      settings.floorThicknessLayers > 0U &&
      settings.floorThicknessLayers <= maximumLayerCount &&
      settings.roofThicknessLayers > 0U &&
      settings.roofThicknessLayers <= maximumLayerCount &&
      std::isfinite(settings.floorTopLayer) &&
      std::isfinite(wallThickness) &&
      cr::validCreativeStructuralRoofSettings(
          settings.roofStyle, settings.roofRidgeAxis,
          settings.roofSlopeDirection, settings.roofPitchDegrees,
          settings.roofOverhangCells, settings.roofMaterial) &&
      settings.roofOverhangCells <=
          cr::kMaximumCreativeWorldLayoutRoofOverhangCells &&
      wallThickness > 0.0 &&
      static_cast<double>(width) > wallThickness * 2.0 &&
      static_cast<double>(depth) > wallThickness * 2.0;
  if (!valid) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "room shape is outside valid bounds");
  }

  if (continuousChanged && valid) {
    settings.footprint.maximum.x = static_cast<std::int32_t>(maximumX);
    settings.footprint.maximum.z = static_cast<std::int32_t>(maximumZ);
    settings.wallThicknessCells = wallThickness;
  }

  if (!explicitTopology) {
    finishCreativeDesktopWorldLayoutPropertyEdit(
        activity, current, settings, valid, "Reset room shape", state,
        roomIndex, room.stableKey, commands);
  }
}

std::vector<std::size_t> compatibleWallMergeEdges(
    const cr::CreativeWorldLayout& source,
    const cr::CreativeWorldLayoutRoomGraph& graph,
    std::size_t selectedEdge) {
  std::vector<std::size_t> compatible;
  if (selectedEdge >= graph.edges.size()) {
    return compatible;
  }
  const cr::CreativeWorldLayoutTopologyEdge& selected =
      graph.edges[selectedEdge];
  for (std::size_t candidate = 0U; candidate < graph.edges.size();
       ++candidate) {
    if (candidate == selectedEdge) {
      continue;
    }
    const cr::CreativeWorldLayoutTopologyEdge& edge = graph.edges[candidate];
    const bool incident =
        edge.startVertexIndex == selected.startVertexIndex ||
        edge.startVertexIndex == selected.endVertexIndex ||
        edge.endVertexIndex == selected.startVertexIndex ||
        edge.endVertexIndex == selected.endVertexIndex;
    if (!incident) {
      continue;
    }
    const cr::CreativeWorldLayoutWallOperationResult probe =
        cr::mergeCreativeWorldLayoutWalls(source, {selectedEdge, candidate});
    if (probe.accepted) {
      compatible.push_back(candidate);
    }
  }
  return compatible;
}

void drawTopologyEdgeSettings(
    CreativeEditorWorldLayoutState& state, std::size_t selectedEdge,
    CreativeDesktopCommandFrame& commands) {
  if (selectedEdge >= state.source.topologyEdges.size()) {
    return;
  }
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  if (!graph.accepted || selectedEdge >= graph.edges.size()) {
    state.roomEdgeSettingsDraft = {};
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "Wall topology is invalid");
    return;
  }
  const cr::CreativeWorldLayoutTopologyEdge& edge = graph.edges[selectedEdge];
  std::size_t ownerRoomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       graph.boundaries) {
    if (boundary.topologyEdgeIndex == selectedEdge) {
      ownerRoomIndex = boundary.roomIndex;
      break;
    }
  }
  const std::uint32_t currentLength =
      roomTopologyEdgeLength(graph, selectedEdge);
  const std::vector<std::size_t> mergeEdges =
      compatibleWallMergeEdges(state.source, graph, selectedEdge);

  CreativeEditorWorldLayoutRoomEdgeSettingsDraft& draft =
      state.roomEdgeSettingsDraft;
  if (draft.topologyEdgeIndex != selectedEdge ||
      draft.sourceRevision != state.revision) {
    draft = {};
    draft.roomIndex = ownerRoomIndex;
    draft.topologyEdgeIndex = selectedEdge;
    draft.sourceRevision = state.revision;
    draft.settings.lengthCells = currentLength;
    draft.settings.wallThicknessCells = edge.wallThicknessCells;
    draft.settings.wallHeightCells = edge.wallHeightCells;
    draft.settings.profile = edge.profile;
    draft.settings.material = edge.material;
    draft.settings.joinStyle = edge.joinStyle;
    draft.splitOffsetCells = currentLength / 2U;
    draft.mergeTopologyEdgeIndex =
        mergeEdges.empty() ? cr::kInvalidCreativeWorldLayoutIndex
                           : mergeEdges.front();
    state.selectedRoomTopologyEdgeStableKey = edge.stableKey;
  }
  if (std::find(mergeEdges.begin(), mergeEdges.end(),
                draft.mergeTopologyEdgeIndex) == mergeEdges.end()) {
    draft.mergeTopologyEdgeIndex =
        mergeEdges.empty() ? cr::kInvalidCreativeWorldLayoutIndex
                           : mergeEdges.front();
  }

  ImGui::SeparatorText("Wall");
  const std::string label = topologyEdgeLabel(graph, selectedEdge);
  ImGui::TextUnformatted(label.c_str());
  ImGui::TextDisabled("%zu room%s | %s",
                      static_cast<std::size_t>(std::count_if(
                          graph.boundaries.begin(), graph.boundaries.end(),
                          [selectedEdge](const auto& boundary) {
                            return boundary.topologyEdgeIndex == selectedEdge;
                          })),
                      std::count_if(
                          graph.boundaries.begin(), graph.boundaries.end(),
                          [selectedEdge](const auto& boundary) {
                            return boundary.topologyEdgeIndex == selectedEdge;
                          }) == 1
                          ? ""
                          : "s",
                      edge.stableKey.c_str());

  CreativeEditorWorldLayoutTopologyEdgeSettings& settings = draft.settings;
  CreativeEditorWorldLayoutTopologyEdgeSettings current = settings;
  current.lengthCells = currentLength;
  current.wallThicknessCells = edge.wallThicknessCells;
  current.wallHeightCells = edge.wallHeightCells;
  current.profile = edge.profile;
  current.material = edge.material;
  current.joinStyle = edge.joinStyle;
  CreativeDesktopPropertyEditActivity activity;
  const bool manipulationActive =
      state.roomManipulation.active || state.roomCornerManipulation.active ||
      state.roomBoundaryManipulation.active;
  ImGui::BeginDisabled(manipulationActive);
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Length##topology_wall", ImGuiDataType_U32,
                         &settings.lengthCells));
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Thickness##topology_wall",
                         &settings.wallThicknessCells, 0.05, 0.25, "%.3f"));
  int anchor = static_cast<int>(settings.fixedEndpoint);
  ImGui::SetNextItemWidth(170.0F);
  if (ImGui::Combo("Keep fixed##topology_wall", &anchor,
                   "First corner\0Second corner\0")) {
    settings.fixedEndpoint =
        static_cast<cr::CreativeWorldLayoutRoomEdgeAnchor>(anchor);
  }

  bool inheritHeight = settings.wallHeightCells == 0U;
  const bool inheritChanged = ImGui::Checkbox(
      "Inherit level height##topology_wall", &inheritHeight);
  if (inheritChanged) {
    settings.wallHeightCells =
        inheritHeight || edge.levelIndex >= state.source.levels.size()
            ? 0U
            : state.source.levels[edge.levelIndex].wallHeightCells;
  }
  observeCreativeDesktopDiscretePropertyWidget(activity, inheritChanged);
  if (!inheritHeight) {
    ImGui::SetNextItemWidth(128.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Height##topology_wall", ImGuiDataType_U16,
                           &settings.wallHeightCells));
  }

  constexpr std::array<const char*, 3U> kProfileLabels = {
      "Automatic", "Exterior", "Interior"};
  int profile = static_cast<int>(settings.profile);
  ImGui::SetNextItemWidth(150.0F);
  const bool profileChanged =
      ImGui::Combo("Profile##topology_wall", &profile, kProfileLabels.data(),
                   static_cast<int>(kProfileLabels.size()));
  if (profileChanged) {
    settings.profile =
        static_cast<cr::CreativeWorldLayoutWallProfile>(profile);
  }
  observeCreativeDesktopDiscretePropertyWidget(activity, profileChanged);
  constexpr std::array<const char*, 5U> kMaterialLabels = {
      "Blockout", "Plaster", "Timber", "Stone", "Brick"};
  int material = static_cast<int>(settings.material);
  ImGui::SetNextItemWidth(150.0F);
  const bool materialChanged = ImGui::Combo(
      "Material##topology_wall", &material, kMaterialLabels.data(),
      static_cast<int>(kMaterialLabels.size()));
  if (materialChanged) {
    settings.material =
        static_cast<cr::CreativeStructuralMaterial>(material);
  }
  observeCreativeDesktopDiscretePropertyWidget(activity, materialChanged);
  ImGui::TextDisabled("Join: Square overlap");
  ImGui::EndDisabled();

  const bool valid = settings.lengthCells > 0U &&
                     std::isfinite(settings.wallThicknessCells) &&
                     settings.wallThicknessCells > 0.0 &&
                     settings.profile <
                         cr::CreativeWorldLayoutWallProfile::Count &&
                     settings.material < cr::CreativeStructuralMaterial::Count &&
                     settings.joinStyle <
                         cr::CreativeWorldLayoutWallJoinStyle::Count;
  bool dirty = !(current == settings);
  if (!valid) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "Wall dimensions and settings are invalid");
  }
  if (!manipulationActive) {
    finishCreativeDesktopWorldLayoutPropertyEdit(
        activity, current, settings, valid, "Reset wall", state,
        selectedEdge, edge.stableKey, commands);
    dirty = !(current == settings);
  }

  ImGui::SeparatorText("Wall operations");
  ImGui::SetNextItemWidth(128.0F);
  ImGui::InputScalar("Split at##topology_wall", ImGuiDataType_U32,
                     &draft.splitOffsetCells);
  const bool splitValid = draft.splitOffsetCells > 0U &&
                          draft.splitOffsetCells < currentLength;
  ImGui::BeginDisabled(!splitValid || dirty || manipulationActive);
  if (ImGui::Button("Split wall")) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutSplitWall,
                  CreativeDesktopWorldLayoutWallSplitPayload{
                      selectedEdge, draft.splitOffsetCells});
  }
  ImGui::EndDisabled();

  const char* mergePreview = "No compatible wall";
  std::string mergePreviewStorage;
  if (draft.mergeTopologyEdgeIndex < graph.edges.size()) {
    mergePreviewStorage =
        topologyEdgeLabel(graph, draft.mergeTopologyEdgeIndex);
    mergePreview = mergePreviewStorage.c_str();
  }
  ImGui::SetNextItemWidth(260.0F);
  ImGui::BeginDisabled(mergeEdges.empty() || dirty || manipulationActive);
  if (ImGui::BeginCombo("Merge with##topology_wall", mergePreview)) {
    for (const std::size_t candidate : mergeEdges) {
      const bool selected = candidate == draft.mergeTopologyEdgeIndex;
      const std::string candidateLabel = topologyEdgeLabel(graph, candidate);
      if (ImGui::Selectable(candidateLabel.c_str(), selected)) {
        draft.mergeTopologyEdgeIndex = candidate;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::Button("Merge walls")) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutMergeWalls,
                  CreativeDesktopWorldLayoutWallMergePayload{
                      selectedEdge, draft.mergeTopologyEdgeIndex});
  }
  ImGui::EndDisabled();
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
  CreativeDesktopPropertyEditActivity activity;
  ImGui::TextUnformatted(isDoor ? "Door settings" : "Window settings");
  if (!opening.insertAssetId.empty()) {
    ImGui::TextDisabled("Asset: %s", opening.insertAssetId.c_str());
    if (!catalogContainsAsset(catalog, opening.insertAssetId)) {
      ImGui::TextColored({1.0F, 0.72F, 0.20F, 1.0F},
                         "Asset unavailable: procedural preview");
    }
  }
  ImGui::BeginDisabled(state.openingManipulation.active);
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Offset##opening", &settings.centerOffsetCells,
                         0.25, 1.0, "%.2f"));
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Width##opening", &settings.widthCells, 0.25, 1.0,
                         "%.2f"));
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Height##opening", &settings.heightCells, 0.25, 1.0,
                         "%.2f"));

  if (isDoor) {
    settings.sillHeightCells = 0.0;
    const CreativeDoorSettingsWidgetActivity doorActivity =
        drawCreativeDoorSettingsWidgets(settings.door,
                                        "world_layout_opening");
    observeCreativeDesktopDiscretePropertyWidget(
        activity, doorActivity.discreteChanged);
    observeCreativeDesktopContinuousPropertyEdit(
        activity, doorActivity.continuousChanged,
        doorActivity.continuousDeactivated);
  } else {
    ImGui::SetNextItemWidth(128.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("Sill##opening", &settings.sillHeightCells, 0.25,
                           1.0, "%.2f"));
    constexpr std::array<const char*, 2U> kWindowInsertLabels = {
        "Glazing", "Paired shutters"};
    int insertKind = static_cast<int>(settings.window.insertKind);
    ImGui::SetNextItemWidth(188.0F);
    if (ImGui::Combo("Treatment##opening", &insertKind,
                     kWindowInsertLabels.data(),
                     static_cast<int>(kWindowInsertLabels.size()))) {
      settings.window.insertKind =
          static_cast<cr::CreativeWindowInsertKind>(insertKind);
      observeCreativeDesktopDiscretePropertyWidget(activity, true);
    }
  }
  constexpr std::array<const char*, 2U> kFacingLabels = {
      "Side A (+ normal)", "Side B (- normal)"};
  int facing = static_cast<int>(settings.facing);
  ImGui::SetNextItemWidth(188.0F);
  if (ImGui::Combo("Front side##opening", &facing, kFacingLabels.data(),
                   static_cast<int>(kFacingLabels.size()))) {
    settings.facing = static_cast<cr::CreativeBuildingOpeningFacing>(facing);
    observeCreativeDesktopDiscretePropertyWidget(activity, true);
  }
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Insert##opening", &settings.includeInsert));
  ImGui::EndDisabled();

  const bool dirty = !(current == settings);
  if (!opening.insertAssetId.empty()) {
    ImGui::BeginDisabled(dirty || state.openingManipulation.active);
    if (ImGui::Button("Use procedural insert")) {
      commands.enqueue(
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
  const bool valuesRepresentable =
      std::isfinite(settings.centerOffsetCells) &&
      std::isfinite(settings.widthCells) &&
      std::isfinite(settings.sillHeightCells) &&
      std::isfinite(settings.heightCells) && settings.widthCells > 0.0 &&
      settings.sillHeightCells >= 0.0 && settings.heightCells > 0.0 &&
      (!isDoor || cr::isValidCreativeDoorSettings(settings.door)) &&
      (isDoor || cr::isValidCreativeWindowSettings(settings.window)) &&
      settings.facing < cr::CreativeBuildingOpeningFacing::Count;
  if (!valuesRepresentable) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "opening dimensions must be finite and positive");
  }
  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, settings, valuesRepresentable, "Reset opening",
      state, openingIndex, opening.stableKey, commands);
}


}  // namespace

void appendCreativeDesktopTopologyEdgeSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    std::size_t topologyEdgeIndex,
    CreativeDesktopCommandFrame& commands) {
  drawTopologyEdgeSettings(worldLayout, topologyEdgeIndex, commands);
}

void drawCreativeEditorWorldLayoutSelectionProperties(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands) {
  drawSelectedRoomSettings(state, commands);
  if (state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::TopologyEdge &&
      state.selection.index < state.source.topologyEdges.size()) {
    appendCreativeDesktopTopologyEdgeSettings(state, state.selection.index,
                                               commands);
  } else {
    state.roomEdgeSettingsDraft = {};
    state.selectedRoomTopologyEdgeStableKey.clear();
  }
  drawSelectedOpeningSettings(state, catalog, commands);
}

}  // namespace iggy3d_creative_app
