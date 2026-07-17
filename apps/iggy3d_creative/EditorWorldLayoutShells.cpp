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

  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = buildingIndex;
  room.stableKey = cr::mintCreativeWorldLayoutStableKey(
      candidate, nextStableOrdinal, "room");
  room.name = "Room " + std::to_string(roomIndex + 1U);
  room.footprint = settings.footprint;
  room.floorTopLayer = settings.floorTopLayer;
  room.wallHeightCells = settings.wallHeightCells;
  room.wallThicknessCells = settings.wallThicknessCells;
  room.floorThicknessLayers = settings.floorThicknessLayers;
  candidate.rooms.push_back(std::move(room));

  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    state.statusMessage = expanded.reasonCode;
    return {false, false, expanded.reasonCode};
  }

  state.source = std::move(candidate);
  state.nextStableOrdinal = nextStableOrdinal;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  detail::noteWorldLayoutSourceChange(
      state, "building shell staged; preview or confirm in 3D");
  return {true, true,
          "creative_editor_world_layout_building_shell_created"};
}

}  // namespace iggy3d_creative_app
