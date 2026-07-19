#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"
#include "EditorWorldLayoutOpeningInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
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

using detail::noteWorldLayoutSourceChange;
using opening_detail::kOpeningGeometryEpsilon;

namespace {

CreativeEditorWorldLayoutEditReceipt commitBuildingBlockoutCandidate(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayout candidate, std::uint64_t nextStableOrdinal,
    std::size_t buildingIndex, std::size_t levelIndex,
    std::string statusMessage =
        "building blockout staged; preview or confirm in 3D",
    std::string reasonCode =
        "creative_editor_world_layout_building_blockout_created") {
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
  noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true, std::move(reasonCode)};
}
bool blockoutOverlapsExistingRoom(
    const cr::CreativeWorldLayout& layout,
    const CreativeEditorWorldLayoutRoomSettings& settings,
    double candidateTop,
    std::size_t ignoredBuildingIndex =
        cr::kInvalidCreativeWorldLayoutIndex) noexcept {
  const double candidateBottom = settings.floorTopLayer;
  for (const cr::CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex == ignoredBuildingIndex) {
      continue;
    }
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

cr::CreativeWorldLayoutBuildingBlockoutRecipe blockoutRecipe(
    const CreativeEditorWorldLayoutBuildingBlockoutSettings& settings) {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint = settings.shell.footprint;
  recipe.request.pattern = settings.pattern;
  recipe.request.wallThicknessCells = settings.shell.wallThicknessCells;
  recipe.request.connectRooms = settings.connectRooms;
  recipe.request.facade = settings.facade;
  recipe.request.wallHeightCells = settings.shell.wallHeightCells;
  recipe.request.storeys = settings.storeys;
  recipe.floorTopLayer = settings.shell.floorTopLayer;
  recipe.floorThicknessLayers = settings.shell.floorThicknessLayers;
  recipe.roofThicknessLayers = settings.shell.roofThicknessLayers;
  recipe.roofStyle = settings.shell.roofStyle;
  recipe.roofRidgeAxis = settings.shell.roofRidgeAxis;
  recipe.roofPitchDegrees = settings.shell.roofPitchDegrees;
  recipe.roofOverhangCells = settings.shell.roofOverhangCells;
  return recipe;
}

CreativeEditorWorldLayoutBuildingBlockoutSettings blockoutSettings(
    const cr::CreativeWorldLayoutBuildingBlockoutRecipe& recipe) {
  CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = recipe.request.footprint;
  settings.shell.floorTopLayer = recipe.floorTopLayer;
  settings.shell.wallHeightCells = recipe.request.wallHeightCells;
  settings.shell.wallThicknessCells = recipe.request.wallThicknessCells;
  settings.shell.floorThicknessLayers = recipe.floorThicknessLayers;
  settings.shell.roofThicknessLayers = recipe.roofThicknessLayers;
  settings.shell.roofStyle = recipe.roofStyle;
  settings.shell.roofRidgeAxis = recipe.roofRidgeAxis;
  settings.shell.roofPitchDegrees = recipe.roofPitchDegrees;
  settings.shell.roofOverhangCells = recipe.roofOverhangCells;
  settings.pattern = recipe.request.pattern;
  settings.connectRooms = recipe.request.connectRooms;
  settings.facade = recipe.request.facade;
  settings.storeys = recipe.request.storeys;
  return settings;
}

bool sameBlockoutRecipe(
    const cr::CreativeWorldLayoutBuildingBlockoutRecipe& lhs,
    const cr::CreativeWorldLayoutBuildingBlockoutRecipe& rhs) noexcept {
  return lhs.version == rhs.version &&
         lhs.request.footprint.minimum == rhs.request.footprint.minimum &&
         lhs.request.footprint.maximum == rhs.request.footprint.maximum &&
         lhs.request.pattern == rhs.request.pattern &&
         lhs.request.wallThicknessCells == rhs.request.wallThicknessCells &&
         lhs.request.connectRooms == rhs.request.connectRooms &&
         lhs.request.facade.includeEntrance ==
             rhs.request.facade.includeEntrance &&
         lhs.request.facade.entranceEdge == rhs.request.facade.entranceEdge &&
         lhs.request.facade.entranceOffsetCells ==
             rhs.request.facade.entranceOffsetCells &&
         lhs.request.facade.includeExteriorWindows ==
             rhs.request.facade.includeExteriorWindows &&
         lhs.request.wallHeightCells == rhs.request.wallHeightCells &&
         lhs.request.storeys.count == rhs.request.storeys.count &&
         lhs.request.storeys.connectStoreys ==
             rhs.request.storeys.connectStoreys &&
         lhs.request.storeys.connectorKind ==
             rhs.request.storeys.connectorKind &&
         lhs.request.storeys.preferredDirection ==
             rhs.request.storeys.preferredDirection &&
         lhs.floorTopLayer == rhs.floorTopLayer &&
         lhs.floorThicknessLayers == rhs.floorThicknessLayers &&
         lhs.roofThicknessLayers == rhs.roofThicknessLayers &&
         lhs.roofStyle == rhs.roofStyle &&
         lhs.roofRidgeAxis == rhs.roofRidgeAxis &&
         lhs.roofPitchDegrees == rhs.roofPitchDegrees &&
         lhs.roofOverhangCells == rhs.roofOverhangCells;
}

std::size_t firstBuildingLevelIndex(const cr::CreativeWorldLayout& layout,
                                    std::size_t buildingIndex) noexcept {
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    if (layout.levels[index].buildingIndex == buildingIndex) {
      return index;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
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

}  // namespace

CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings) {
  if (!detail::validWorldLayoutShellSettings(settings.shell)) {
    state.statusMessage =
        "building blockout needs valid floor, wall, and roof dimensions";
    return {false, false,
            "creative_editor_world_layout_building_blockout_settings_invalid"};
  }

  const cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe =
      blockoutRecipe(settings);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan blockout =
      cr::planCreativeWorldLayoutBuildingBlockout(recipe.request);
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
      candidate.rooms.push_back(detail::makeWorldLayoutRoom(
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

  cr::CreativeWorldLayoutBuildingBlockoutProvenance provenance;
  provenance.present = true;
  provenance.valid = true;
  provenance.recipe = recipe;
  const cr::CreativeWorldLayoutBuildingBlockoutFingerprint baseline =
      cr::fingerprintCreativeWorldLayoutBuildingBlockout(candidate,
                                                         buildingIndex);
  if (!baseline.valid) {
    state.statusMessage = "building blockout provenance could not be measured";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_provenance_invalid"};
  }
  provenance.instanceBaselineFingerprint = baseline.value;
  if (!cr::setCreativeWorldLayoutBuildingBlockoutProvenance(
          candidate, buildingIndex, provenance)) {
    state.statusMessage = "building blockout provenance could not be recorded";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_provenance_invalid"};
  }

  return commitBuildingBlockoutCandidate(
      state, std::move(candidate), nextStableOrdinal, buildingIndex,
      firstLevelIndex);
}

bool readCreativeEditorWorldLayoutBuildingBlockoutSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBlockoutSettings& output) noexcept {
  const cr::CreativeWorldLayoutBuildingBlockoutProvenance provenance =
      cr::creativeWorldLayoutBuildingBlockoutProvenance(state.source,
                                                        buildingIndex);
  if (!provenance.valid) {
    return false;
  }
  output = blockoutSettings(provenance.recipe);
  return true;
}

CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings) {
  if (buildingIndex >= state.source.buildings.size() ||
      !detail::validWorldLayoutShellSettings(settings.shell)) {
    state.statusMessage =
        "building blockout update needs a valid linked building and settings";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_invalid"};
  }

  const cr::CreativeWorldLayoutBuildingBlockoutSyncReceipt sync =
      cr::inspectCreativeWorldLayoutBuildingBlockoutSync(state.source,
                                                         buildingIndex);
  if (!sync.accepted ||
      sync.state !=
          cr::CreativeWorldLayoutBuildingBlockoutSyncState::Current) {
    if (sync.state ==
        cr::CreativeWorldLayoutBuildingBlockoutSyncState::Unlinked) {
      state.statusMessage =
          "selected building was not created from an editable blockout";
      return {
          false, false,
          "creative_editor_world_layout_building_blockout_update_unlinked"};
    }
    if (sync.state ==
        cr::CreativeWorldLayoutBuildingBlockoutSyncState::LocallyModified) {
      state.statusMessage =
          "building blockout has local refinements; update cancelled";
      return {
          false, false,
          "creative_editor_world_layout_building_blockout_update_conflict"};
    }
    state.statusMessage = "building blockout provenance is invalid";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_provenance_"
        "invalid"};
  }

  const cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe =
      blockoutRecipe(settings);
  if (!cr::validCreativeWorldLayoutBuildingBlockoutRecipe(recipe)) {
    state.statusMessage = "building blockout update settings are invalid";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_invalid"};
  }
  if (sameBlockoutRecipe(sync.provenance.recipe, recipe)) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                       buildingIndex};
    return {
        true, false,
        "creative_editor_world_layout_building_blockout_update_no_change"};
  }

  double blockoutTop = 0.0;
  if (!blockoutTopLayer(settings.shell, settings.storeys.count,
                        blockoutTop)) {
    state.statusMessage =
        "building blockout elevation is outside supported limits";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_elevation_invalid"};
  }
  if (blockoutOverlapsExistingRoom(state.source, settings.shell, blockoutTop,
                                   buildingIndex)) {
    state.statusMessage = "building blockout overlaps an existing building";
    return {false, false,
            "creative_editor_world_layout_building_blockout_overlap"};
  }

  CreativeEditorWorldLayoutState replacementState;
  resetCreativeEditorWorldLayout(replacementState,
                                 "building_blockout_replacement");
  const CreativeEditorWorldLayoutEditReceipt generated =
      createCreativeEditorWorldLayoutBuildingBlockout(replacementState,
                                                      settings);
  if (!generated.accepted || replacementState.source.buildings.size() != 1U) {
    state.statusMessage = replacementState.statusMessage;
    return {false, false, generated.reasonCode};
  }

  cr::CreativeWorldLayout candidate = state.source;
  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  if (!cr::replaceCreativeWorldLayoutBuildingInCandidate(
          candidate, buildingIndex, replacementState.source,
          nextStableOrdinal, true)) {
    state.statusMessage = "building blockout replacement could not be remapped";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_remap_failed"};
  }

  cr::CreativeWorldLayoutBuildingBlockoutProvenance provenance;
  provenance.present = true;
  provenance.valid = true;
  provenance.recipe = recipe;
  const cr::CreativeWorldLayoutBuildingBlockoutFingerprint baseline =
      cr::fingerprintCreativeWorldLayoutBuildingBlockout(candidate,
                                                         buildingIndex);
  if (!baseline.valid) {
    state.statusMessage = "updated blockout provenance could not be measured";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_provenance_"
        "invalid"};
  }
  provenance.instanceBaselineFingerprint = baseline.value;
  if (!cr::setCreativeWorldLayoutBuildingBlockoutProvenance(
          candidate, buildingIndex, provenance)) {
    state.statusMessage = "updated blockout provenance could not be recorded";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_provenance_"
        "invalid"};
  }

  const std::size_t levelIndex =
      firstBuildingLevelIndex(candidate, buildingIndex);
  if (levelIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage = "updated blockout has no building level";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_remap_failed"};
  }
  return commitBuildingBlockoutCandidate(
      state, std::move(candidate), nextStableOrdinal, buildingIndex,
      levelIndex, "building blockout updated; preview or confirm in 3D",
      "creative_editor_world_layout_building_blockout_updated");
}

}  // namespace iggy3d_creative_app

