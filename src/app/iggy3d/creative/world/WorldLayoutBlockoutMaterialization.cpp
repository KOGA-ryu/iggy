#include "app/iggy3d/creative/world/WorldLayoutBlockoutMaterialization.hpp"

#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr double kOpeningSnapCells = 0.25;
constexpr double kOpeningEndClearanceCells = 0.25;
constexpr double kOpeningGeometryEpsilon = 1.0e-9;

void setFailure(CreativeWorldLayoutBuildingEditResult& result,
                CreativeWorldLayoutBuildingEditStatus status,
                std::string reasonCode) {
  result.accepted = false;
  result.changed = false;
  result.status = status;
  result.edited = {};
  result.reasonCode = std::move(reasonCode);
}

std::string openingName(
    CreativeWorldLayoutBuildingBlockoutOpeningRole role,
    std::size_t ordinal) {
  switch (role) {
    case CreativeWorldLayoutBuildingBlockoutOpeningRole::InteriorConnection:
      return "Interior Door " + std::to_string(ordinal);
    case CreativeWorldLayoutBuildingBlockoutOpeningRole::Entrance:
      return "Entrance";
    case CreativeWorldLayoutBuildingBlockoutOpeningRole::ExteriorWindow:
      return "Exterior Window " + std::to_string(ordinal);
    case CreativeWorldLayoutBuildingBlockoutOpeningRole::Count:
      break;
  }
  return "Opening " + std::to_string(ordinal);
}

std::string_view connectorLabel(
    CreativeWorldLayoutVerticalConnectorKind kind) noexcept {
  switch (kind) {
    case CreativeWorldLayoutVerticalConnectorKind::Stair:
      return "Stair";
    case CreativeWorldLayoutVerticalConnectorKind::Ramp:
      return "Ramp";
    case CreativeWorldLayoutVerticalConnectorKind::Count:
      break;
  }
  return "Vertical Connector";
}

std::string_view connectorKeyPrefix(
    CreativeWorldLayoutVerticalConnectorKind kind) noexcept {
  switch (kind) {
    case CreativeWorldLayoutVerticalConnectorKind::Stair:
      return "stair";
    case CreativeWorldLayoutVerticalConnectorKind::Ramp:
      return "ramp";
    case CreativeWorldLayoutVerticalConnectorKind::Count:
      break;
  }
  return "vertical_connector";
}

bool sharedSpanMatchesIntent(
    const std::vector<CreativeWorldLayoutSharedRoomEdgeSpan>& sharedSpans,
    std::size_t firstRoomIndex,
    const CreativeWorldLayoutBuildingBlockoutOpening& intent) noexcept {
  if (intent.adjacentRoomIndex == kInvalidCreativeWorldLayoutIndex) {
    return false;
  }
  const std::size_t roomIndex = firstRoomIndex + intent.roomIndex;
  const std::size_t adjacentRoomIndex =
      firstRoomIndex + intent.adjacentRoomIndex;
  return std::any_of(
      sharedSpans.begin(), sharedSpans.end(),
      [&](const CreativeWorldLayoutSharedRoomEdgeSpan& span) {
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

double openingOffset(const CreativeWorldLayoutRoom& room,
                     const CreativeWorldLayoutBuildingBlockoutOpening& intent) {
  return intent.roomEdge == CreativeWorldLayoutRoomEdge::North ||
                 intent.roomEdge == CreativeWorldLayoutRoomEdge::South
             ? intent.xCells - room.footprint.minimum.x
             : intent.zCells - room.footprint.minimum.z;
}

double openingHostLength(const CreativeWorldLayoutRoom& room,
                         CreativeWorldLayoutRoomEdge edge) {
  return edge == CreativeWorldLayoutRoomEdge::North ||
                 edge == CreativeWorldLayoutRoomEdge::South
             ? static_cast<double>(room.footprint.maximum.x) -
                   room.footprint.minimum.x
             : static_cast<double>(room.footprint.maximum.z) -
                   room.footprint.minimum.z;
}

bool openingIntervalsOverlap(const CreativeWorldLayoutOpening& lhs,
                             const CreativeWorldLayoutOpening& rhs) noexcept {
  if (lhs.hostKind != rhs.hostKind || lhs.roomIndex != rhs.roomIndex ||
      lhs.roomEdge != rhs.roomEdge) {
    return false;
  }
  const double lhsBegin = lhs.centerOffsetCells - lhs.widthCells * 0.5;
  const double lhsEnd = lhs.centerOffsetCells + lhs.widthCells * 0.5;
  const double rhsBegin = rhs.centerOffsetCells - rhs.widthCells * 0.5;
  const double rhsEnd = rhs.centerOffsetCells + rhs.widthCells * 0.5;
  return std::max(lhsBegin, rhsBegin) <
         std::min(lhsEnd, rhsEnd) - kOpeningGeometryEpsilon;
}

bool makeOpening(
    const CreativeWorldLayout& layout,
    const std::vector<CreativeWorldLayoutSharedRoomEdgeSpan>& sharedSpans,
    std::size_t firstStoreyRoomIndex,
    const CreativeWorldLayoutBuildingBlockoutOpening& intent,
    CreativeWorldLayoutOpening& opening,
    std::string& reasonCode) {
  const std::size_t roomIndex = firstStoreyRoomIndex + intent.roomIndex;
  if (intent.role >= CreativeWorldLayoutBuildingBlockoutOpeningRole::Count ||
      roomIndex >= layout.rooms.size() ||
      intent.roomEdge >= CreativeWorldLayoutRoomEdge::Count) {
    reasonCode =
        "creative_world_layout_building_blockout_opening_host_invalid";
    return false;
  }

  opening.hostKind = CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = roomIndex;
  opening.roomEdge = intent.roomEdge;
  opening.kind = intent.kind;
  opening.includeInsert = true;
  switch (intent.kind) {
    case CreativeBuildingOpeningKind::Door:
      opening.widthCells = 1.0;
      opening.cutoutBottomCells = 0.0;
      opening.cutoutHeightCells = 2.1;
      opening.insertBottomCells = 0.0;
      opening.insertHeightCells = 2.1;
      opening.insertWidthCells = 1.0;
      opening.insertThicknessCells = 0.15;
      break;
    case CreativeBuildingOpeningKind::Window:
      opening.widthCells = 1.5;
      opening.cutoutBottomCells = 1.0;
      opening.cutoutHeightCells = 1.2;
      opening.insertBottomCells = 1.0;
      opening.insertHeightCells = 1.2;
      opening.insertWidthCells = 1.5;
      opening.insertThicknessCells = 0.10;
      break;
  }

  const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
  const CreativeWorldLayoutLevel& level = layout.levels[room.levelIndex];
  if (opening.cutoutBottomCells + opening.cutoutHeightCells >
      level.wallHeightCells + kOpeningGeometryEpsilon) {
    reasonCode =
        "creative_world_layout_building_blockout_opening_height_invalid";
    return false;
  }
  const double halfWidth = opening.widthCells * 0.5;
  const double minimumCenter = halfWidth + kOpeningEndClearanceCells;
  const double maximumCenter = openingHostLength(room, intent.roomEdge) -
                               halfWidth - kOpeningEndClearanceCells;
  if (minimumCenter > maximumCenter + kOpeningGeometryEpsilon) {
    reasonCode = "creative_world_layout_building_blockout_wall_too_short";
    return false;
  }
  opening.centerOffsetCells = std::clamp(
      std::round(openingOffset(room, intent) / kOpeningSnapCells) *
          kOpeningSnapCells,
      minimumCenter, maximumCenter);

  const bool shared = creativeWorldLayoutRoomEdgeIntervalIsShared(
      layout, roomIndex, intent.roomEdge, opening.centerOffsetCells,
      opening.widthCells);
  if (intent.role ==
      CreativeWorldLayoutBuildingBlockoutOpeningRole::InteriorConnection) {
    if (intent.kind != CreativeBuildingOpeningKind::Door || !shared ||
        !sharedSpanMatchesIntent(sharedSpans, firstStoreyRoomIndex, intent)) {
      reasonCode =
          "creative_world_layout_building_blockout_opening_host_invalid";
      return false;
    }
  } else if (shared ||
             (intent.role ==
                  CreativeWorldLayoutBuildingBlockoutOpeningRole::Entrance &&
              intent.kind != CreativeBuildingOpeningKind::Door) ||
             (intent.role == CreativeWorldLayoutBuildingBlockoutOpeningRole::
                                 ExteriorWindow &&
              intent.kind != CreativeBuildingOpeningKind::Window)) {
    reasonCode =
        "creative_world_layout_building_blockout_opening_host_invalid";
    return false;
  }

  if (std::any_of(layout.openings.begin(), layout.openings.end(),
                  [&](const CreativeWorldLayoutOpening& existing) {
                    return openingIntervalsOverlap(opening, existing);
                  })) {
    reasonCode = "creative_world_layout_building_blockout_opening_overlap";
    return false;
  }
  return true;
}

}  // namespace

CreativeWorldLayoutBuildingEditResult
materializeCreativeWorldLayoutBuildingBlockout(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingBlockoutRecipe& recipe,
    std::uint64_t nextStableOrdinal,
    CreativeWorldLayoutBuildingBlockoutMaterializationOptions options) {
  CreativeWorldLayoutBuildingEditResult result;
  result.requested = true;
  result.nextStableOrdinal = nextStableOrdinal;
  if (!validCreativeWorldLayoutBuildingOwnership(source)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidOwnership,
               "creative_world_layout_building_blockout_ownership_invalid");
    return result;
  }
  const CreativeWorldLayoutBuildingBlockoutPlan blockout =
      planCreativeWorldLayoutBuildingBlockout(recipe.request);
  if (!blockout.accepted) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
               std::string(blockout.reasonCode));
    return result;
  }
  if (!validCreativeWorldLayoutBuildingBlockoutRecipe(recipe)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
               "creative_world_layout_building_blockout_recipe_invalid");
    return result;
  }

  CreativeWorldLayout candidate = source;
  std::uint64_t nextOrdinal = nextStableOrdinal;
  const std::size_t buildingIndex = candidate.buildings.size();
  const std::size_t firstLevelIndex = candidate.levels.size();
  const std::size_t firstRoomIndex = candidate.rooms.size();

  CreativeWorldLayoutBuilding building;
  building.stableKey =
      mintCreativeWorldLayoutStableKey(candidate, nextOrdinal, "building");
  building.name = options.buildingName.empty()
                      ? "Building " + std::to_string(buildingIndex + 1U)
                      : std::string(options.buildingName);
  building.rootMode = CreativeBuildingRootMode::None;
  building.rootFootprint = blockout.footprint;
  building.rootHeightCells = recipe.request.floorToFloorCells;
  for (std::string_view tag : options.buildingTags) {
    building.tags.emplace_back(tag);
  }
  candidate.buildings.push_back(std::move(building));

  for (std::uint16_t storey = 0U; storey < blockout.storeyCount; ++storey) {
    CreativeWorldLayoutLevel level;
    level.buildingIndex = buildingIndex;
    level.stableKey =
        mintCreativeWorldLayoutStableKey(candidate, nextOrdinal, "level");
    level.name = "Level " + std::to_string(storey);
    level.floorTopLayer = static_cast<double>(
        static_cast<long double>(recipe.floorTopLayer) +
        static_cast<long double>(recipe.request.floorToFloorCells) * storey);
    level.wallHeightCells = recipe.request.floorToFloorCells;
    level.floorThicknessLayers = recipe.floorThicknessLayers;
    level.ceilingThicknessLayers = recipe.ceilingThicknessLayers;
    level.roofThicknessLayers = recipe.roofThicknessLayers;
    level.roofStyle = recipe.roofStyle;
    level.roofRidgeAxis = recipe.roofRidgeAxis;
    level.roofSlopeDirection = recipe.roofSlopeDirection;
    level.roofPitchDegrees = recipe.roofPitchDegrees;
    level.roofOverhangCells = recipe.roofOverhangCells;
    level.roofMaterial = recipe.roofMaterial;
    candidate.levels.push_back(std::move(level));
  }

  for (std::uint16_t storey = 0U; storey < blockout.storeyCount; ++storey) {
    const std::size_t levelIndex = firstLevelIndex + storey;
    for (std::size_t roomOrdinal = 0U; roomOrdinal < blockout.roomCount;
         ++roomOrdinal) {
      CreativeWorldLayoutRoom room;
      room.buildingIndex = buildingIndex;
      room.levelIndex = levelIndex;
      room.stableKey =
          mintCreativeWorldLayoutStableKey(candidate, nextOrdinal, "room");
      room.name = "Room " + std::to_string(candidate.rooms.size() + 1U);
      room.footprint = blockout.rooms[roomOrdinal];
      room.wallThicknessCells = recipe.request.wallThicknessCells;
      candidate.rooms.push_back(std::move(room));
    }
  }

  const std::vector<CreativeWorldLayoutSharedRoomEdgeSpan> sharedSpans =
      inspectCreativeWorldLayoutSharedRoomEdges(candidate);
  std::size_t interiorDoorOrdinal = 1U;
  std::size_t exteriorWindowOrdinal = 1U;
  for (std::uint16_t storey = 0U; storey < blockout.storeyCount; ++storey) {
    const std::size_t firstStoreyRoomIndex =
        firstRoomIndex + static_cast<std::size_t>(storey) * blockout.roomCount;
    for (std::size_t index = 0U; index < blockout.openingCount; ++index) {
      CreativeWorldLayoutBuildingBlockoutOpening intent =
          blockout.openings[index];
      if (storey > 0U &&
          intent.role ==
              CreativeWorldLayoutBuildingBlockoutOpeningRole::Entrance) {
        if (!recipe.request.facade.includeExteriorWindows) {
          continue;
        }
        intent.role =
            CreativeWorldLayoutBuildingBlockoutOpeningRole::ExteriorWindow;
        intent.kind = CreativeBuildingOpeningKind::Window;
      }
      CreativeWorldLayoutOpening opening;
      std::string reasonCode;
      if (!makeOpening(candidate, sharedSpans, firstStoreyRoomIndex, intent,
                       opening, reasonCode)) {
        setFailure(result,
                   CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
                   std::move(reasonCode));
        return result;
      }
      opening.stableKey = mintCreativeWorldLayoutStableKey(
          candidate, nextOrdinal,
          opening.kind == CreativeBuildingOpeningKind::Door ? "door"
                                                             : "window");
      std::size_t openingOrdinal = 1U;
      if (intent.role == CreativeWorldLayoutBuildingBlockoutOpeningRole::
                             InteriorConnection) {
        openingOrdinal = interiorDoorOrdinal++;
      } else if (intent.role ==
                 CreativeWorldLayoutBuildingBlockoutOpeningRole::
                     ExteriorWindow) {
        openingOrdinal = exteriorWindowOrdinal++;
      }
      opening.name = openingName(intent.role, openingOrdinal);
      candidate.openings.push_back(std::move(opening));
    }
  }

  if (blockout.hasVerticalConnector) {
    const CreativeWorldLayoutBuildingBlockoutVerticalConnector& intent =
        blockout.verticalConnector;
    for (std::uint16_t storey = 0U; storey + 1U < blockout.storeyCount;
         ++storey) {
      CreativeWorldLayoutVerticalConnector connector;
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
      connector.stableKey = mintCreativeWorldLayoutStableKey(
          candidate, nextOrdinal, connectorKeyPrefix(intent.kind));
      connector.name = std::string(connectorLabel(intent.kind)) + " " +
                       std::to_string(storey + 1U);
      connector.footprint = intent.footprint;
      candidate.verticalConnectors.push_back(std::move(connector));
      const CreativeWorldLayoutVerticalConnectorPlan connectorPlan =
          planCreativeWorldLayoutVerticalConnector(
              {}, candidate, candidate.verticalConnectors.size() - 1U);
      if (!connectorPlan.accepted) {
        setFailure(result,
                   CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
                   std::string(connectorPlan.reasonCode));
        return result;
      }
    }
  }

  const CreativeWorldLayoutRoomCompileResult expanded =
      expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted || creativeWorldLayoutHasInteriorRoomWindow(candidate)) {
    setFailure(
        result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
        expanded.accepted
            ? "creative_world_layout_building_blockout_interior_window"
            : expanded.reasonCode);
    return result;
  }

  CreativeWorldLayoutBuildingBlockoutProvenance provenance;
  provenance.present = true;
  provenance.valid = true;
  provenance.recipe = recipe;
  const CreativeWorldLayoutBuildingBlockoutFingerprint baseline =
      fingerprintCreativeWorldLayoutBuildingBlockout(candidate,
                                                     buildingIndex);
  if (!baseline.valid) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
               "creative_world_layout_building_blockout_provenance_invalid");
    return result;
  }
  provenance.instanceBaselineFingerprint = baseline.value;
  if (!setCreativeWorldLayoutBuildingBlockoutProvenance(
          candidate, buildingIndex, provenance)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
               "creative_world_layout_building_blockout_provenance_invalid");
    return result;
  }

  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutBuildingEditStatus::Ready;
  result.resultBuildingIndex = buildingIndex;
  result.nextStableOrdinal = nextOrdinal;
  result.edited = std::move(candidate);
  result.reasonCode = "creative_world_layout_building_blockout_ready";
  return result;
}

}  // namespace iggy3d::creative
