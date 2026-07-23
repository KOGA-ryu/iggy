#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"
#include "EditorWorldLayoutOpeningInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

using detail::clearWorldLayoutInteraction;
using detail::noteWorldLayoutSourceChange;
using detail::worldLayoutManipulatedRect;
using detail::worldLayoutRectHandleAt;
using opening_detail::kOpeningEndClearanceCells;

namespace detail {

bool validWorldLayoutShellSettings(
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
             settings.roofSlopeDirection, settings.roofPitchDegrees,
             settings.roofOverhangCells, settings.roofMaterial) &&
         settings.roofOverhangCells <=
             cr::kMaximumCreativeWorldLayoutRoofOverhangCells;
}

cr::CreativeWorldLayoutRoom makeWorldLayoutRoom(
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

}  // namespace detail

namespace {

CreativeEditorWorldLayoutEditReceipt commitRoomCandidate(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayout candidate, std::uint64_t nextStableOrdinal,
    std::size_t roomIndex, std::string statusMessage,
    std::string reasonCode) {
  if (roomIndex >= candidate.rooms.size() ||
      !cr::refreshCreativeWorldLayoutBuildingRoomFootprint(
          candidate, candidate.rooms[roomIndex].buildingIndex)) {
    state.statusMessage = "room has no valid building footprint";
    return {false, false,
            "creative_editor_world_layout_room_building_footprint_invalid"};
  }
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

struct RoomSettingsValidation {
  bool accepted = false;
  bool changed = false;
  std::string reasonCode =
      "creative_editor_world_layout_room_settings_invalid";
  std::string message = "room shell settings are invalid";
  cr::CreativeWorldLayout candidate;
};

std::string roomFootprintEditMessage(
    const cr::CreativeWorldLayoutRoomFootprintEditResult& result) {
  switch (result.status) {
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::InvalidFootprint:
      return "room boundary would collapse a room";
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::
        SharedRoomMoveUnsupported:
      return "move the building instead of tearing an adjoining room";
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::OpeningDoesNotFit:
      return "resize would move an opening outside its wall";
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::OpeningOverlap:
      return "resize would overlap openings on a shared wall";
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::InteriorWindow:
      return "resize would turn a window into an interior opening";
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::
        VerticalConnectorDoesNotFit:
      return "resize would move a stair or ramp outside its rooms";
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::NoChange:
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::Ready:
      return "room drag preview";
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::NotRequested:
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::InvalidRequest:
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::InvalidOwnership:
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::InvalidSourceTopology:
    case cr::CreativeWorldLayoutRoomFootprintEditStatus::
        ResultingTopologyInvalid:
      return result.reasonCode;
  }
  return result.reasonCode;
}

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
                   level->roofOverhangCells, level->roofSlopeDirection,
                   level->roofMaterial};
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
          settings.roofSlopeDirection, settings.roofPitchDegrees,
          settings.roofOverhangCells, settings.roofMaterial) ||
      settings.roofOverhangCells >
          cr::kMaximumCreativeWorldLayoutRoofOverhangCells ||
      width <= settings.wallThicknessCells * 2.0 ||
      depth <= settings.wallThicknessCells * 2.0) {
    return {};
  }

  if (cr::creativeWorldLayoutLevelForRoom(state.source, roomIndex) == nullptr) {
    return {};
  }

  cr::CreativeWorldLayout prepared = state.source;
  cr::CreativeWorldLayoutRoom& candidateRoom = prepared.rooms[roomIndex];
  const cr::CreativeWorldLayoutRoom& existingRoom =
      state.source.rooms[roomIndex];
  const bool explicitTopology = !state.source.roomBoundaries.empty();
  if (explicitTopology &&
      !(settings.footprint.minimum == existingRoom.footprint.minimum &&
        settings.footprint.maximum == existingRoom.footprint.maximum)) {
    RoomSettingsValidation rejected;
    rejected.reasonCode =
        "creative_editor_world_layout_room_topology_footprint_edit_required";
    rejected.message =
        "use floor-plan boundary controls to reshape this room";
    return rejected;
  }
  if (explicitTopology &&
      settings.wallThicknessCells != existingRoom.wallThicknessCells) {
    RoomSettingsValidation rejected;
    rejected.reasonCode =
        "creative_editor_world_layout_room_topology_wall_edit_required";
    rejected.message =
        "edit wall thickness on the floor-plan boundary";
    return rejected;
  }
  candidateRoom.wallThicknessCells = settings.wallThicknessCells;
  cr::CreativeWorldLayoutLevel& candidateLevel =
      prepared.levels[candidateRoom.levelIndex];
  candidateLevel.floorTopLayer = settings.floorTopLayer;
  candidateLevel.wallHeightCells = settings.wallHeightCells;
  candidateLevel.floorThicknessLayers = settings.floorThicknessLayers;
  candidateLevel.roofThicknessLayers = settings.roofThicknessLayers;
  candidateLevel.roofStyle = settings.roofStyle;
  candidateLevel.roofRidgeAxis = settings.roofRidgeAxis;
  candidateLevel.roofSlopeDirection = settings.roofSlopeDirection;
  candidateLevel.roofPitchDegrees = settings.roofPitchDegrees;
  candidateLevel.roofOverhangCells = settings.roofOverhangCells;
  candidateLevel.roofMaterial = settings.roofMaterial;

  cr::CreativeWorldLayout candidate;
  bool footprintChanged = false;
  if (explicitTopology) {
    const cr::CreativeWorldLayoutRoomGraph graph =
        cr::buildCreativeWorldLayoutRoomGraph(prepared);
    if (!graph.accepted) {
      RoomSettingsValidation rejected;
      rejected.reasonCode = graph.reasonCode;
      rejected.message = "repair the floor plan before editing room settings";
      return rejected;
    }
    candidate = std::move(prepared);
  } else {
    cr::CreativeWorldLayoutRoomFootprintEditResult topology =
        cr::editCreativeWorldLayoutRoomFootprint(
            prepared,
            {roomIndex, settings.footprint, kOpeningEndClearanceCells});
    if (!topology.accepted) {
      RoomSettingsValidation rejected;
      rejected.reasonCode = topology.reasonCode;
      rejected.message = roomFootprintEditMessage(topology);
      return rejected;
    }
    footprintChanged = topology.changed;
    candidate = std::move(topology.edited);
  }

  const cr::CreativeWorldLayoutLevel& existingLevel =
      state.source.levels[existingRoom.levelIndex];
  RoomSettingsValidation accepted;
  accepted.accepted = true;
  accepted.changed = footprintChanged ||
                     existingRoom.wallThicknessCells !=
                         settings.wallThicknessCells ||
                     existingLevel.floorTopLayer != settings.floorTopLayer ||
                     existingLevel.wallHeightCells != settings.wallHeightCells ||
                     existingLevel.floorThicknessLayers !=
                         settings.floorThicknessLayers ||
                     existingLevel.roofThicknessLayers !=
                         settings.roofThicknessLayers ||
                     existingLevel.roofStyle != settings.roofStyle ||
                     existingLevel.roofRidgeAxis != settings.roofRidgeAxis ||
                     existingLevel.roofSlopeDirection !=
                         settings.roofSlopeDirection ||
                     existingLevel.roofPitchDegrees !=
                         settings.roofPitchDegrees ||
                     existingLevel.roofOverhangCells !=
                         settings.roofOverhangCells ||
                     existingLevel.roofMaterial != settings.roofMaterial;
  accepted.reasonCode =
      "creative_editor_world_layout_room_settings_ready";
  accepted.message = "room shell settings ready";
  accepted.candidate = std::move(candidate);
  return accepted;
}

}  // namespace

CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingShell(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomSettings settings) {
  if (!detail::validWorldLayoutShellSettings(settings)) {
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
  level.roofSlopeDirection = settings.roofSlopeDirection;
  level.roofPitchDegrees = settings.roofPitchDegrees;
  level.roofOverhangCells = settings.roofOverhangCells;
  level.roofMaterial = settings.roofMaterial;
  candidate.levels.push_back(std::move(level));

  candidate.rooms.push_back(detail::makeWorldLayoutRoom(
      settings, buildingIndex, levelIndex, roomIndex,
      cr::mintCreativeWorldLayoutStableKey(candidate, nextStableOrdinal,
                                            "room")));

  return commitRoomCandidate(
      state, std::move(candidate), nextStableOrdinal, roomIndex,
      "building shell staged; preview or confirm in 3D",
      "creative_editor_world_layout_building_shell_created");
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
  candidate.rooms.push_back(detail::makeWorldLayoutRoom(
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
  if (!validation.changed) {
    return {true, false,
            "creative_editor_world_layout_room_settings_no_change"};
  }
  const std::size_t levelIndex = state.source.rooms[roomIndex].levelIndex;
  state.source = std::move(validation.candidate);
  state.activeLevelIndex = levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  noteWorldLayoutSourceChange(state, "room shell settings updated");
  return {true, true, "creative_editor_world_layout_room_settings_updated"};
}

bool readCreativeEditorWorldLayoutRoomMetadata(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomMetadata& output) noexcept {
  if (roomIndex >= state.source.rooms.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  output = {room.name, room.type};
  return true;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutRoomMetadata(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomMetadata metadata) {
  if (roomIndex >= state.source.rooms.size() ||
      !detail::hasVisibleWorldLayoutName(metadata.name) ||
      metadata.name.size() > 127U ||
      metadata.type >= cr::CreativeWorldLayoutRoomType::Count) {
    state.statusMessage = "room name and type are invalid";
    return {false, false,
            "creative_editor_world_layout_room_metadata_invalid"};
  }
  cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  if (room.name == metadata.name && room.type == metadata.type) {
    return {true, false,
            "creative_editor_world_layout_room_metadata_no_change"};
  }
  room.name = std::move(metadata.name);
  room.type = metadata.type;
  state.activeLevelIndex = room.levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  noteWorldLayoutSourceChange(state, "room identity updated");
  return {true, true,
          "creative_editor_world_layout_room_metadata_updated"};
}

CreativeEditorWorldLayoutRoomTarget findCreativeEditorWorldLayoutRoomTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  if (!state.source.roomBoundaries.empty()) {
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
    state.roomManipulation = {};
    state.roomManipulation.active = true;
    state.roomManipulation.sourceRevision = state.revision;
    state.roomManipulation.target = target;
    state.roomManipulation.startPoint = point;
    state.roomManipulation.originalFootprint = footprint;
    state.roomManipulation.previewFootprint = footprint;
    state.roomManipulation.previewEdit =
        cr::editCreativeWorldLayoutRoomFootprint(
            state.source,
            {target.roomIndex, footprint, kOpeningEndClearanceCells});
    state.roomManipulation.previewValid =
        state.roomManipulation.previewEdit.accepted;
    state.roomManipulation.reasonCode =
        state.roomManipulation.previewEdit.reasonCode;
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
    cr::CreativeWorldLayoutRoomFootprintEditResult edit;
    std::string message;
    if (coordinateValid) {
      edit = cr::editCreativeWorldLayoutRoomFootprint(
          state.source, {roomIndex, footprint, kOpeningEndClearanceCells});
      message = roomFootprintEditMessage(edit);
    } else {
      edit.requested = true;
      edit.status =
          cr::CreativeWorldLayoutRoomFootprintEditStatus::InvalidFootprint;
      edit.reasonCode =
          "creative_editor_world_layout_room_manipulation_out_of_range";
      message = "room drag exceeds the layout coordinate range";
    }
    state.roomManipulation.previewFootprint = footprint;
    state.roomManipulation.previewValid = edit.accepted;
    state.roomManipulation.reasonCode = edit.reasonCode;
    state.roomManipulation.previewEdit = std::move(edit);
    state.statusMessage = std::move(message);
    return {true, true, state.roomManipulation.reasonCode};
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
  const std::size_t committedRoomIndex =
      state.roomManipulation.target.roomIndex;
  const std::size_t committedLevelIndex =
      state.source.rooms[committedRoomIndex].levelIndex;
  const bool changed = state.roomManipulation.previewEdit.changed;
  cr::CreativeWorldLayout candidate =
      std::move(state.roomManipulation.previewEdit.edited);
  state.roomManipulation = {};
  if (!changed) {
    state.statusMessage = "room footprint unchanged";
    return {true, false,
            "creative_editor_world_layout_room_manipulation_no_change"};
  }
  state.source = std::move(candidate);
  state.activeLevelIndex = committedLevelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                     committedRoomIndex};
  noteWorldLayoutSourceChange(state, "room boundary updated");
  return {true, true,
          "creative_editor_world_layout_room_manipulation_updated"};
}

}  // namespace iggy3d_creative_app
