#include "app/iggy3d/creative/world/WorldLayoutBuildingUsability.hpp"

#include "app/iggy3d/creative/recipes/RampRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

bool validConfig(
    const CreativeWorldLayoutBuildingUsabilityConfig& config) noexcept {
  return std::isfinite(config.gridCellSizeMeters) &&
         config.gridCellSizeMeters > 0.0 &&
         std::isfinite(config.actorRadiusMeters) &&
         config.actorRadiusMeters > 0.0 &&
         std::isfinite(config.actorHeightMeters) &&
         config.actorHeightMeters > config.actorRadiusMeters * 2.0 &&
         std::isfinite(config.maximumStepMeters) &&
         config.maximumStepMeters >= 0.0 && std::isfinite(config.skinMeters) &&
         config.skinMeters >= 0.0 &&
         std::isfinite(config.maximumRampSlopeDegrees) &&
         config.maximumRampSlopeDegrees > 0.0 &&
         config.maximumRampSlopeDegrees < 90.0;
}

void appendIssue(CreativeWorldLayoutBuildingUsabilityReceipt& receipt,
                 CreativeWorldLayoutBuildingUsabilityIssueKind kind,
                 CreativeWorldLayoutTable table, std::size_t index,
                 std::size_t buildingIndex) noexcept {
  if (receipt.issueCount < receipt.issues.size()) {
    receipt.issues[receipt.issueCount++] = {kind, table, index, buildingIndex};
    return;
  }
  receipt.capacityExceeded = true;
  ++receipt.droppedIssueCount;
}

bool edgeRunsAlongX(CreativeWorldLayoutRoomEdge edge) noexcept {
  return edge == CreativeWorldLayoutRoomEdge::North ||
         edge == CreativeWorldLayoutRoomEdge::South;
}

double openingRunOrigin(const CreativeWorldLayoutRoom& room,
                        CreativeWorldLayoutRoomEdge edge) noexcept {
  return edgeRunsAlongX(edge) ? static_cast<double>(room.footprint.minimum.x)
                              : static_cast<double>(room.footprint.minimum.z);
}

double spanRunMinimum(const CreativeWorldLayoutSharedRoomEdgeSpan& span,
                      bool alongX) noexcept {
  return alongX ? static_cast<double>(std::min(span.start.x, span.end.x))
                : static_cast<double>(std::min(span.start.z, span.end.z));
}

double spanRunMaximum(const CreativeWorldLayoutSharedRoomEdgeSpan& span,
                      bool alongX) noexcept {
  return alongX ? static_cast<double>(std::max(span.start.x, span.end.x))
                : static_cast<double>(std::max(span.start.z, span.end.z));
}

std::size_t sharedRoomForOpening(
    const CreativeWorldLayout& layout,
    const std::vector<CreativeWorldLayoutSharedRoomEdgeSpan>& sharedSpans,
    const CreativeWorldLayoutOpening& opening) noexcept {
  if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
      opening.roomIndex >= layout.rooms.size() ||
      !std::isfinite(opening.centerOffsetCells) ||
      !std::isfinite(opening.widthCells) || opening.widthCells <= 0.0) {
    return kInvalidCreativeWorldLayoutIndex;
  }
  if (opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex) {
    if (opening.roomTopologyEdgeIndex >= layout.topologyEdges.size()) {
      return kInvalidCreativeWorldLayoutIndex;
    }
    for (const CreativeWorldLayoutRoomBoundary& boundary :
         layout.roomBoundaries) {
      if (boundary.topologyEdgeIndex == opening.roomTopologyEdgeIndex &&
          boundary.roomIndex != opening.roomIndex &&
          boundary.roomIndex < layout.rooms.size()) {
        return boundary.roomIndex;
      }
    }
    return kInvalidCreativeWorldLayoutIndex;
  }
  if (opening.roomEdge >= CreativeWorldLayoutRoomEdge::Count) {
    return kInvalidCreativeWorldLayoutIndex;
  }
  const CreativeWorldLayoutRoom& room = layout.rooms[opening.roomIndex];
  const bool alongX = edgeRunsAlongX(opening.roomEdge);
  const double center = openingRunOrigin(room, opening.roomEdge) +
                        opening.centerOffsetCells;
  const double openingBegin = center - opening.widthCells * 0.5;
  const double openingEnd = center + opening.widthCells * 0.5;
  for (const CreativeWorldLayoutSharedRoomEdgeSpan& span : sharedSpans) {
    std::size_t otherRoom = kInvalidCreativeWorldLayoutIndex;
    if (span.firstRoomIndex == opening.roomIndex &&
        span.firstRoomEdge == opening.roomEdge) {
      otherRoom = span.secondRoomIndex;
    } else if (span.secondRoomIndex == opening.roomIndex &&
               span.secondRoomEdge == opening.roomEdge) {
      otherRoom = span.firstRoomIndex;
    } else {
      continue;
    }
    if (std::max(openingBegin, spanRunMinimum(span, alongX)) +
            kGeometryEpsilon <
        std::min(openingEnd, spanRunMaximum(span, alongX))) {
      return otherRoom;
    }
  }
  return kInvalidCreativeWorldLayoutIndex;
}

std::size_t openingBuildingIndex(const CreativeWorldLayout& layout,
                                 const CreativeWorldLayoutOpening& opening)
    noexcept {
  if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
      opening.roomIndex < layout.rooms.size()) {
    return layout.rooms[opening.roomIndex].buildingIndex;
  }
  if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex < layout.walls.size()) {
    return layout.walls[opening.wallIndex].buildingIndex;
  }
  return kInvalidCreativeWorldLayoutIndex;
}

bool doorHasClearance(
    const CreativeWorldLayoutOpening& opening,
    const CreativeWorldLayoutBuildingUsabilityConfig& config) noexcept {
  return evaluateCreativeWorldLayoutOpeningClearance(
             {&opening, config.gridCellSizeMeters, config.actorRadiusMeters,
              config.actorHeightMeters, config.maximumStepMeters,
              config.skinMeters})
      .traversable;
}

bool rectContains(CreativeWorldLayoutRect outer,
                  CreativeWorldLayoutRect inner) noexcept {
  return inner.minimum.x >= outer.minimum.x &&
         inner.minimum.z >= outer.minimum.z &&
         inner.maximum.x <= outer.maximum.x &&
         inner.maximum.z <= outer.maximum.z &&
         inner.minimum.x < inner.maximum.x &&
         inner.minimum.z < inner.maximum.z;
}

bool connectorOwnershipValid(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutVerticalConnector& connector) noexcept {
  if (connector.buildingIndex >= layout.buildings.size() ||
      connector.lowerRoomIndex >= layout.rooms.size() ||
      connector.upperRoomIndex >= layout.rooms.size() ||
      connector.lowerRoomIndex == connector.upperRoomIndex ||
      connector.direction >= CreativeWorldLayoutVerticalDirection::Count ||
      connector.kind >= CreativeWorldLayoutVerticalConnectorKind::Count) {
    return false;
  }
  const CreativeWorldLayoutRoom& lower =
      layout.rooms[connector.lowerRoomIndex];
  const CreativeWorldLayoutRoom& upper =
      layout.rooms[connector.upperRoomIndex];
  if (lower.buildingIndex != connector.buildingIndex ||
      upper.buildingIndex != connector.buildingIndex ||
      lower.levelIndex >= layout.levels.size() ||
      upper.levelIndex >= layout.levels.size() ||
      lower.levelIndex == upper.levelIndex ||
      !rectContains(lower.footprint, connector.footprint) ||
      !rectContains(upper.footprint, connector.footprint)) {
    return false;
  }
  const CreativeWorldLayoutLevel& lowerLevel = layout.levels[lower.levelIndex];
  const CreativeWorldLayoutLevel& upperLevel = layout.levels[upper.levelIndex];
  const double riseCells = upperLevel.floorTopLayer - lowerLevel.floorTopLayer;
  return lowerLevel.buildingIndex == connector.buildingIndex &&
         upperLevel.buildingIndex == connector.buildingIndex &&
         std::isfinite(riseCells) && riseCells > 0.0;
}

bool connectorHasClearance(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutVerticalConnector& connector,
    const CreativeWorldLayoutBuildingUsabilityConfig& config) noexcept {
  const CreativeWorldLayoutRoom& lower =
      layout.rooms[connector.lowerRoomIndex];
  const CreativeWorldLayoutRoom& upper =
      layout.rooms[connector.upperRoomIndex];
  const CreativeWorldLayoutLevel& lowerLevel = layout.levels[lower.levelIndex];
  const CreativeWorldLayoutLevel& upperLevel = layout.levels[upper.levelIndex];
  const double riseCells = upperLevel.floorTopLayer - lowerLevel.floorTopLayer;
  const double widthCells =
      connector.direction == CreativeWorldLayoutVerticalDirection::PositiveX ||
              connector.direction ==
                  CreativeWorldLayoutVerticalDirection::NegativeX
          ? static_cast<double>(connector.footprint.maximum.z -
                                connector.footprint.minimum.z)
          : static_cast<double>(connector.footprint.maximum.x -
                                connector.footprint.minimum.x);
  const double runCells =
      connector.direction == CreativeWorldLayoutVerticalDirection::PositiveX ||
              connector.direction ==
                  CreativeWorldLayoutVerticalDirection::NegativeX
          ? static_cast<double>(connector.footprint.maximum.x -
                                connector.footprint.minimum.x)
          : static_cast<double>(connector.footprint.maximum.z -
                                connector.footprint.minimum.z);
  const double minimumWidth =
      config.actorRadiusMeters * 2.0 + config.skinMeters * 2.0;
  if (widthCells * config.gridCellSizeMeters + kGeometryEpsilon <
          minimumWidth ||
      runCells <= 0.0) {
    return false;
  }
  if (connector.kind == CreativeWorldLayoutVerticalConnectorKind::Ramp) {
    CreativeRampRecipeRequest rampRequest;
    rampRequest.authoredBounds = {
        {0.0, 0.0, 0.0},
        {widthCells * config.gridCellSizeMeters,
         riseCells * config.gridCellSizeMeters,
         runCells * config.gridCellSizeMeters}};
    rampRequest.transform.position =
        measureCreativeBounds(rampRequest.authoredBounds).center;
    rampRequest.availableHeadroomMeters =
        static_cast<double>(upperLevel.wallHeightCells) *
        config.gridCellSizeMeters;
    rampRequest.maximumWalkableSlopeDegrees =
        config.maximumRampSlopeDegrees;
    rampRequest.material = connector.material;
    if (!planCreativeRamp(rampRequest).accepted) {
      return false;
    }
  }
  return true;
}

bool hasGeneratedObject(const CreativeDocument& document,
                        const CreativeWorldLayout& layout,
                        CreativeWorldLayoutTable table, std::size_t index,
                        CreativeObjectKind kind) {
  return std::any_of(
      document.objects().begin(), document.objects().end(),
      [&](const CreativeObject& object) {
        if (object.kind != kind) {
          return false;
        }
        const CreativeWorldLayoutObjectProvenance provenance =
            resolveCreativeWorldLayoutObjectProvenance(layout, object);
        return provenance.owned && provenance.table == table &&
               provenance.index == index;
      });
}

bool levelHasBuildingRoom(const CreativeWorldLayout& layout,
                          std::size_t buildingIndex,
                          std::size_t levelIndex) noexcept {
  return std::any_of(layout.rooms.begin(), layout.rooms.end(),
                     [buildingIndex, levelIndex](const auto& room) {
                       return room.buildingIndex == buildingIndex &&
                              room.levelIndex == levelIndex;
                     });
}

std::size_t lowestBuildingLevel(const CreativeWorldLayout& layout,
                                std::size_t buildingIndex) noexcept {
  std::size_t result = kInvalidCreativeWorldLayoutIndex;
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const CreativeWorldLayoutLevel& level = layout.levels[index];
    if (level.buildingIndex != buildingIndex ||
        !levelHasBuildingRoom(layout, buildingIndex, index)) {
      continue;
    }
    if (result == kInvalidCreativeWorldLayoutIndex ||
        level.floorTopLayer < layout.levels[result].floorTopLayer) {
      result = index;
    }
  }
  return result;
}

}  // namespace

std::string_view creativeWorldLayoutBuildingUsabilityReasonCode(
    CreativeWorldLayoutBuildingUsabilityIssueKind kind) noexcept {
  switch (kind) {
    case CreativeWorldLayoutBuildingUsabilityIssueKind::BuildingWithoutRooms:
      return "creative_world_layout_building_without_rooms";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::MissingExteriorEntrance:
      return "creative_world_layout_building_exterior_entrance_missing";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::OpeningClearanceTooSmall:
      return "creative_world_layout_building_opening_clearance_too_small";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::InvalidConnector:
      return "creative_world_layout_building_connector_invalid";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::ConnectorClearanceTooSmall:
      return "creative_world_layout_building_connector_clearance_too_small";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::MissingVerticalConnection:
      return "creative_world_layout_building_vertical_connection_missing";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::DisconnectedRoom:
      return "creative_world_layout_building_room_disconnected";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::MissingGeneratedFloor:
      return "creative_world_layout_building_generated_floor_missing";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::MissingGeneratedOpening:
      return "creative_world_layout_building_generated_opening_missing";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::MissingGeneratedConnector:
      return "creative_world_layout_building_generated_connector_missing";
    case CreativeWorldLayoutBuildingUsabilityIssueKind::Count:
      break;
  }
  return "creative_world_layout_building_usability_issue_invalid";
}

CreativeWorldLayoutBuildingUsabilityReceipt
validateCreativeWorldLayoutBuildingUsability(
    const CreativeWorldLayoutBuildingUsabilityRequest& request) {
  CreativeWorldLayoutBuildingUsabilityReceipt receipt;
  receipt.requested = true;
  if (request.layout == nullptr || !validConfig(request.config)) {
    receipt.status = CreativeWorldLayoutBuildingUsabilityStatus::InvalidRequest;
    return receipt;
  }

  const CreativeWorldLayout& layout = *request.layout;
  receipt.accepted = true;
  receipt.buildingCount = layout.buildings.size();
  receipt.roomCount = layout.rooms.size();
  const std::vector<CreativeWorldLayoutSharedRoomEdgeSpan> sharedSpans =
      inspectCreativeWorldLayoutSharedRoomEdges(layout);
  std::vector<bool> buildingHasIssue(layout.buildings.size(), false);
  std::vector<std::size_t> openingBuildings(
      layout.openings.size(), kInvalidCreativeWorldLayoutIndex);
  std::vector<std::size_t> openingSharedRooms(
      layout.openings.size(), kInvalidCreativeWorldLayoutIndex);
  std::vector<bool> usableDoors(layout.openings.size(), false);
  for (std::size_t openingIndex = 0U;
       openingIndex < layout.openings.size(); ++openingIndex) {
    const CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
    openingBuildings[openingIndex] = openingBuildingIndex(layout, opening);
    if (opening.kind != CreativeBuildingOpeningKind::Door) {
      continue;
    }
    usableDoors[openingIndex] = doorHasClearance(opening, request.config);
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      openingSharedRooms[openingIndex] =
          sharedRoomForOpening(layout, sharedSpans, opening);
    }
  }

  if (request.generatedDocument != nullptr) {
    for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
         ++roomIndex) {
      const std::size_t buildingIndex = layout.rooms[roomIndex].buildingIndex;
      if (buildingIndex < layout.buildings.size() &&
          !hasGeneratedObject(*request.generatedDocument, layout,
                              CreativeWorldLayoutTable::Room, roomIndex,
                              CreativeObjectKind::Floor)) {
        appendIssue(
            receipt,
            CreativeWorldLayoutBuildingUsabilityIssueKind::MissingGeneratedFloor,
            CreativeWorldLayoutTable::Room, roomIndex, buildingIndex);
        buildingHasIssue[buildingIndex] = true;
      }
    }
    for (std::size_t openingIndex = 0U;
         openingIndex < layout.openings.size(); ++openingIndex) {
      const CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
      const std::size_t buildingIndex = openingBuildings[openingIndex];
      if (buildingIndex >= layout.buildings.size() || !opening.includeInsert) {
        continue;
      }
      const CreativeObjectKind kind =
          opening.kind == CreativeBuildingOpeningKind::Door
              ? CreativeObjectKind::Door
              : CreativeObjectKind::Window;
      if (!hasGeneratedObject(*request.generatedDocument, layout,
                              CreativeWorldLayoutTable::Opening, openingIndex,
                              kind)) {
        appendIssue(
            receipt,
            CreativeWorldLayoutBuildingUsabilityIssueKind::
                MissingGeneratedOpening,
            CreativeWorldLayoutTable::Opening, openingIndex, buildingIndex);
        buildingHasIssue[buildingIndex] = true;
      }
    }
    for (std::size_t connectorIndex = 0U;
         connectorIndex < layout.verticalConnectors.size(); ++connectorIndex) {
      const CreativeWorldLayoutVerticalConnector& connector =
          layout.verticalConnectors[connectorIndex];
      if (connector.buildingIndex >= layout.buildings.size() ||
          connector.kind >= CreativeWorldLayoutVerticalConnectorKind::Count) {
        continue;
      }
      const CreativeObjectKind kind =
          connector.kind == CreativeWorldLayoutVerticalConnectorKind::Stair
              ? CreativeObjectKind::Stair
              : CreativeObjectKind::Ramp;
      if (!hasGeneratedObject(*request.generatedDocument, layout,
                              CreativeWorldLayoutTable::VerticalConnector,
                              connectorIndex, kind)) {
        appendIssue(
            receipt,
            CreativeWorldLayoutBuildingUsabilityIssueKind::
                MissingGeneratedConnector,
            CreativeWorldLayoutTable::VerticalConnector, connectorIndex,
            connector.buildingIndex);
        buildingHasIssue[connector.buildingIndex] = true;
      }
    }
  }

  for (std::size_t openingIndex = 0U;
       openingIndex < layout.openings.size(); ++openingIndex) {
    const std::size_t buildingIndex = openingBuildings[openingIndex];
    if (buildingIndex < layout.buildings.size() &&
        layout.openings[openingIndex].kind ==
            CreativeBuildingOpeningKind::Door &&
        !usableDoors[openingIndex]) {
      appendIssue(
          receipt,
          CreativeWorldLayoutBuildingUsabilityIssueKind::OpeningClearanceTooSmall,
          CreativeWorldLayoutTable::Opening, openingIndex, buildingIndex);
      buildingHasIssue[buildingIndex] = true;
    }
  }

  std::vector<bool> usableConnectors(layout.verticalConnectors.size(), false);

  for (std::size_t connectorIndex = 0U;
       connectorIndex < layout.verticalConnectors.size(); ++connectorIndex) {
    const CreativeWorldLayoutVerticalConnector& connector =
        layout.verticalConnectors[connectorIndex];
    if (!connectorOwnershipValid(layout, connector)) {
      appendIssue(receipt,
                  CreativeWorldLayoutBuildingUsabilityIssueKind::InvalidConnector,
                  CreativeWorldLayoutTable::VerticalConnector, connectorIndex,
                  connector.buildingIndex);
      if (connector.buildingIndex < buildingHasIssue.size()) {
        buildingHasIssue[connector.buildingIndex] = true;
      }
      continue;
    }
    usableConnectors[connectorIndex] =
        connectorHasClearance(layout, connector, request.config);
    if (!usableConnectors[connectorIndex]) {
      appendIssue(
          receipt,
          CreativeWorldLayoutBuildingUsabilityIssueKind::ConnectorClearanceTooSmall,
          CreativeWorldLayoutTable::VerticalConnector, connectorIndex,
          connector.buildingIndex);
      buildingHasIssue[connector.buildingIndex] = true;
    }
  }

  for (std::size_t buildingIndex = 0U;
       buildingIndex < layout.buildings.size(); ++buildingIndex) {
    const std::size_t issueSignalBefore =
        receipt.issueCount + receipt.droppedIssueCount;
    const bool hasRoom = std::any_of(
        layout.rooms.begin(), layout.rooms.end(),
        [buildingIndex](const auto& room) {
          return room.buildingIndex == buildingIndex;
        });
    if (!hasRoom) {
      appendIssue(
          receipt,
          CreativeWorldLayoutBuildingUsabilityIssueKind::BuildingWithoutRooms,
          CreativeWorldLayoutTable::Building, buildingIndex, buildingIndex);
      buildingHasIssue[buildingIndex] = true;
      continue;
    }

    std::vector<bool> reachable(layout.rooms.size(), false);
    bool hasExteriorEntrance = false;
    for (std::size_t openingIndex = 0U;
         openingIndex < layout.openings.size(); ++openingIndex) {
      const CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
      if (openingBuildings[openingIndex] != buildingIndex ||
          !usableDoors[openingIndex]) {
        continue;
      }
      if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
          opening.roomIndex >= layout.rooms.size()) {
        continue;
      }
      if (openingSharedRooms[openingIndex] ==
          kInvalidCreativeWorldLayoutIndex) {
        hasExteriorEntrance = true;
        reachable[opening.roomIndex] = true;
      }
    }

    if (!hasExteriorEntrance) {
      appendIssue(
          receipt,
          CreativeWorldLayoutBuildingUsabilityIssueKind::MissingExteriorEntrance,
          CreativeWorldLayoutTable::Building, buildingIndex, buildingIndex);
    } else {
      bool changed = true;
      while (changed) {
        changed = false;
        for (std::size_t openingIndex = 0U;
             openingIndex < layout.openings.size(); ++openingIndex) {
          const CreativeWorldLayoutOpening& opening =
              layout.openings[openingIndex];
          if (openingBuildings[openingIndex] != buildingIndex ||
              !usableDoors[openingIndex] ||
              opening.hostKind !=
                  CreativeWorldLayoutOpeningHostKind::RoomEdge ||
              opening.roomIndex >= layout.rooms.size()) {
            continue;
          }
          const std::size_t otherRoom = openingSharedRooms[openingIndex];
          if (otherRoom >= layout.rooms.size() ||
              layout.rooms[otherRoom].buildingIndex != buildingIndex) {
            continue;
          }
          if (reachable[opening.roomIndex] != reachable[otherRoom]) {
            reachable[opening.roomIndex] = true;
            reachable[otherRoom] = true;
            changed = true;
          }
        }
        for (std::size_t connectorIndex = 0U;
             connectorIndex < layout.verticalConnectors.size();
             ++connectorIndex) {
          const CreativeWorldLayoutVerticalConnector& connector =
              layout.verticalConnectors[connectorIndex];
          if (!usableConnectors[connectorIndex] ||
              connector.buildingIndex != buildingIndex) {
            continue;
          }
          if (reachable[connector.lowerRoomIndex] !=
              reachable[connector.upperRoomIndex]) {
            reachable[connector.lowerRoomIndex] = true;
            reachable[connector.upperRoomIndex] = true;
            changed = true;
          }
        }
      }
      for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
           ++roomIndex) {
        if (layout.rooms[roomIndex].buildingIndex != buildingIndex) {
          continue;
        }
        if (reachable[roomIndex]) {
          ++receipt.reachableRoomCount;
        } else {
          appendIssue(
              receipt,
              CreativeWorldLayoutBuildingUsabilityIssueKind::DisconnectedRoom,
              CreativeWorldLayoutTable::Room, roomIndex, buildingIndex);
        }
      }
    }

    const std::size_t lowestLevel = lowestBuildingLevel(layout, buildingIndex);
    for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
         ++levelIndex) {
      if (levelIndex == lowestLevel ||
          layout.levels[levelIndex].buildingIndex != buildingIndex ||
          !levelHasBuildingRoom(layout, buildingIndex, levelIndex)) {
        continue;
      }
      bool connectedFromBelow = false;
      for (std::size_t connectorIndex = 0U;
           connectorIndex < layout.verticalConnectors.size();
           ++connectorIndex) {
        const CreativeWorldLayoutVerticalConnector& connector =
            layout.verticalConnectors[connectorIndex];
        if (usableConnectors[connectorIndex] &&
            connector.buildingIndex == buildingIndex &&
            connector.upperRoomIndex < layout.rooms.size() &&
            layout.rooms[connector.upperRoomIndex].levelIndex == levelIndex) {
          connectedFromBelow = true;
          break;
        }
      }
      if (!connectedFromBelow) {
        appendIssue(
            receipt,
            CreativeWorldLayoutBuildingUsabilityIssueKind::MissingVerticalConnection,
            CreativeWorldLayoutTable::Level, levelIndex, buildingIndex);
      }
    }

    if (receipt.issueCount + receipt.droppedIssueCount > issueSignalBefore) {
      buildingHasIssue[buildingIndex] = true;
    }
  }

  receipt.usableBuildingCount = static_cast<std::size_t>(std::count(
      buildingHasIssue.begin(), buildingHasIssue.end(), false));

  receipt.usable = receipt.issueCount == 0U && !receipt.capacityExceeded;
  receipt.status = receipt.usable
                       ? CreativeWorldLayoutBuildingUsabilityStatus::Ready
                       : CreativeWorldLayoutBuildingUsabilityStatus::IssuesFound;
  return receipt;
}

}  // namespace iggy3d::creative
