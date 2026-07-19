#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"
#include "EditorWorldLayoutOpeningInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {

using detail::clearWorldLayoutInteraction;
using detail::noteWorldLayoutSourceChange;
using detail::worldLayoutManipulatedRect;
using detail::worldLayoutRectHandleAt;
using opening_detail::kOpeningEndClearanceCells;
using opening_detail::kOpeningGeometryEpsilon;

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
         settings.floorThicknessLayers > 0U &&
         settings.roofThicknessLayers > 0U &&
         cr::validCreativeStructuralRoofSettings(
             settings.roofStyle, settings.roofRidgeAxis,
             settings.roofPitchDegrees, settings.roofOverhangCells) &&
         settings.roofOverhangCells <=
             cr::kMaximumCreativeWorldLayoutRoofOverhangCells;
}

cr::CreativeWorldLayoutRoom makeRoom(
    const CreativeEditorWorldLayoutRoomSettings& settings,
    std::size_t buildingIndex, std::size_t levelIndex, std::size_t roomIndex,
    std::string stableKey) {
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = buildingIndex;
  room.levelIndex = levelIndex;
  room.stableKey = std::move(stableKey);
  room.name = "Room " + std::to_string(roomIndex + 1U);
  room.footprint = settings.footprint;
  room.wallThicknessCells = settings.wallThicknessCells;
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
  state.activeLevelIndex = state.source.rooms[roomIndex].levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  detail::noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true, std::move(reasonCode)};
}

CreativeEditorWorldLayoutEditReceipt commitBuildingBlockoutCandidate(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayout candidate, std::uint64_t nextStableOrdinal,
    std::size_t buildingIndex, std::size_t levelIndex) {
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    state.statusMessage =
        expanded.status ==
                cr::CreativeWorldLayoutRoomCompileStatus::OverlappingRooms
            ? "rooms may touch but cannot overlap"
            : expanded.reasonCode;
    return {false, false, expanded.reasonCode};
  }
  if (cr::creativeWorldLayoutHasInteriorRoomWindow(candidate)) {
    state.statusMessage =
        "blockout would turn an exterior window into an interior opening";
    return {false, false,
            "creative_editor_world_layout_blockout_interior_window"};
  }

  state.source = std::move(candidate);
  state.nextStableOrdinal = nextStableOrdinal;
  state.activeLevelIndex = levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  noteWorldLayoutSourceChange(
      state, "building blockout staged; preview or confirm in 3D");
  return {true, true,
          "creative_editor_world_layout_building_blockout_created"};
}

bool roomFootprintOverlaps(const cr::CreativeWorldLayout& layout,
                           cr::CreativeWorldLayoutRect footprint,
                           std::size_t buildingIndex,
                           std::size_t levelIndex,
                           std::size_t ignoredRoom =
                               cr::kInvalidCreativeWorldLayoutIndex) {
  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    if (index == ignoredRoom) {
      continue;
    }
    if (layout.rooms[index].buildingIndex != buildingIndex ||
        layout.rooms[index].levelIndex != levelIndex) {
      continue;
    }
    const cr::CreativeWorldLayoutRect existing = layout.rooms[index].footprint;
    if (std::max(footprint.minimum.x, existing.minimum.x) <
            std::min(footprint.maximum.x, existing.maximum.x) &&
        std::max(footprint.minimum.z, existing.minimum.z) <
            std::min(footprint.maximum.z, existing.maximum.z)) {
      return true;
    }
  }
  return false;
}

bool blockoutOverlapsExistingRoom(
    const cr::CreativeWorldLayout& layout,
    const CreativeEditorWorldLayoutRoomSettings& settings,
    double candidateTop) noexcept {
  const double candidateBottom = settings.floorTopLayer;
  for (const cr::CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.levelIndex >= layout.levels.size()) {
      continue;
    }
    const cr::CreativeWorldLayoutRect existing = room.footprint;
    const bool horizontalOverlap =
        std::max(settings.footprint.minimum.x, existing.minimum.x) <
            std::min(settings.footprint.maximum.x, existing.maximum.x) &&
        std::max(settings.footprint.minimum.z, existing.minimum.z) <
            std::min(settings.footprint.maximum.z, existing.maximum.z);
    const cr::CreativeWorldLayoutLevel& level = layout.levels[room.levelIndex];
    const double existingBottom = level.floorTopLayer;
    const double existingTop =
        existingBottom + static_cast<double>(level.wallHeightCells);
    const bool verticalOverlap =
        std::max(candidateBottom, existingBottom) <
        std::min(candidateTop, existingTop) - kOpeningGeometryEpsilon;
    if (horizontalOverlap && verticalOverlap) {
      return true;
    }
  }
  return false;
}

bool blockoutTopLayer(
    const CreativeEditorWorldLayoutRoomSettings& settings,
    std::uint16_t storeyCount, double& topLayer) noexcept {
  const long double value =
      static_cast<long double>(settings.floorTopLayer) +
      static_cast<long double>(settings.wallHeightCells) * storeyCount;
  if (!std::isfinite(value) ||
      value < -static_cast<long double>(std::numeric_limits<double>::max()) ||
      value > static_cast<long double>(std::numeric_limits<double>::max())) {
    return false;
  }
  topLayer = static_cast<double>(value);
  return std::isfinite(topLayer) && topLayer > settings.floorTopLayer;
}

bool sharedSpanMatchesOpeningIntent(
    const std::vector<cr::CreativeWorldLayoutSharedRoomEdgeSpan>& sharedSpans,
    std::size_t firstRoomIndex,
    const cr::CreativeWorldLayoutBuildingBlockoutOpening& intent) noexcept {
  if (intent.adjacentRoomIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    return false;
  }
  const std::size_t roomIndex = firstRoomIndex + intent.roomIndex;
  const std::size_t adjacentRoomIndex =
      firstRoomIndex + intent.adjacentRoomIndex;
  return std::any_of(
      sharedSpans.begin(), sharedSpans.end(),
      [&](const cr::CreativeWorldLayoutSharedRoomEdgeSpan& span) {
        return (span.firstRoomIndex == roomIndex &&
                span.firstRoomEdge == intent.roomEdge &&
                span.secondRoomIndex == adjacentRoomIndex &&
                span.secondRoomEdge == intent.adjacentRoomEdge) ||
               (span.secondRoomIndex == roomIndex &&
                span.secondRoomEdge == intent.roomEdge &&
                span.firstRoomIndex == adjacentRoomIndex &&
                span.firstRoomEdge == intent.adjacentRoomEdge);
      });
}

bool openingMatchesBlockoutIntent(
    const cr::CreativeWorldLayout& layout, std::size_t firstRoomIndex,
    const std::vector<cr::CreativeWorldLayoutSharedRoomEdgeSpan>& sharedSpans,
    const cr::CreativeWorldLayoutBuildingBlockoutOpening& intent,
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  const std::size_t roomIndex = firstRoomIndex + intent.roomIndex;
  if (intent.role >=
          cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::Count ||
      roomIndex >= layout.rooms.size() || opening.kind != intent.kind ||
      opening.hostKind != cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
      opening.roomIndex != roomIndex || opening.roomEdge != intent.roomEdge) {
    return false;
  }
  const bool shared = cr::creativeWorldLayoutRoomEdgeIntervalIsShared(
      layout, roomIndex, intent.roomEdge, opening.centerOffsetCells,
      opening.widthCells);
  switch (intent.role) {
    case cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
        InteriorConnection:
      return opening.kind == cr::CreativeBuildingOpeningKind::Door && shared &&
             sharedSpanMatchesOpeningIntent(sharedSpans, firstRoomIndex,
                                             intent);
    case cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::Entrance:
      return opening.kind == cr::CreativeBuildingOpeningKind::Door && !shared;
    case cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::ExteriorWindow:
      return opening.kind == cr::CreativeBuildingOpeningKind::Window &&
             !shared;
    case cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::Count:
      break;
  }
  return false;
}

std::string blockoutOpeningName(
    cr::CreativeWorldLayoutBuildingBlockoutOpeningRole role,
    std::size_t ordinal) {
  switch (role) {
    case cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
        InteriorConnection:
      return "Interior Door " + std::to_string(ordinal);
    case cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::Entrance:
      return "Entrance";
    case cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::ExteriorWindow:
      return "Exterior Window " + std::to_string(ordinal);
    case cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::Count:
      break;
  }
  return "Opening " + std::to_string(ordinal);
}

std::string_view blockoutVerticalConnectorLabel(
    cr::CreativeWorldLayoutVerticalConnectorKind kind) noexcept {
  switch (kind) {
    case cr::CreativeWorldLayoutVerticalConnectorKind::Stair:
      return "Stair";
    case cr::CreativeWorldLayoutVerticalConnectorKind::Ramp:
      return "Ramp";
    case cr::CreativeWorldLayoutVerticalConnectorKind::Count:
      break;
  }
  return "Vertical Connector";
}

std::string_view blockoutVerticalConnectorKeyPrefix(
    cr::CreativeWorldLayoutVerticalConnectorKind kind) noexcept {
  switch (kind) {
    case cr::CreativeWorldLayoutVerticalConnectorKind::Stair:
      return "stair";
    case cr::CreativeWorldLayoutVerticalConnectorKind::Ramp:
      return "ramp";
    case cr::CreativeWorldLayoutVerticalConnectorKind::Count:
      break;
  }
  return "vertical_connector";
}

struct RoomSettingsValidation {
  bool accepted = false;
  std::string reasonCode =
      "creative_editor_world_layout_room_settings_invalid";
  std::string message = "room shell settings are invalid";
};

CreativeEditorWorldLayoutRoomSettings roomSettings(
    const cr::CreativeWorldLayout& layout, std::size_t roomIndex,
    cr::CreativeWorldLayoutRect footprint) noexcept {
  if (roomIndex >= layout.rooms.size()) {
    return {};
  }
  const cr::CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
  const cr::CreativeWorldLayoutLevel* level =
      cr::creativeWorldLayoutLevelForRoom(layout, roomIndex);
  return level == nullptr
             ? CreativeEditorWorldLayoutRoomSettings{}
             : CreativeEditorWorldLayoutRoomSettings{
                   footprint, level->floorTopLayer, level->wallHeightCells,
                   room.wallThicknessCells, level->floorThicknessLayers,
                   level->roofThicknessLayers, level->roofStyle,
                   level->roofRidgeAxis, level->roofPitchDegrees,
                   level->roofOverhangCells};
}

RoomSettingsValidation validateRoomSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    const CreativeEditorWorldLayoutRoomSettings& settings) {
  const double width = static_cast<double>(settings.footprint.maximum.x) -
                       settings.footprint.minimum.x;
  const double depth = static_cast<double>(settings.footprint.maximum.z) -
                       settings.footprint.minimum.z;
  if (roomIndex >= state.source.rooms.size() || width <= 0.0 || depth <= 0.0 ||
      !std::isfinite(settings.floorTopLayer) ||
      settings.wallHeightCells == 0U || settings.floorThicknessLayers == 0U ||
      settings.roofThicknessLayers == 0U ||
      !std::isfinite(settings.wallThicknessCells) ||
      settings.wallThicknessCells <= 0.0 ||
      !cr::validCreativeStructuralRoofSettings(
          settings.roofStyle, settings.roofRidgeAxis,
          settings.roofPitchDegrees, settings.roofOverhangCells) ||
      settings.roofOverhangCells >
          cr::kMaximumCreativeWorldLayoutRoofOverhangCells ||
      width <= settings.wallThicknessCells * 2.0 ||
      depth <= settings.wallThicknessCells * 2.0) {
    return {};
  }

  const cr::CreativeWorldLayoutRoom& existingRoom =
      state.source.rooms[roomIndex];
  if (cr::creativeWorldLayoutLevelForRoom(state.source, roomIndex) == nullptr) {
    return {};
  }
  if (roomFootprintOverlaps(state.source, settings.footprint,
                            existingRoom.buildingIndex,
                            existingRoom.levelIndex,
                            roomIndex)) {
    return {false, "creative_editor_world_layout_room_overlap",
            "rooms may touch but cannot overlap"};
  }

  for (const cr::CreativeWorldLayoutOpening& opening : state.source.openings) {
    if (opening.hostKind != cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomIndex != roomIndex) {
      continue;
    }
    const bool horizontal =
        opening.roomEdge == cr::CreativeWorldLayoutRoomEdge::North ||
        opening.roomEdge == cr::CreativeWorldLayoutRoomEdge::South;
    const double edgeLength = horizontal ? width : depth;
    const double halfWidth = opening.widthCells * 0.5;
    if (opening.centerOffsetCells - halfWidth <
            kOpeningEndClearanceCells - kOpeningGeometryEpsilon ||
        opening.centerOffsetCells + halfWidth >
            edgeLength - kOpeningEndClearanceCells +
                kOpeningGeometryEpsilon) {
      return {
          false,
          "creative_editor_world_layout_room_resize_opening_invalid",
          "resize would move an opening outside its wall",
      };
    }
  }

  cr::CreativeWorldLayout candidate = state.source;
  cr::CreativeWorldLayoutRoom& candidateRoom = candidate.rooms[roomIndex];
  candidateRoom.footprint = settings.footprint;
  candidateRoom.wallThicknessCells = settings.wallThicknessCells;
  cr::CreativeWorldLayoutLevel& candidateLevel =
      candidate.levels[candidateRoom.levelIndex];
  candidateLevel.floorTopLayer = settings.floorTopLayer;
  candidateLevel.wallHeightCells = settings.wallHeightCells;
  candidateLevel.floorThicknessLayers = settings.floorThicknessLayers;
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    return {false, expanded.reasonCode, expanded.reasonCode};
  }
  if (cr::creativeWorldLayoutHasInteriorRoomWindow(candidate)) {
    return {false,
            "creative_editor_world_layout_room_resize_interior_window",
            "resize would turn a window into an interior opening"};
  }
  for (std::size_t index = 0U; index < expanded.expanded.openings.size();
       ++index) {
    const cr::CreativeWorldLayoutOpening& opening =
        expanded.expanded.openings[index];
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const cr::CreativeWorldLayoutOpening& existing =
          expanded.expanded.openings[prior];
      if (opening.wallIndex == existing.wallIndex &&
          std::fabs(opening.centerOffsetCells -
                    existing.centerOffsetCells) <=
              (opening.widthCells + existing.widthCells) * 0.5 + 1.0e-9) {
        return {false, "creative_editor_world_layout_opening_overlap",
                "resize would overlap openings on a shared wall"};
      }
    }
  }
  return {true, "creative_editor_world_layout_room_settings_ready",
          "room shell settings ready"};
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
  const std::size_t levelIndex = candidate.levels.size();
  const std::size_t roomIndex = candidate.rooms.size();

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = cr::mintCreativeWorldLayoutStableKey(
      candidate, nextStableOrdinal, "building");
  building.name = "Building " + std::to_string(buildingIndex + 1U);
  building.rootMode = cr::CreativeBuildingRootMode::None;
  building.rootFootprint = settings.footprint;
  building.rootHeightCells = settings.wallHeightCells;
  candidate.buildings.push_back(std::move(building));

  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = buildingIndex;
  level.stableKey = cr::mintCreativeWorldLayoutStableKey(
      candidate, nextStableOrdinal, "level");
  level.name = "Level 0";
  level.floorTopLayer = settings.floorTopLayer;
  level.wallHeightCells = settings.wallHeightCells;
  level.floorThicknessLayers = settings.floorThicknessLayers;
  level.roofThicknessLayers = settings.roofThicknessLayers;
  level.roofStyle = settings.roofStyle;
  level.roofRidgeAxis = settings.roofRidgeAxis;
  level.roofPitchDegrees = settings.roofPitchDegrees;
  level.roofOverhangCells = settings.roofOverhangCells;
  candidate.levels.push_back(std::move(level));

  candidate.rooms.push_back(makeRoom(
      settings, buildingIndex, levelIndex, roomIndex,
      cr::mintCreativeWorldLayoutStableKey(candidate, nextStableOrdinal,
                                            "room")));

  return commitRoomCandidate(
      state, std::move(candidate), nextStableOrdinal, roomIndex,
      "building shell staged; preview or confirm in 3D",
      "creative_editor_world_layout_building_shell_created");
}

CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings) {
  if (!validShellSettings(settings.shell)) {
    state.statusMessage =
        "building blockout needs valid floor, wall, and roof dimensions";
    return {false, false,
            "creative_editor_world_layout_building_blockout_settings_invalid"};
  }

  cr::CreativeWorldLayoutBuildingBlockoutRequest blockoutRequest;
  blockoutRequest.footprint = settings.shell.footprint;
  blockoutRequest.pattern = settings.pattern;
  blockoutRequest.wallThicknessCells = settings.shell.wallThicknessCells;
  blockoutRequest.connectRooms = settings.connectRooms;
  blockoutRequest.facade = settings.facade;
  blockoutRequest.wallHeightCells = settings.shell.wallHeightCells;
  blockoutRequest.storeys = settings.storeys;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan blockout =
      cr::planCreativeWorldLayoutBuildingBlockout(blockoutRequest);
  if (!blockout.accepted) {
    state.statusMessage = blockout.reasonCode;
    return {false, false, std::string(blockout.reasonCode)};
  }
  double blockoutTop = 0.0;
  if (!blockoutTopLayer(settings.shell, blockout.storeyCount, blockoutTop)) {
    state.statusMessage = "building blockout elevation is outside supported limits";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_elevation_invalid"};
  }
  if (blockoutOverlapsExistingRoom(state.source, settings.shell,
                                   blockoutTop)) {
    state.statusMessage = "building blockout overlaps an existing building";
    return {false, false,
            "creative_editor_world_layout_building_blockout_overlap"};
  }

  cr::CreativeWorldLayout candidate = state.source;
  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  const std::size_t buildingIndex = candidate.buildings.size();
  const std::size_t firstLevelIndex = candidate.levels.size();

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = cr::mintCreativeWorldLayoutStableKey(
      candidate, nextStableOrdinal, "building");
  building.name = "Building " + std::to_string(buildingIndex + 1U);
  building.rootMode = cr::CreativeBuildingRootMode::None;
  building.rootFootprint = blockout.footprint;
  building.rootHeightCells = settings.shell.wallHeightCells;
  candidate.buildings.push_back(std::move(building));

  for (std::uint16_t storey = 0U; storey < blockout.storeyCount; ++storey) {
    cr::CreativeWorldLayoutLevel level;
    level.buildingIndex = buildingIndex;
    level.stableKey = cr::mintCreativeWorldLayoutStableKey(
        candidate, nextStableOrdinal, "level");
    level.name = "Level " + std::to_string(storey);
    level.floorTopLayer = static_cast<double>(
        static_cast<long double>(settings.shell.floorTopLayer) +
        static_cast<long double>(settings.shell.wallHeightCells) * storey);
    level.wallHeightCells = settings.shell.wallHeightCells;
    level.floorThicknessLayers = settings.shell.floorThicknessLayers;
    level.roofThicknessLayers = settings.shell.roofThicknessLayers;
    level.roofStyle = settings.shell.roofStyle;
    level.roofRidgeAxis = settings.shell.roofRidgeAxis;
    level.roofPitchDegrees = settings.shell.roofPitchDegrees;
    level.roofOverhangCells = settings.shell.roofOverhangCells;
    candidate.levels.push_back(std::move(level));
  }

  const std::size_t firstRoomIndex = candidate.rooms.size();
  for (std::uint16_t storey = 0U; storey < blockout.storeyCount; ++storey) {
    const std::size_t levelIndex = firstLevelIndex + storey;
    for (std::size_t room = 0U; room < blockout.roomCount; ++room) {
      CreativeEditorWorldLayoutRoomSettings roomSettings = settings.shell;
      roomSettings.footprint = blockout.rooms[room];
      const std::size_t roomIndex =
          firstRoomIndex + static_cast<std::size_t>(storey) *
                               blockout.roomCount +
          room;
      candidate.rooms.push_back(makeRoom(
          roomSettings, buildingIndex, levelIndex, roomIndex,
          cr::mintCreativeWorldLayoutStableKey(candidate, nextStableOrdinal,
                                                "room")));
    }
  }

  if (blockout.openingCount > 0U) {
    CreativeEditorWorldLayoutState candidateState;
    candidateState.source = std::move(candidate);
    const std::vector<cr::CreativeWorldLayoutSharedRoomEdgeSpan> sharedSpans =
        cr::inspectCreativeWorldLayoutSharedRoomEdges(candidateState.source);
    std::size_t interiorDoorOrdinal = 1U;
    std::size_t exteriorWindowOrdinal = 1U;
    for (std::uint16_t storey = 0U; storey < blockout.storeyCount; ++storey) {
      candidateState.activeLevelIndex = firstLevelIndex + storey;
      const std::size_t firstStoreyRoomIndex =
          firstRoomIndex +
          static_cast<std::size_t>(storey) * blockout.roomCount;
      for (std::size_t index = 0U; index < blockout.openingCount; ++index) {
        cr::CreativeWorldLayoutBuildingBlockoutOpening intent =
            blockout.openings[index];
        if (storey > 0U &&
            intent.role ==
                cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::Entrance) {
          if (!settings.facade.includeExteriorWindows) {
            continue;
          }
          intent.role = cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
              ExteriorWindow;
          intent.kind = cr::CreativeBuildingOpeningKind::Window;
        }
        CreativeEditorWorldLayoutOpeningPlacementRequest request;
        request.point = {intent.xCells, intent.zCells};
        request.kind = intent.kind;
        CreativeEditorWorldLayoutOpeningPlacementPlan placement =
            planCreativeEditorWorldLayoutOpeningPlacement(candidateState,
                                                          request);
        if (!placement.accepted) {
          state.statusMessage = placement.message;
          return {false, false, std::string(placement.reasonCode)};
        }
        if (!openingMatchesBlockoutIntent(
                candidateState.source, firstStoreyRoomIndex, sharedSpans,
                intent, placement.opening)) {
          state.statusMessage =
              "building blockout could not resolve an opening host";
          return {
              false, false,
              "creative_editor_world_layout_building_blockout_opening_host"};
        }

        cr::CreativeWorldLayoutOpening opening = std::move(placement.opening);
        opening.stableKey = cr::mintCreativeWorldLayoutStableKey(
            candidateState.source, nextStableOrdinal,
            opening.kind == cr::CreativeBuildingOpeningKind::Door ? "door"
                                                                   : "window");
        std::size_t openingOrdinal = 1U;
        if (intent.role ==
            cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                InteriorConnection) {
          openingOrdinal = interiorDoorOrdinal++;
        } else if (
            intent.role == cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                               ExteriorWindow) {
          openingOrdinal = exteriorWindowOrdinal++;
        }
        opening.name = blockoutOpeningName(intent.role, openingOrdinal);
        candidateState.source.openings.push_back(std::move(opening));
      }
    }
    candidate = std::move(candidateState.source);
  }

  if (blockout.hasVerticalConnector) {
    const cr::CreativeWorldLayoutBuildingBlockoutVerticalConnector& intent =
        blockout.verticalConnector;
    for (std::uint16_t storey = 0U; storey + 1U < blockout.storeyCount;
         ++storey) {
      cr::CreativeWorldLayoutVerticalConnector connector;
      connector.buildingIndex = buildingIndex;
      connector.lowerRoomIndex =
          firstRoomIndex +
          static_cast<std::size_t>(storey) * blockout.roomCount +
          intent.roomIndex;
      connector.upperRoomIndex =
          firstRoomIndex +
          static_cast<std::size_t>(storey + 1U) * blockout.roomCount +
          intent.roomIndex;
      connector.kind = intent.kind;
      connector.direction = intent.direction;
      connector.stableKey = cr::mintCreativeWorldLayoutStableKey(
          candidate, nextStableOrdinal,
          blockoutVerticalConnectorKeyPrefix(intent.kind));
      connector.name =
          std::string(blockoutVerticalConnectorLabel(intent.kind)) + " " +
          std::to_string(storey + 1U);
      connector.footprint = intent.footprint;
      candidate.verticalConnectors.push_back(std::move(connector));
      const std::size_t connectorIndex =
          candidate.verticalConnectors.size() - 1U;
      const cr::CreativeWorldLayoutVerticalConnectorPlan connectorPlan =
          cr::planCreativeWorldLayoutVerticalConnector({}, candidate,
                                                       connectorIndex);
      if (!connectorPlan.accepted) {
        state.statusMessage = std::string(connectorPlan.reasonCode);
        return {false, false, std::string(connectorPlan.reasonCode)};
      }
    }
  }

  return commitBuildingBlockoutCandidate(
      state, std::move(candidate), nextStableOrdinal, buildingIndex,
      firstLevelIndex);
}

CreativeEditorWorldLayoutEditReceipt createCreativeEditorWorldLayoutRoom(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutRoomSettings settings) {
  const bool levelValid =
      levelIndex < state.source.levels.size() &&
      state.source.levels[levelIndex].buildingIndex <
          state.source.buildings.size();
  const double width = static_cast<double>(settings.footprint.maximum.x) -
                       settings.footprint.minimum.x;
  const double depth = static_cast<double>(settings.footprint.maximum.z) -
                       settings.footprint.minimum.z;
  const bool geometryValid =
      width > 0.0 && depth > 0.0 &&
      std::isfinite(settings.wallThicknessCells) &&
      settings.wallThicknessCells > 0.0 &&
      width > settings.wallThicknessCells * 2.0 &&
      depth > settings.wallThicknessCells * 2.0;
  if (!levelValid || !geometryValid) {
    state.statusMessage =
        !levelValid
            ? "select the building level that should own this room"
            : "room needs valid floor and wall dimensions";
    return {false, false,
            "creative_editor_world_layout_room_creation_invalid"};
  }

  cr::CreativeWorldLayout candidate = state.source;
  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  const std::size_t roomIndex = candidate.rooms.size();
  const std::size_t buildingIndex = candidate.levels[levelIndex].buildingIndex;
  candidate.rooms.push_back(makeRoom(
      settings, buildingIndex, levelIndex, roomIndex,
      cr::mintCreativeWorldLayoutStableKey(candidate, nextStableOrdinal,
                                            "room")));
  return commitRoomCandidate(
      state, std::move(candidate), nextStableOrdinal, roomIndex,
      "room added to " + state.source.buildings[buildingIndex].name,
      "creative_editor_world_layout_room_added");
}

bool readCreativeEditorWorldLayoutRoomSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings& output) noexcept {
  if (roomIndex >= state.source.rooms.size()) {
    return false;
  }
  output = roomSettings(state.source, roomIndex,
                        state.source.rooms[roomIndex].footprint);
  return cr::creativeWorldLayoutLevelForRoom(state.source, roomIndex) !=
         nullptr;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings) {
  const RoomSettingsValidation validation =
      validateRoomSettings(state, roomIndex, settings);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, validation.reasonCode};
  }
  cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  cr::CreativeWorldLayoutLevel& level = state.source.levels[room.levelIndex];
  if (room.footprint.minimum == settings.footprint.minimum &&
      room.footprint.maximum == settings.footprint.maximum &&
      level.floorTopLayer == settings.floorTopLayer &&
      level.wallHeightCells == settings.wallHeightCells &&
      room.wallThicknessCells == settings.wallThicknessCells &&
      level.floorThicknessLayers == settings.floorThicknessLayers &&
      level.roofThicknessLayers == settings.roofThicknessLayers &&
      level.roofStyle == settings.roofStyle &&
      level.roofRidgeAxis == settings.roofRidgeAxis &&
      level.roofPitchDegrees == settings.roofPitchDegrees &&
      level.roofOverhangCells == settings.roofOverhangCells) {
    return {true, false,
            "creative_editor_world_layout_room_settings_no_change"};
  }
  room.footprint = settings.footprint;
  room.wallThicknessCells = settings.wallThicknessCells;
  level.floorTopLayer = settings.floorTopLayer;
  level.wallHeightCells = settings.wallHeightCells;
  level.floorThicknessLayers = settings.floorThicknessLayers;
  level.roofThicknessLayers = settings.roofThicknessLayers;
  level.roofStyle = settings.roofStyle;
  level.roofRidgeAxis = settings.roofRidgeAxis;
  level.roofPitchDegrees = settings.roofPitchDegrees;
  level.roofOverhangCells = settings.roofOverhangCells;
  state.activeLevelIndex = room.levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  noteWorldLayoutSourceChange(state, "room shell settings updated");
  return {true, true, "creative_editor_world_layout_room_settings_updated"};
}

CreativeEditorWorldLayoutRoomTarget findCreativeEditorWorldLayoutRoomTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  const CreativeEditorWorldLayoutSelection hit =
      detail::hitTestWorldLayout(state.source, point,
                                 state.activeLevelIndex);
  if (hit.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      hit.index < state.source.rooms.size()) {
    const CreativeEditorWorldLayoutRoomHandle handle = worldLayoutRectHandleAt(
        state.source.rooms[hit.index].footprint, point, toleranceCells);
    if (handle != CreativeEditorWorldLayoutRoomHandle::None) {
      return {hit.index, handle};
    }
  }
  if (hit.kind != CreativeEditorWorldLayoutSelectionKind::None) {
    return {};
  }
  if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      detail::worldLayoutRoomOnActiveLevel(state, state.source,
                                           state.selection.index)) {
    const CreativeEditorWorldLayoutRoomHandle handle = worldLayoutRectHandleAt(
        state.source.rooms[state.selection.index].footprint, point,
        toleranceCells);
    if (handle != CreativeEditorWorldLayoutRoomHandle::None) {
      return {state.selection.index, handle};
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutRoomManipulationPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Cancel) {
    const bool changed = state.roomManipulation.active;
    state.roomManipulation = {};
    state.statusMessage = "room manipulation cancelled";
    return {true, changed,
            "creative_editor_world_layout_room_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutRoomTarget target =
        findCreativeEditorWorldLayoutRoomTarget(state, point, toleranceCells);
    if (target.handle == CreativeEditorWorldLayoutRoomHandle::None ||
        target.roomIndex >= state.source.rooms.size()) {
      return detail::selectWorldLayoutAtPoint(state, point);
    }
    const cr::CreativeWorldLayoutRect footprint =
        state.source.rooms[target.roomIndex].footprint;
    clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                       target.roomIndex};
    state.anchorActive = false;
    state.roomManipulation = {
        true,
        state.revision,
        target,
        point,
        footprint,
        footprint,
        true,
        "creative_editor_world_layout_room_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutRoomHandle::Move
            ? "drag to move room"
            : "drag to resize room";
    return {true, true,
            "creative_editor_world_layout_room_manipulation_started"};
  }
  if (!state.roomManipulation.active ||
      state.roomManipulation.target.roomIndex >= state.source.rooms.size()) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_not_active"};
  }
  const std::size_t roomIndex = state.roomManipulation.target.roomIndex;
  const cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  if (state.revision != state.roomManipulation.sourceRevision ||
      !(room.footprint.minimum ==
            state.roomManipulation.originalFootprint.minimum &&
        room.footprint.maximum ==
            state.roomManipulation.originalFootprint.maximum)) {
    state.roomManipulation = {};
    state.statusMessage = "room changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_room_manipulation_stale"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Update) {
    cr::CreativeWorldLayoutRect footprint;
    const bool coordinateValid = worldLayoutManipulatedRect(
        state.roomManipulation, point, footprint);
    if (coordinateValid &&
        footprint.minimum == state.roomManipulation.previewFootprint.minimum &&
        footprint.maximum == state.roomManipulation.previewFootprint.maximum) {
      return {true, false, state.roomManipulation.reasonCode};
    }
    RoomSettingsValidation validation;
    if (coordinateValid) {
      validation = validateRoomSettings(
          state, roomIndex,
          roomSettings(state.source, roomIndex, footprint));
    } else {
      validation.reasonCode =
          "creative_editor_world_layout_room_manipulation_out_of_range";
      validation.message = "room drag exceeds the layout coordinate range";
    }
    state.roomManipulation.previewFootprint = footprint;
    state.roomManipulation.previewValid = validation.accepted;
    state.roomManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted ? "room drag preview"
                                              : validation.message;
    return {true, true, validation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutRoomManipulation(
          state, CreativeEditorWorldLayoutRoomManipulationPhase::Update, point,
          toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.roomManipulation.previewValid) {
    const std::string reasonCode = state.roomManipulation.reasonCode;
    state.roomManipulation = {};
    return {false, false, reasonCode};
  }
  const cr::CreativeWorldLayoutRect footprint =
      state.roomManipulation.previewFootprint;
  state.roomManipulation = {};
  return setCreativeEditorWorldLayoutRoomSettings(
      state, roomIndex, roomSettings(state.source, roomIndex, footprint));
}

}  // namespace iggy3d_creative_app
