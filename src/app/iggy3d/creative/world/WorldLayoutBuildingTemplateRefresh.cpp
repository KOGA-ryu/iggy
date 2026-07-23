#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

bool openingOwnedByBuilding(const CreativeWorldLayout& layout,
                            const CreativeWorldLayoutOpening& opening,
                            std::size_t buildingIndex) noexcept {
  return opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge
             ? layout.rooms[opening.roomIndex].buildingIndex == buildingIndex
             : layout.walls[opening.wallIndex].buildingIndex == buildingIndex;
}

struct OwnedIdentity {
  std::string stableKey;
  std::string name;
};

template <typename Rows, typename Predicate>
std::vector<OwnedIdentity> ownedIdentities(const Rows& rows,
                                           Predicate isOwned) {
  std::vector<OwnedIdentity> result;
  for (const auto& row : rows) {
    if (isOwned(row)) {
      result.push_back({row.stableKey, row.name});
    }
  }
  return result;
}

template <typename Rows, typename Predicate>
std::vector<std::string> ownedStableKeys(const Rows& rows,
                                         Predicate isOwned) {
  std::vector<std::string> result;
  for (const auto& row : rows) {
    if (isOwned(row)) {
      result.push_back(row.stableKey);
    }
  }
  return result;
}

void applyReusedOrMintedIdentity(
    CreativeWorldLayout& layout, std::uint64_t& nextStableOrdinal,
    const std::vector<OwnedIdentity>& oldIdentities, std::size_t ordinal,
    std::string_view prefix, bool preserveExistingNames,
    std::string& stableKey, std::string& name) {
  if (ordinal < oldIdentities.size()) {
    stableKey = oldIdentities[ordinal].stableKey;
    if (preserveExistingNames) {
      name = oldIdentities[ordinal].name;
    }
    return;
  }
  stableKey =
      mintCreativeWorldLayoutStableKey(layout, nextStableOrdinal, prefix);
}

void applyReusedOrMintedStableKey(
    CreativeWorldLayout& layout, std::uint64_t& nextStableOrdinal,
    const std::vector<std::string>& oldStableKeys, std::size_t ordinal,
    std::string_view prefix, std::string& stableKey) {
  if (ordinal < oldStableKeys.size()) {
    stableKey = oldStableKeys[ordinal];
    return;
  }
  stableKey =
      mintCreativeWorldLayoutStableKey(layout, nextStableOrdinal, prefix);
}

bool replaceBuildingContents(CreativeWorldLayout& layout,
                             std::size_t buildingIndex,
                             const CreativeWorldLayout& positioned,
                             std::uint64_t& nextStableOrdinal,
                             bool preserveExistingNames) {
  if (buildingIndex >= layout.buildings.size() ||
      positioned.buildings.size() != 1U ||
      !validCreativeWorldLayoutBuildingOwnership(layout) ||
      !validCreativeWorldLayoutBuildingOwnership(positioned)) {
    return false;
  }

  const std::vector<OwnedIdentity> oldLevelIdentities = ownedIdentities(
      layout.levels,
      [buildingIndex](const auto& row) {
        return row.buildingIndex == buildingIndex;
      });
  const std::vector<OwnedIdentity> oldRoomIdentities = ownedIdentities(
      layout.rooms,
      [buildingIndex](const auto& row) {
        return row.buildingIndex == buildingIndex;
      });
  const std::vector<OwnedIdentity> oldVerticalConnectorIdentities =
      ownedIdentities(layout.verticalConnectors,
                      [buildingIndex](const auto& row) {
                        return row.buildingIndex == buildingIndex;
                      });
  const std::vector<OwnedIdentity> oldBoxIdentities = ownedIdentities(
      layout.boxes,
      [buildingIndex](const auto& row) {
        return row.buildingIndex == buildingIndex;
      });
  const std::vector<OwnedIdentity> oldWallIdentities = ownedIdentities(
      layout.walls,
      [buildingIndex](const auto& row) {
        return row.buildingIndex == buildingIndex;
      });
  const std::vector<OwnedIdentity> oldOpeningIdentities = ownedIdentities(
      layout.openings,
      [&](const auto& row) {
        return openingOwnedByBuilding(layout, row, buildingIndex);
      });
  const auto oldLevelOwned = [&](std::size_t levelIndex) {
    return levelIndex < layout.levels.size() &&
           layout.levels[levelIndex].buildingIndex == buildingIndex;
  };
  const std::vector<OwnedIdentity> oldRoofApertureIdentities =
      ownedIdentities(
          layout.roofApertures,
          [&](const CreativeWorldLayoutRoofAperture& row) {
            return oldLevelOwned(row.levelIndex);
          });
  const std::vector<std::string> oldTopologyVertexKeys = ownedStableKeys(
      layout.topologyVertices,
      [&](const CreativeWorldLayoutTopologyVertex& vertex) {
        return oldLevelOwned(vertex.levelIndex);
      });
  const std::vector<std::string> oldTopologyEdgeKeys = ownedStableKeys(
      layout.topologyEdges,
      [&](const CreativeWorldLayoutTopologyEdge& edge) {
        return oldLevelOwned(edge.levelIndex);
      });
  const std::string buildingStableKey =
      layout.buildings[buildingIndex].stableKey;
  const std::string buildingName = layout.buildings[buildingIndex].name;

  CreativeWorldLayoutBuilding replacement = positioned.buildings[0];
  replacement.stableKey = buildingStableKey;
  if (preserveExistingNames) {
    replacement.name = buildingName;
  }
  layout.buildings[buildingIndex] = std::move(replacement);

  std::vector<std::size_t> oldLevelMap(layout.levels.size(),
                                       kInvalidCreativeWorldLayoutIndex);
  std::vector<CreativeWorldLayoutLevel> levels;
  levels.reserve(layout.levels.size() + positioned.levels.size());
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    if (layout.levels[index].buildingIndex == buildingIndex) {
      continue;
    }
    oldLevelMap[index] = levels.size();
    levels.push_back(layout.levels[index]);
  }
  std::vector<std::size_t> newLevelMap(positioned.levels.size(),
                                       kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.levels.size(); ++index) {
    CreativeWorldLayoutLevel level = positioned.levels[index];
    level.buildingIndex = buildingIndex;
    applyReusedOrMintedIdentity(
        layout, nextStableOrdinal, oldLevelIdentities, index, "level",
        preserveExistingNames, level.stableKey, level.name);
    newLevelMap[index] = levels.size();
    levels.push_back(std::move(level));
  }

  std::vector<CreativeWorldLayoutRoofAperture> roofApertures;
  roofApertures.reserve(layout.roofApertures.size() +
                        positioned.roofApertures.size());
  for (CreativeWorldLayoutRoofAperture aperture : layout.roofApertures) {
    if (oldLevelOwned(aperture.levelIndex)) {
      continue;
    }
    aperture.levelIndex = oldLevelMap[aperture.levelIndex];
    roofApertures.push_back(std::move(aperture));
  }
  for (std::size_t index = 0U;
       index < positioned.roofApertures.size(); ++index) {
    CreativeWorldLayoutRoofAperture aperture =
        positioned.roofApertures[index];
    aperture.levelIndex = newLevelMap[aperture.levelIndex];
    applyReusedOrMintedIdentity(
        layout, nextStableOrdinal, oldRoofApertureIdentities, index,
        aperture.kind == CreativeStructuralRoofApertureKind::Skylight
            ? "skylight"
            : "chimney_clearance",
        preserveExistingNames, aperture.stableKey, aperture.name);
    roofApertures.push_back(std::move(aperture));
  }

  std::vector<std::size_t> oldRoomMap(layout.rooms.size(),
                                      kInvalidCreativeWorldLayoutIndex);
  std::vector<CreativeWorldLayoutRoom> rooms;
  rooms.reserve(layout.rooms.size() + positioned.rooms.size());
  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    if (layout.rooms[index].buildingIndex == buildingIndex) {
      continue;
    }
    oldRoomMap[index] = rooms.size();
    CreativeWorldLayoutRoom room = layout.rooms[index];
    room.levelIndex = oldLevelMap[room.levelIndex];
    rooms.push_back(std::move(room));
  }
  std::vector<std::size_t> newRoomMap(positioned.rooms.size(),
                                      kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.rooms.size(); ++index) {
    CreativeWorldLayoutRoom room = positioned.rooms[index];
    room.buildingIndex = buildingIndex;
    room.levelIndex = newLevelMap[room.levelIndex];
    applyReusedOrMintedIdentity(
        layout, nextStableOrdinal, oldRoomIdentities, index, "room",
        preserveExistingNames, room.stableKey, room.name);
    newRoomMap[index] = rooms.size();
    rooms.push_back(std::move(room));
  }

  std::vector<std::size_t> oldTopologyVertexMap(
      layout.topologyVertices.size(), kInvalidCreativeWorldLayoutIndex);
  std::vector<CreativeWorldLayoutTopologyVertex> topologyVertices;
  topologyVertices.reserve(layout.topologyVertices.size() +
                           positioned.topologyVertices.size());
  for (std::size_t index = 0U; index < layout.topologyVertices.size();
       ++index) {
    CreativeWorldLayoutTopologyVertex vertex =
        layout.topologyVertices[index];
    if (oldLevelOwned(vertex.levelIndex)) {
      continue;
    }
    vertex.levelIndex = oldLevelMap[vertex.levelIndex];
    oldTopologyVertexMap[index] = topologyVertices.size();
    topologyVertices.push_back(std::move(vertex));
  }
  std::vector<std::size_t> newTopologyVertexMap(
      positioned.topologyVertices.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U;
       index < positioned.topologyVertices.size(); ++index) {
    CreativeWorldLayoutTopologyVertex vertex =
        positioned.topologyVertices[index];
    vertex.levelIndex = newLevelMap[vertex.levelIndex];
    applyReusedOrMintedStableKey(
        layout, nextStableOrdinal, oldTopologyVertexKeys, index,
        "wall_vertex", vertex.stableKey);
    newTopologyVertexMap[index] = topologyVertices.size();
    topologyVertices.push_back(std::move(vertex));
  }

  std::vector<std::size_t> oldTopologyEdgeMap(
      layout.topologyEdges.size(), kInvalidCreativeWorldLayoutIndex);
  std::vector<CreativeWorldLayoutTopologyEdge> topologyEdges;
  topologyEdges.reserve(layout.topologyEdges.size() +
                        positioned.topologyEdges.size());
  for (std::size_t index = 0U; index < layout.topologyEdges.size(); ++index) {
    CreativeWorldLayoutTopologyEdge edge = layout.topologyEdges[index];
    if (oldLevelOwned(edge.levelIndex)) {
      continue;
    }
    edge.levelIndex = oldLevelMap[edge.levelIndex];
    edge.startVertexIndex = oldTopologyVertexMap[edge.startVertexIndex];
    edge.endVertexIndex = oldTopologyVertexMap[edge.endVertexIndex];
    oldTopologyEdgeMap[index] = topologyEdges.size();
    topologyEdges.push_back(std::move(edge));
  }
  std::vector<std::size_t> newTopologyEdgeMap(
      positioned.topologyEdges.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.topologyEdges.size();
       ++index) {
    CreativeWorldLayoutTopologyEdge edge = positioned.topologyEdges[index];
    edge.levelIndex = newLevelMap[edge.levelIndex];
    edge.startVertexIndex = newTopologyVertexMap[edge.startVertexIndex];
    edge.endVertexIndex = newTopologyVertexMap[edge.endVertexIndex];
    applyReusedOrMintedStableKey(
        layout, nextStableOrdinal, oldTopologyEdgeKeys, index, "wall",
        edge.stableKey);
    newTopologyEdgeMap[index] = topologyEdges.size();
    topologyEdges.push_back(std::move(edge));
  }

  std::vector<CreativeWorldLayoutRoomBoundary> roomBoundaries;
  roomBoundaries.reserve(layout.roomBoundaries.size() +
                         positioned.roomBoundaries.size());
  for (CreativeWorldLayoutRoomBoundary boundary : layout.roomBoundaries) {
    if (oldRoomMap[boundary.roomIndex] ==
        kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    boundary.roomIndex = oldRoomMap[boundary.roomIndex];
    boundary.topologyEdgeIndex =
        oldTopologyEdgeMap[boundary.topologyEdgeIndex];
    roomBoundaries.push_back(std::move(boundary));
  }
  for (CreativeWorldLayoutRoomBoundary boundary :
       positioned.roomBoundaries) {
    boundary.roomIndex = newRoomMap[boundary.roomIndex];
    boundary.topologyEdgeIndex =
        newTopologyEdgeMap[boundary.topologyEdgeIndex];
    roomBoundaries.push_back(std::move(boundary));
  }

  std::vector<CreativeWorldLayoutVerticalConnector> verticalConnectors;
  verticalConnectors.reserve(layout.verticalConnectors.size() +
                             positioned.verticalConnectors.size());
  for (CreativeWorldLayoutVerticalConnector connector :
       layout.verticalConnectors) {
    if (connector.buildingIndex == buildingIndex) {
      continue;
    }
    connector.lowerRoomIndex = oldRoomMap[connector.lowerRoomIndex];
    connector.upperRoomIndex = oldRoomMap[connector.upperRoomIndex];
    verticalConnectors.push_back(std::move(connector));
  }
  for (std::size_t index = 0U; index < positioned.verticalConnectors.size();
       ++index) {
    CreativeWorldLayoutVerticalConnector connector =
        positioned.verticalConnectors[index];
    connector.buildingIndex = buildingIndex;
    connector.lowerRoomIndex = newRoomMap[connector.lowerRoomIndex];
    connector.upperRoomIndex = newRoomMap[connector.upperRoomIndex];
    applyReusedOrMintedIdentity(
        layout, nextStableOrdinal, oldVerticalConnectorIdentities, index,
        "vertical_connector", preserveExistingNames, connector.stableKey,
        connector.name);
    verticalConnectors.push_back(std::move(connector));
  }

  std::vector<CreativeWorldLayoutBox> boxes;
  boxes.reserve(layout.boxes.size() + positioned.boxes.size());
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex != buildingIndex) {
      boxes.push_back(box);
    }
  }
  for (std::size_t index = 0U; index < positioned.boxes.size(); ++index) {
    CreativeWorldLayoutBox box = positioned.boxes[index];
    box.buildingIndex = buildingIndex;
    applyReusedOrMintedIdentity(
        layout, nextStableOrdinal, oldBoxIdentities, index, "floor",
        preserveExistingNames, box.stableKey, box.name);
    boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> oldWallMap(layout.walls.size(),
                                      kInvalidCreativeWorldLayoutIndex);
  std::vector<CreativeWorldLayoutWall> walls;
  walls.reserve(layout.walls.size() + positioned.walls.size());
  for (std::size_t index = 0U; index < layout.walls.size(); ++index) {
    if (layout.walls[index].buildingIndex == buildingIndex) {
      continue;
    }
    oldWallMap[index] = walls.size();
    walls.push_back(layout.walls[index]);
  }
  std::vector<std::size_t> newWallMap(positioned.walls.size(),
                                      kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.walls.size(); ++index) {
    CreativeWorldLayoutWall wall = positioned.walls[index];
    wall.buildingIndex = buildingIndex;
    applyReusedOrMintedIdentity(
        layout, nextStableOrdinal, oldWallIdentities, index, "wall",
        preserveExistingNames, wall.stableKey, wall.name);
    newWallMap[index] = walls.size();
    walls.push_back(std::move(wall));
  }

  std::vector<CreativeWorldLayoutOpening> openings;
  openings.reserve(layout.openings.size() + positioned.openings.size());
  for (CreativeWorldLayoutOpening opening : layout.openings) {
    if (openingOwnedByBuilding(layout, opening, buildingIndex)) {
      continue;
    }
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      opening.roomIndex = oldRoomMap[opening.roomIndex];
      if (opening.roomTopologyEdgeIndex !=
          kInvalidCreativeWorldLayoutIndex) {
        opening.roomTopologyEdgeIndex =
            oldTopologyEdgeMap[opening.roomTopologyEdgeIndex];
      }
    } else {
      opening.wallIndex = oldWallMap[opening.wallIndex];
    }
    openings.push_back(std::move(opening));
  }
  for (std::size_t index = 0U; index < positioned.openings.size(); ++index) {
    CreativeWorldLayoutOpening opening = positioned.openings[index];
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      opening.roomIndex = newRoomMap[opening.roomIndex];
      if (opening.roomTopologyEdgeIndex !=
          kInvalidCreativeWorldLayoutIndex) {
        opening.roomTopologyEdgeIndex =
            newTopologyEdgeMap[opening.roomTopologyEdgeIndex];
      }
    } else {
      opening.wallIndex = newWallMap[opening.wallIndex];
    }
    applyReusedOrMintedIdentity(
        layout, nextStableOrdinal, oldOpeningIdentities, index,
        opening.kind == CreativeBuildingOpeningKind::Door ? "door" : "window",
        preserveExistingNames, opening.stableKey, opening.name);
    openings.push_back(std::move(opening));
  }

  layout.levels = std::move(levels);
  layout.roofApertures = std::move(roofApertures);
  layout.rooms = std::move(rooms);
  layout.topologyVertices = std::move(topologyVertices);
  layout.topologyEdges = std::move(topologyEdges);
  layout.roomBoundaries = std::move(roomBoundaries);
  layout.verticalConnectors = std::move(verticalConnectors);
  layout.boxes = std::move(boxes);
  layout.walls = std::move(walls);
  layout.openings = std::move(openings);
  return validCreativeWorldLayoutBuildingOwnership(layout);
}

bool replaceBuildingFromTemplate(
    CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    const CreativeWorldLayoutBuildingTemplate& sourceTemplate,
    const CreativeWorldLayoutBuildingTemplateInstanceProvenance& provenance,
    std::uint64_t& nextStableOrdinal) {
  CreativeWorldLayoutBuildingTemplateResult oriented =
      orientCreativeWorldLayoutBuildingTemplate(sourceTemplate,
                                                provenance.orientation);
  if (!oriented.accepted) {
    return false;
  }
  CreativeWorldLayout positioned = oriented.value.normalizedLayout;
  positioned.buildings[0].name = sourceTemplate.label;
  if (provenance.anchor != CreativeTerrainCoord2{}) {
    CreativeWorldLayoutBuildingEditResult moved =
        moveCreativeWorldLayoutBuilding(
            positioned, {0U, provenance.anchor.x, provenance.anchor.z});
    if (!moved.accepted) {
      return false;
    }
    positioned = std::move(moved.edited);
  }
  if (!replaceBuildingContents(layout, buildingIndex, positioned,
                               nextStableOrdinal, false)) {
    return false;
  }

  CreativeWorldLayoutBuildingTemplateInstanceProvenance updated = provenance;
  updated.present = true;
  updated.valid = true;
  updated.sourceFingerprint = sourceTemplate.sourceFingerprint.value;
  const CreativeWorldLayoutBuildingTemplateFingerprint baseline =
      fingerprintCreativeWorldLayoutBuilding(layout, buildingIndex);
  if (!baseline.valid) {
    return false;
  }
  updated.instanceBaselineFingerprint = baseline.value;
  return setCreativeWorldLayoutBuildingTemplateInstanceProvenance(
      layout, buildingIndex, updated);
}

void setRefreshFailure(
    CreativeWorldLayoutBuildingTemplateRefreshResult& result,
    CreativeWorldLayoutBuildingTemplateRefreshStatus status,
    std::string reasonCode) {
  result.accepted = false;
  result.changed = false;
  result.status = status;
  result.edited = {};
  result.reasonCode = std::move(reasonCode);
}

}  // namespace

bool replaceCreativeWorldLayoutBuildingInCandidate(
    CreativeWorldLayout& candidate, std::size_t buildingIndex,
    const CreativeWorldLayout& replacement,
    std::uint64_t& nextStableOrdinal, bool preserveExistingNames) {
  return replaceBuildingContents(candidate, buildingIndex, replacement,
                                 nextStableOrdinal, preserveExistingNames);
}

CreativeWorldLayoutBuildingTemplateRefreshResult
refreshCreativeWorldLayoutBuildingTemplateInstances(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingTemplateRefreshRequest& request) {
  CreativeWorldLayoutBuildingTemplateRefreshResult result;
  result.requested = true;
  result.mode = request.mode;
  result.nextStableOrdinal = request.nextStableOrdinal;
  if (request.sourceTemplate == nullptr ||
      !validCreativeWorldLayoutBuildingTemplate(*request.sourceTemplate) ||
      request.mode >= CreativeWorldLayoutBuildingTemplateRefreshMode::Count ||
      (request.mode ==
           CreativeWorldLayoutBuildingTemplateRefreshMode::SelectedInstance &&
       request.selectedBuildingIndex >= source.buildings.size())) {
    setRefreshFailure(
        result, CreativeWorldLayoutBuildingTemplateRefreshStatus::InvalidRequest,
        "creative_world_layout_building_template_refresh_request_invalid");
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(source)) {
    setRefreshFailure(
        result,
        CreativeWorldLayoutBuildingTemplateRefreshStatus::InvalidOwnership,
        "creative_world_layout_building_template_refresh_ownership_invalid");
    return result;
  }

  std::vector<std::size_t> eligible;
  for (std::size_t index = 0U; index < source.buildings.size(); ++index) {
    const CreativeWorldLayoutBuildingTemplateInstanceProvenance provenance =
        creativeWorldLayoutBuildingTemplateInstanceProvenance(source, index);
    if (!provenance.present ||
        provenance.templateId != request.sourceTemplate->templateId) {
      continue;
    }
    ++result.matchedInstanceCount;
    const CreativeWorldLayoutBuildingTemplateSyncReceipt sync =
        inspectCreativeWorldLayoutBuildingTemplateSync(
            source, index, request.sourceTemplate);
    switch (sync.state) {
      case CreativeWorldLayoutBuildingTemplateSyncState::Current:
        ++result.currentInstanceCount;
        break;
      case CreativeWorldLayoutBuildingTemplateSyncState::SourceChanged:
        ++result.sourceChangedInstanceCount;
        break;
      case CreativeWorldLayoutBuildingTemplateSyncState::LocallyModified:
        ++result.locallyModifiedInstanceCount;
        break;
      case CreativeWorldLayoutBuildingTemplateSyncState::Conflict:
      case CreativeWorldLayoutBuildingTemplateSyncState::SourceMissing:
        ++result.conflictInstanceCount;
        break;
      case CreativeWorldLayoutBuildingTemplateSyncState::Unlinked:
        break;
    }
    const bool selected =
        request.mode !=
            CreativeWorldLayoutBuildingTemplateRefreshMode::SelectedInstance ||
        index == request.selectedBuildingIndex;
    const bool eligibleState =
        request.mode == CreativeWorldLayoutBuildingTemplateRefreshMode::ForceAll
            ? sync.state !=
                  CreativeWorldLayoutBuildingTemplateSyncState::Current
            : request.mode == CreativeWorldLayoutBuildingTemplateRefreshMode::
                                   SelectedInstance
                  ? sync.state !=
                        CreativeWorldLayoutBuildingTemplateSyncState::Current
                  : sync.state == CreativeWorldLayoutBuildingTemplateSyncState::
                                      SourceChanged;
    if (selected && eligibleState && provenance.valid) {
      eligible.push_back(index);
    }
  }

  if (result.matchedInstanceCount == 0U) {
    setRefreshFailure(
        result,
        CreativeWorldLayoutBuildingTemplateRefreshStatus::NoMatchingInstances,
        "creative_world_layout_building_template_refresh_instances_missing");
    return result;
  }
  if (eligible.empty()) {
    setRefreshFailure(
        result,
        CreativeWorldLayoutBuildingTemplateRefreshStatus::NoEligibleInstances,
        "creative_world_layout_building_template_refresh_no_eligible_"
        "instances");
    return result;
  }

  CreativeWorldLayout edited = source;
  std::uint64_t nextStableOrdinal = request.nextStableOrdinal;
  for (std::size_t buildingIndex : eligible) {
    const CreativeWorldLayoutBuildingTemplateInstanceProvenance provenance =
        creativeWorldLayoutBuildingTemplateInstanceProvenance(edited,
                                                              buildingIndex);
    if (!provenance.valid ||
        !replaceBuildingFromTemplate(edited, buildingIndex,
                                     *request.sourceTemplate, provenance,
                                     nextStableOrdinal)) {
      setRefreshFailure(
          result,
          CreativeWorldLayoutBuildingTemplateRefreshStatus::CoordinateOverflow,
          "creative_world_layout_building_template_refresh_replacement_failed");
      return result;
    }
    ++result.refreshedInstanceCount;
  }
  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutBuildingTemplateRefreshStatus::Ready;
  result.nextStableOrdinal = nextStableOrdinal;
  result.edited = std::move(edited);
  result.reasonCode =
      "creative_world_layout_building_template_refresh_ready";
  return result;
}

}  // namespace iggy3d::creative
