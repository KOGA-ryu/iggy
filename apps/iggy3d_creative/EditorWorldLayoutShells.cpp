#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <cmath>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace {

bool validShellSettings(
    const CreativeEditorWorldLayoutRoomSettings& settings) noexcept {
  const std::int64_t width =
      static_cast<std::int64_t>(settings.footprint.maximum.x) -
      settings.footprint.minimum.x;
  const std::int64_t depth =
      static_cast<std::int64_t>(settings.footprint.maximum.z) -
      settings.footprint.minimum.z;
  return width > 0 && depth > 0 && std::isfinite(settings.floorTopLayer) &&
         settings.wallHeightCells > 0U &&
         std::isfinite(settings.wallThicknessCells) &&
         settings.wallThicknessCells > 0.0 &&
         static_cast<double>(width) > settings.wallThicknessCells * 2.0 &&
         static_cast<double>(depth) > settings.wallThicknessCells * 2.0 &&
         settings.floorThicknessLayers > 0U;
}

cr::CreativeWorldLayoutRoom makeRoom(
    const CreativeEditorWorldLayoutRoomSettings& settings,
    std::size_t buildingIndex, std::size_t roomIndex,
    std::string stableKey) {
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = buildingIndex;
  room.stableKey = std::move(stableKey);
  room.name = "Room " + std::to_string(roomIndex + 1U);
  room.footprint = settings.footprint;
  room.floorTopLayer = settings.floorTopLayer;
  room.wallHeightCells = settings.wallHeightCells;
  room.wallThicknessCells = settings.wallThicknessCells;
  room.floorThicknessLayers = settings.floorThicknessLayers;
  return room;
}

CreativeEditorWorldLayoutEditReceipt commitRoomCandidate(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayout candidate, std::uint64_t nextStableOrdinal,
    std::size_t roomIndex, std::string statusMessage,
    std::string reasonCode) {
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    const bool overlaps =
        expanded.status ==
        cr::CreativeWorldLayoutRoomCompileStatus::OverlappingRooms;
    state.statusMessage = overlaps ? "rooms may touch but cannot overlap"
                                   : expanded.reasonCode;
    return {false, false, expanded.reasonCode};
  }
  if (cr::creativeWorldLayoutHasInteriorRoomWindow(candidate)) {
    state.statusMessage =
        "room would turn an exterior window into an interior opening";
    return {false, false,
            "creative_editor_world_layout_room_interior_window"};
  }

  state.source = std::move(candidate);
  state.nextStableOrdinal = nextStableOrdinal;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  detail::noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true, std::move(reasonCode)};
}

}  // namespace

CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingShell(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomSettings settings) {
  if (!validShellSettings(settings)) {
    state.statusMessage = "building shell needs valid floor and wall dimensions";
    return {false, false,
            "creative_editor_world_layout_building_shell_invalid"};
  }

  cr::CreativeWorldLayout candidate = state.source;
  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  const std::size_t buildingIndex = candidate.buildings.size();
  const std::size_t roomIndex = candidate.rooms.size();

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = cr::mintCreativeWorldLayoutStableKey(
      candidate, nextStableOrdinal, "building");
  building.name = "Building " + std::to_string(buildingIndex + 1U);
  building.rootMode = cr::CreativeBuildingRootMode::None;
  building.rootFootprint = settings.footprint;
  building.rootHeightCells = settings.wallHeightCells;
  candidate.buildings.push_back(std::move(building));

  candidate.rooms.push_back(makeRoom(
      settings, buildingIndex, roomIndex,
      cr::mintCreativeWorldLayoutStableKey(candidate, nextStableOrdinal,
                                            "room")));

  return commitRoomCandidate(
      state, std::move(candidate), nextStableOrdinal, roomIndex,
      "building shell staged; preview or confirm in 3D",
      "creative_editor_world_layout_building_shell_created");
}

CreativeEditorWorldLayoutEditReceipt createCreativeEditorWorldLayoutRoom(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutRoomSettings settings) {
  if (buildingIndex >= state.source.buildings.size() ||
      !validShellSettings(settings)) {
    state.statusMessage =
        buildingIndex >= state.source.buildings.size()
            ? "select the building that should own this room"
            : "room needs valid floor and wall dimensions";
    return {false, false,
            "creative_editor_world_layout_room_creation_invalid"};
  }

  cr::CreativeWorldLayout candidate = state.source;
  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  const std::size_t roomIndex = candidate.rooms.size();
  candidate.rooms.push_back(makeRoom(
      settings, buildingIndex, roomIndex,
      cr::mintCreativeWorldLayoutStableKey(candidate, nextStableOrdinal,
                                            "room")));
  return commitRoomCandidate(
      state, std::move(candidate), nextStableOrdinal, roomIndex,
      "room added to " + state.source.buildings[buildingIndex].name,
      "creative_editor_world_layout_room_added");
}

}  // namespace iggy3d_creative_app
