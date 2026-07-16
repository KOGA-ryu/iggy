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

std::vector<std::string> ownedRoomKeys(const CreativeWorldLayout& layout,
                                       std::size_t buildingIndex) {
  std::vector<std::string> result;
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex == buildingIndex) {
      result.push_back(room.stableKey);
    }
  }
  return result;
}

std::vector<std::string> ownedBoxKeys(const CreativeWorldLayout& layout,
                                      std::size_t buildingIndex) {
  std::vector<std::string> result;
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex == buildingIndex) {
      result.push_back(box.stableKey);
    }
  }
  return result;
}

std::vector<std::string> ownedWallKeys(const CreativeWorldLayout& layout,
                                       std::size_t buildingIndex) {
  std::vector<std::string> result;
  for (const CreativeWorldLayoutWall& wall : layout.walls) {
    if (wall.buildingIndex == buildingIndex) {
      result.push_back(wall.stableKey);
    }
  }
  return result;
}

std::vector<std::string> ownedOpeningKeys(const CreativeWorldLayout& layout,
                                          std::size_t buildingIndex) {
  std::vector<std::string> result;
  for (const CreativeWorldLayoutOpening& opening : layout.openings) {
    if (openingOwnedByBuilding(layout, opening, buildingIndex)) {
      result.push_back(opening.stableKey);
    }
  }
  return result;
}

std::string reusedOrMintedKey(CreativeWorldLayout& layout,
                              std::uint64_t& nextStableOrdinal,
                              const std::vector<std::string>& oldKeys,
                              std::size_t ordinal,
                              std::string_view prefix) {
  return ordinal < oldKeys.size()
             ? oldKeys[ordinal]
             : mintCreativeWorldLayoutStableKey(layout, nextStableOrdinal,
                                                prefix);
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
  if (provenance.anchor != CreativeTerrainCoord2{}) {
    CreativeWorldLayoutBuildingEditResult moved =
        moveCreativeWorldLayoutBuilding(
            positioned, {0U, provenance.anchor.x, provenance.anchor.z});
    if (!moved.accepted) {
      return false;
    }
    positioned = std::move(moved.edited);
  }

  const std::vector<std::string> oldRoomKeys =
      ownedRoomKeys(layout, buildingIndex);
  const std::vector<std::string> oldBoxKeys =
      ownedBoxKeys(layout, buildingIndex);
  const std::vector<std::string> oldWallKeys =
      ownedWallKeys(layout, buildingIndex);
  const std::vector<std::string> oldOpeningKeys =
      ownedOpeningKeys(layout, buildingIndex);
  const std::string buildingStableKey =
      layout.buildings[buildingIndex].stableKey;

  CreativeWorldLayoutBuilding replacement = positioned.buildings[0];
  replacement.stableKey = buildingStableKey;
  replacement.name = sourceTemplate.label;
  layout.buildings[buildingIndex] = std::move(replacement);

  std::vector<std::size_t> oldRoomMap(layout.rooms.size(),
                                      kInvalidCreativeWorldLayoutIndex);
  std::vector<CreativeWorldLayoutRoom> rooms;
  rooms.reserve(layout.rooms.size() + positioned.rooms.size());
  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    if (layout.rooms[index].buildingIndex == buildingIndex) {
      continue;
    }
    oldRoomMap[index] = rooms.size();
    rooms.push_back(layout.rooms[index]);
  }
  std::vector<std::size_t> newRoomMap(positioned.rooms.size(),
                                      kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.rooms.size(); ++index) {
    CreativeWorldLayoutRoom room = positioned.rooms[index];
    room.buildingIndex = buildingIndex;
    room.stableKey = reusedOrMintedKey(layout, nextStableOrdinal, oldRoomKeys,
                                       index, "room");
    newRoomMap[index] = rooms.size();
    rooms.push_back(std::move(room));
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
    box.stableKey = reusedOrMintedKey(layout, nextStableOrdinal, oldBoxKeys,
                                      index, "floor");
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
    wall.stableKey = reusedOrMintedKey(layout, nextStableOrdinal, oldWallKeys,
                                       index, "wall");
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
    } else {
      opening.wallIndex = oldWallMap[opening.wallIndex];
    }
    openings.push_back(std::move(opening));
  }
  for (std::size_t index = 0U; index < positioned.openings.size(); ++index) {
    CreativeWorldLayoutOpening opening = positioned.openings[index];
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      opening.roomIndex = newRoomMap[opening.roomIndex];
    } else {
      opening.wallIndex = newWallMap[opening.wallIndex];
    }
    opening.stableKey = reusedOrMintedKey(
        layout, nextStableOrdinal, oldOpeningKeys, index,
        opening.kind == CreativeBuildingOpeningKind::Door ? "door" :
                                                            "window");
    openings.push_back(std::move(opening));
  }

  layout.rooms = std::move(rooms);
  layout.boxes = std::move(boxes);
  layout.walls = std::move(walls);
  layout.openings = std::move(openings);

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
             layout, buildingIndex, updated) &&
         validCreativeWorldLayoutBuildingOwnership(layout);
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
        "creative_world_layout_building_template_refresh_no_eligible_instances");
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
