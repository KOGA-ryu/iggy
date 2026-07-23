#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

void setTemplateFailure(CreativeWorldLayoutBuildingTemplateResult& result,
                        CreativeWorldLayoutBuildingTemplateStatus status,
                        std::string reasonCode) {
  result.accepted = false;
  result.status = status;
  result.value = {};
  result.reasonCode = std::move(reasonCode);
}

void setEditFailure(CreativeWorldLayoutBuildingEditResult& result,
                    CreativeWorldLayoutBuildingEditStatus status,
                    std::string reasonCode) {
  result.accepted = false;
  result.changed = false;
  result.status = status;
  result.edited = {};
  result.reasonCode = std::move(reasonCode);
}

std::string copiedName(std::string_view source, bool appendCopySuffix) {
  return appendCopySuffix ? std::string(source) + " Copy" : std::string(source);
}

bool sameBounds(CreativeWorldLayoutBuildingBounds lhs,
                CreativeWorldLayoutBuildingBounds rhs) noexcept {
  return lhs.valid == rhs.valid && lhs.minimum == rhs.minimum &&
         lhs.maximum == rhs.maximum;
}

bool templateKeysValid(const CreativeWorldLayout& layout) {
  std::unordered_set<std::string> keys;
  const auto registerKey = [&](const auto& symbol) {
    return validCreativeWorldLayoutStableKey(symbol.stableKey) &&
           keys.insert(symbol.stableKey).second;
  };
  if (!registerKey(layout.buildings.front())) {
    return false;
  }
  for (const CreativeWorldLayoutLevel& value : layout.levels) {
    if (!registerKey(value)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutRoom& value : layout.rooms) {
    if (!registerKey(value)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutTopologyVertex& value :
       layout.topologyVertices) {
    if (!registerKey(value)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutTopologyEdge& value : layout.topologyEdges) {
    if (!registerKey(value)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutVerticalConnector& value :
       layout.verticalConnectors) {
    if (!registerKey(value)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutBox& value : layout.boxes) {
    if (!registerKey(value)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutWall& value : layout.walls) {
    if (!registerKey(value)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutOpening& value : layout.openings) {
    if (!registerKey(value)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutRoofAperture& value :
       layout.roofApertures) {
    if (!registerKey(value)) {
      return false;
    }
  }
  return true;
}

bool openingOwnedByBuilding(const CreativeWorldLayout& layout,
                            const CreativeWorldLayoutOpening& opening,
                            std::size_t buildingIndex) noexcept {
  return opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge
             ? layout.rooms[opening.roomIndex].buildingIndex == buildingIndex
             : layout.walls[opening.wallIndex].buildingIndex == buildingIndex;
}

CreativeWorldLayout isolateBuilding(const CreativeWorldLayout& source,
                                    std::size_t buildingIndex,
                                    std::string templateId,
                                    std::string label) {
  CreativeWorldLayout isolated;
  isolated.schemaVersion = source.schemaVersion;
  isolated.stableKey = std::move(templateId);
  isolated.terrainOwnership =
      CreativeWorldLayoutTerrainOwnership::PreserveExisting;

  CreativeWorldLayoutBuilding building = source.buildings[buildingIndex];
  building.name = std::move(label);
  std::erase_if(building.tags, [](const std::string& tag) {
    return isCreativeWorldLayoutBuildingTemplateProvenanceTag(tag) ||
           isCreativeWorldLayoutBuildingBlockoutProvenanceTag(tag);
  });
  isolated.buildings.push_back(std::move(building));

  std::vector<std::size_t> levelMap(source.levels.size(),
                                    kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < source.levels.size(); ++index) {
    if (source.levels[index].buildingIndex != buildingIndex) {
      continue;
    }
    CreativeWorldLayoutLevel level = source.levels[index];
    level.buildingIndex = 0U;
    levelMap[index] = isolated.levels.size();
    isolated.levels.push_back(std::move(level));
  }

  for (const CreativeWorldLayoutRoofAperture& sourceAperture :
       source.roofApertures) {
    if (sourceAperture.levelIndex >= levelMap.size() ||
        levelMap[sourceAperture.levelIndex] ==
            kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    CreativeWorldLayoutRoofAperture aperture = sourceAperture;
    aperture.levelIndex = levelMap[sourceAperture.levelIndex];
    isolated.roofApertures.push_back(std::move(aperture));
  }

  std::vector<std::size_t> roomMap(source.rooms.size(),
                                   kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < source.rooms.size(); ++index) {
    if (source.rooms[index].buildingIndex != buildingIndex) {
      continue;
    }
    CreativeWorldLayoutRoom room = source.rooms[index];
    room.buildingIndex = 0U;
    room.levelIndex = levelMap[room.levelIndex];
    roomMap[index] = isolated.rooms.size();
    isolated.rooms.push_back(std::move(room));
  }

  std::vector<std::size_t> topologyVertexMap(
      source.topologyVertices.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < source.topologyVertices.size();
       ++index) {
    const CreativeWorldLayoutTopologyVertex& sourceVertex =
        source.topologyVertices[index];
    if (levelMap[sourceVertex.levelIndex] ==
        kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    CreativeWorldLayoutTopologyVertex vertex = sourceVertex;
    vertex.levelIndex = levelMap[sourceVertex.levelIndex];
    topologyVertexMap[index] = isolated.topologyVertices.size();
    isolated.topologyVertices.push_back(std::move(vertex));
  }

  std::vector<std::size_t> topologyEdgeMap(
      source.topologyEdges.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < source.topologyEdges.size(); ++index) {
    const CreativeWorldLayoutTopologyEdge& sourceEdge =
        source.topologyEdges[index];
    if (levelMap[sourceEdge.levelIndex] == kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    CreativeWorldLayoutTopologyEdge edge = sourceEdge;
    edge.levelIndex = levelMap[sourceEdge.levelIndex];
    edge.startVertexIndex = topologyVertexMap[sourceEdge.startVertexIndex];
    edge.endVertexIndex = topologyVertexMap[sourceEdge.endVertexIndex];
    topologyEdgeMap[index] = isolated.topologyEdges.size();
    isolated.topologyEdges.push_back(std::move(edge));
  }

  for (const CreativeWorldLayoutRoomBoundary& sourceBoundary :
       source.roomBoundaries) {
    if (roomMap[sourceBoundary.roomIndex] ==
            kInvalidCreativeWorldLayoutIndex ||
        topologyEdgeMap[sourceBoundary.topologyEdgeIndex] ==
            kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    CreativeWorldLayoutRoomBoundary boundary = sourceBoundary;
    boundary.roomIndex = roomMap[sourceBoundary.roomIndex];
    boundary.topologyEdgeIndex =
        topologyEdgeMap[sourceBoundary.topologyEdgeIndex];
    isolated.roomBoundaries.push_back(std::move(boundary));
  }

  for (const CreativeWorldLayoutVerticalConnector& sourceConnector :
       source.verticalConnectors) {
    if (sourceConnector.buildingIndex != buildingIndex) {
      continue;
    }
    CreativeWorldLayoutVerticalConnector connector = sourceConnector;
    connector.buildingIndex = 0U;
    connector.lowerRoomIndex = roomMap[connector.lowerRoomIndex];
    connector.upperRoomIndex = roomMap[connector.upperRoomIndex];
    isolated.verticalConnectors.push_back(std::move(connector));
  }

  for (const CreativeWorldLayoutBox& sourceBox : source.boxes) {
    if (sourceBox.buildingIndex != buildingIndex) {
      continue;
    }
    CreativeWorldLayoutBox box = sourceBox;
    box.buildingIndex = 0U;
    isolated.boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> wallMap(source.walls.size(),
                                   kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < source.walls.size(); ++index) {
    if (source.walls[index].buildingIndex != buildingIndex) {
      continue;
    }
    CreativeWorldLayoutWall wall = source.walls[index];
    wall.buildingIndex = 0U;
    wallMap[index] = isolated.walls.size();
    isolated.walls.push_back(std::move(wall));
  }

  for (const CreativeWorldLayoutOpening& sourceOpening : source.openings) {
    if (!openingOwnedByBuilding(source, sourceOpening, buildingIndex)) {
      continue;
    }
    CreativeWorldLayoutOpening opening = sourceOpening;
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      opening.roomIndex = roomMap[opening.roomIndex];
      if (opening.roomTopologyEdgeIndex !=
          kInvalidCreativeWorldLayoutIndex) {
        opening.roomTopologyEdgeIndex =
            topologyEdgeMap[opening.roomTopologyEdgeIndex];
      }
    } else {
      opening.wallIndex = wallMap[opening.wallIndex];
    }
    isolated.openings.push_back(std::move(opening));
  }
  return isolated;
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutBuildingTemplateStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutBuildingTemplateStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutBuildingTemplateStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutBuildingTemplateStatus::InvalidOwnership:
      return "InvalidOwnership";
    case CreativeWorldLayoutBuildingTemplateStatus::EmptyBuilding:
      return "EmptyBuilding";
    case CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate:
      return "InvalidTemplate";
    case CreativeWorldLayoutBuildingTemplateStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeWorldLayoutBuildingTemplateStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

bool validCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayoutBuildingTemplate& value) noexcept {
  if (!validCreativeWorldLayoutStableKey(value.templateId) ||
      value.label.empty() ||
      value.normalizedLayout.schemaVersion !=
          kCreativeWorldLayoutSchemaVersion ||
      value.normalizedLayout.stableKey != value.templateId ||
      value.normalizedLayout.buildings.size() != 1U ||
      !value.normalizedLayout.objects.empty() ||
      !value.normalizedLayout.terrainProfiles.empty() ||
      !value.normalizedLayout.terrainPaths.empty() ||
      !value.sourceFingerprint.valid ||
      value.orientation >=
          CreativeWorldLayoutBuildingTemplateOrientation::Count ||
      !validCreativeWorldLayoutBuildingOwnership(value.normalizedLayout)) {
    return false;
  }
  CreativeWorldLayoutBuildingBounds measured;
  const CreativeWorldLayoutBuildingTemplateFingerprint currentFingerprint =
      fingerprintCreativeWorldLayoutBuilding(value.normalizedLayout, 0U);
  return currentFingerprint.valid &&
         (value.orientation !=
              CreativeWorldLayoutBuildingTemplateOrientation::Identity ||
          currentFingerprint == value.sourceFingerprint) &&
         measureCreativeWorldLayoutBuildingBounds(value.normalizedLayout, 0U,
                                                  measured) &&
         measured.minimum == CreativeTerrainCoord2{} &&
         sameBounds(value.bounds, measured) &&
         templateKeysValid(value.normalizedLayout);
}

CreativeWorldLayoutBuildingTemplateResult
captureCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingTemplateCaptureRequest& request) {
  CreativeWorldLayoutBuildingTemplateResult result;
  result.requested = true;
  if (request.buildingIndex >= source.buildings.size() ||
      !validCreativeWorldLayoutStableKey(request.templateId)) {
    setTemplateFailure(
        result, CreativeWorldLayoutBuildingTemplateStatus::InvalidRequest,
        "creative_world_layout_building_template_capture_request_invalid");
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(source)) {
    setTemplateFailure(
        result, CreativeWorldLayoutBuildingTemplateStatus::InvalidOwnership,
        "creative_world_layout_building_template_capture_ownership_invalid");
    return result;
  }
  CreativeWorldLayoutBuildingBounds sourceBounds;
  if (!measureCreativeWorldLayoutBuildingBounds(source, request.buildingIndex,
                                                sourceBounds)) {
    setTemplateFailure(result,
                       CreativeWorldLayoutBuildingTemplateStatus::EmptyBuilding,
                       "creative_world_layout_building_template_capture_empty");
    return result;
  }

  const std::string label = request.label.empty()
                                ? source.buildings[request.buildingIndex].name
                                : request.label;
  if (label.empty()) {
    setTemplateFailure(
        result, CreativeWorldLayoutBuildingTemplateStatus::InvalidRequest,
        "creative_world_layout_building_template_capture_label_invalid");
    return result;
  }
  CreativeWorldLayout isolated =
      isolateBuilding(source, request.buildingIndex, request.templateId, label);
  CreativeWorldLayoutBuildingEditResult normalized =
      moveCreativeWorldLayoutBuilding(
          isolated, {0U, -static_cast<std::int64_t>(sourceBounds.minimum.x),
                     -static_cast<std::int64_t>(sourceBounds.minimum.z)});
  if (!normalized.accepted) {
    setTemplateFailure(
        result,
        normalized.status ==
                CreativeWorldLayoutBuildingEditStatus::CoordinateOverflow
            ? CreativeWorldLayoutBuildingTemplateStatus::CoordinateOverflow
            : CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate,
        normalized.reasonCode);
    return result;
  }

  result.value.templateId = request.templateId;
  result.value.label = label;
  result.value.normalizedLayout = std::move(normalized.edited);
  result.value.orientation =
      CreativeWorldLayoutBuildingTemplateOrientation::Identity;
  result.value.sourceFingerprint = fingerprintCreativeWorldLayoutBuilding(
      result.value.normalizedLayout, 0U);
  if (!measureCreativeWorldLayoutBuildingBounds(result.value.normalizedLayout,
                                                0U, result.value.bounds) ||
      !validCreativeWorldLayoutBuildingTemplate(result.value)) {
    setTemplateFailure(
        result, CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate,
        "creative_world_layout_building_template_capture_result_invalid");
    return result;
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutBuildingTemplateStatus::Ready;
  result.reasonCode = "creative_world_layout_building_template_capture_ready";
  return result;
}

CreativeWorldLayoutBuildingTemplateResult
loadCreativeWorldLayoutBuildingTemplate(CreativeWorldLayout normalizedLayout) {
  CreativeWorldLayoutBuildingTemplateResult result;
  result.requested = true;
  if (normalizedLayout.buildings.size() != 1U) {
    setTemplateFailure(
        result, CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate,
        "creative_world_layout_building_template_load_invalid");
    return result;
  }
  result.value.templateId = normalizedLayout.stableKey;
  result.value.label = normalizedLayout.buildings[0].name;
  result.value.normalizedLayout = std::move(normalizedLayout);
  result.value.orientation =
      CreativeWorldLayoutBuildingTemplateOrientation::Identity;
  result.value.sourceFingerprint = fingerprintCreativeWorldLayoutBuilding(
      result.value.normalizedLayout, 0U);
  if (!measureCreativeWorldLayoutBuildingBounds(result.value.normalizedLayout,
                                                0U, result.value.bounds) ||
      !validCreativeWorldLayoutBuildingTemplate(result.value)) {
    setTemplateFailure(
        result, CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate,
        "creative_world_layout_building_template_load_invalid");
    return result;
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutBuildingTemplateStatus::Ready;
  result.reasonCode = "creative_world_layout_building_template_load_ready";
  return result;
}

CreativeWorldLayoutBuildingTemplateResult
transformCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayoutBuildingTemplate& source,
    CreativeWorldLayoutBuildingTransformOperation operation) {
  CreativeWorldLayoutBuildingTemplateResult result;
  result.requested = true;
  if (!validCreativeWorldLayoutBuildingTemplate(source) ||
      operation >= CreativeWorldLayoutBuildingTransformOperation::Count) {
    setTemplateFailure(
        result, CreativeWorldLayoutBuildingTemplateStatus::InvalidRequest,
        "creative_world_layout_building_template_transform_request_invalid");
    return result;
  }
  CreativeWorldLayoutBuildingTransformResult transformed =
      transformCreativeWorldLayoutBuilding(source.normalizedLayout,
                                           {0U, operation});
  if (!transformed.accepted) {
    setTemplateFailure(
        result,
        transformed.status ==
                CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow
            ? CreativeWorldLayoutBuildingTemplateStatus::CoordinateOverflow
            : CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate,
        transformed.reasonCode);
    return result;
  }
  result.value = source;
  result.value.normalizedLayout = std::move(transformed.transformed);
  result.value.bounds = transformed.transformedBounds;
  result.value.orientation =
      composeCreativeWorldLayoutBuildingTemplateOrientation(source.orientation,
                                                            operation);
  if (!validCreativeWorldLayoutBuildingTemplate(result.value)) {
    setTemplateFailure(
        result, CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate,
        "creative_world_layout_building_template_transform_result_invalid");
    return result;
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutBuildingTemplateStatus::Ready;
  result.reasonCode = "creative_world_layout_building_template_transform_ready";
  return result;
}

CreativeWorldLayoutBuildingEditResult stampCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayout& destination,
    const CreativeWorldLayoutBuildingTemplate& source,
    const CreativeWorldLayoutBuildingTemplateStampRequest& request) {
  CreativeWorldLayoutBuildingEditResult result;
  result.requested = true;
  result.nextStableOrdinal = request.nextStableOrdinal;
  if (!validCreativeWorldLayoutBuildingTemplate(source)) {
    setEditFailure(
        result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
        "creative_world_layout_building_template_stamp_template_invalid");
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(destination)) {
    setEditFailure(
        result, CreativeWorldLayoutBuildingEditStatus::InvalidOwnership,
        "creative_world_layout_building_template_stamp_ownership_invalid");
    return result;
  }
  if (!canMoveCreativeWorldLayoutBuilding(
          source.normalizedLayout, {0U, request.anchor.x, request.anchor.z})) {
    setEditFailure(
        result, CreativeWorldLayoutBuildingEditStatus::CoordinateOverflow,
        "creative_world_layout_building_template_stamp_coordinate_overflow");
    return result;
  }

  CreativeWorldLayout positioned = source.normalizedLayout;
  if (request.anchor != CreativeTerrainCoord2{}) {
    CreativeWorldLayoutBuildingEditResult moved =
        moveCreativeWorldLayoutBuilding(
            positioned, {0U, request.anchor.x, request.anchor.z});
    if (!moved.accepted) {
      setEditFailure(
          result, CreativeWorldLayoutBuildingEditStatus::CoordinateOverflow,
          "creative_world_layout_building_template_stamp_coordinate_overflow");
      return result;
    }
    positioned = std::move(moved.edited);
  }

  CreativeWorldLayout edited = destination;
  std::uint64_t nextOrdinal = request.nextStableOrdinal;
  const std::size_t newBuildingIndex = edited.buildings.size();
  CreativeWorldLayoutBuilding building = positioned.buildings[0];
  if (building.rootMode == CreativeBuildingRootMode::None) {
    CreativeWorldLayoutBuildingBounds positionedBounds;
    if (!measureCreativeWorldLayoutBuildingBounds(positioned, 0U,
                                                  positionedBounds)) {
      setEditFailure(
          result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
          "creative_world_layout_building_template_stamp_bounds_invalid");
      return result;
    }
    building.rootFootprint =
        {positionedBounds.minimum, positionedBounds.maximum};
  }
  building.stableKey =
      mintCreativeWorldLayoutStableKey(edited, nextOrdinal, "building");
  building.name = copiedName(source.label, request.appendCopySuffix);
  edited.buildings.push_back(std::move(building));

  std::vector<std::size_t> levelMap(positioned.levels.size(),
                                    kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.levels.size(); ++index) {
    CreativeWorldLayoutLevel level = positioned.levels[index];
    level.buildingIndex = newBuildingIndex;
    level.stableKey =
        mintCreativeWorldLayoutStableKey(edited, nextOrdinal, "level");
    level.name = copiedName(level.name, request.appendCopySuffix);
    levelMap[index] = edited.levels.size();
    edited.levels.push_back(std::move(level));
  }

  for (CreativeWorldLayoutRoofAperture aperture :
       positioned.roofApertures) {
    aperture.levelIndex = levelMap[aperture.levelIndex];
    aperture.stableKey = mintCreativeWorldLayoutStableKey(
        edited, nextOrdinal,
        aperture.kind == CreativeStructuralRoofApertureKind::Skylight
            ? "skylight"
            : "chimney_clearance");
    aperture.name = copiedName(aperture.name, request.appendCopySuffix);
    edited.roofApertures.push_back(std::move(aperture));
  }

  std::vector<std::size_t> roomMap(positioned.rooms.size(),
                                   kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.rooms.size(); ++index) {
    CreativeWorldLayoutRoom room = positioned.rooms[index];
    room.buildingIndex = newBuildingIndex;
    room.levelIndex = levelMap[room.levelIndex];
    room.stableKey =
        mintCreativeWorldLayoutStableKey(edited, nextOrdinal, "room");
    room.name = copiedName(room.name, request.appendCopySuffix);
    roomMap[index] = edited.rooms.size();
    edited.rooms.push_back(std::move(room));
  }

  std::vector<std::size_t> topologyVertexMap(
      positioned.topologyVertices.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U;
       index < positioned.topologyVertices.size(); ++index) {
    CreativeWorldLayoutTopologyVertex vertex =
        positioned.topologyVertices[index];
    vertex.levelIndex = levelMap[vertex.levelIndex];
    vertex.stableKey = mintCreativeWorldLayoutStableKey(
        edited, nextOrdinal, "wall_vertex");
    topologyVertexMap[index] = edited.topologyVertices.size();
    edited.topologyVertices.push_back(std::move(vertex));
  }

  std::vector<std::size_t> topologyEdgeMap(
      positioned.topologyEdges.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.topologyEdges.size();
       ++index) {
    CreativeWorldLayoutTopologyEdge edge = positioned.topologyEdges[index];
    edge.levelIndex = levelMap[edge.levelIndex];
    edge.startVertexIndex = topologyVertexMap[edge.startVertexIndex];
    edge.endVertexIndex = topologyVertexMap[edge.endVertexIndex];
    edge.stableKey =
        mintCreativeWorldLayoutStableKey(edited, nextOrdinal, "wall");
    topologyEdgeMap[index] = edited.topologyEdges.size();
    edited.topologyEdges.push_back(std::move(edge));
  }

  for (CreativeWorldLayoutRoomBoundary boundary :
       positioned.roomBoundaries) {
    boundary.roomIndex = roomMap[boundary.roomIndex];
    boundary.topologyEdgeIndex =
        topologyEdgeMap[boundary.topologyEdgeIndex];
    edited.roomBoundaries.push_back(std::move(boundary));
  }

  for (CreativeWorldLayoutVerticalConnector connector :
       positioned.verticalConnectors) {
    connector.buildingIndex = newBuildingIndex;
    connector.lowerRoomIndex = roomMap[connector.lowerRoomIndex];
    connector.upperRoomIndex = roomMap[connector.upperRoomIndex];
    connector.stableKey = mintCreativeWorldLayoutStableKey(
        edited, nextOrdinal, "vertical_connector");
    connector.name = copiedName(connector.name, request.appendCopySuffix);
    edited.verticalConnectors.push_back(std::move(connector));
  }

  for (CreativeWorldLayoutBox box : positioned.boxes) {
    box.buildingIndex = newBuildingIndex;
    box.stableKey =
        mintCreativeWorldLayoutStableKey(edited, nextOrdinal, "floor");
    box.name = copiedName(box.name, request.appendCopySuffix);
    edited.boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> wallMap(positioned.walls.size(),
                                   kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < positioned.walls.size(); ++index) {
    CreativeWorldLayoutWall wall = positioned.walls[index];
    wall.buildingIndex = newBuildingIndex;
    wall.stableKey =
        mintCreativeWorldLayoutStableKey(edited, nextOrdinal, "wall");
    wall.name = copiedName(wall.name, request.appendCopySuffix);
    wallMap[index] = edited.walls.size();
    edited.walls.push_back(std::move(wall));
  }

  for (CreativeWorldLayoutOpening opening : positioned.openings) {
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      opening.roomIndex = roomMap[opening.roomIndex];
      if (opening.roomTopologyEdgeIndex !=
          kInvalidCreativeWorldLayoutIndex) {
        opening.roomTopologyEdgeIndex =
            topologyEdgeMap[opening.roomTopologyEdgeIndex];
      }
    } else {
      opening.wallIndex = wallMap[opening.wallIndex];
    }
    opening.stableKey = mintCreativeWorldLayoutStableKey(
        edited, nextOrdinal,
        opening.kind == CreativeBuildingOpeningKind::Door ? "door" : "window");
    opening.name = copiedName(opening.name, request.appendCopySuffix);
    edited.openings.push_back(std::move(opening));
  }

  if (request.linkTemplateInstance) {
    const CreativeWorldLayoutBuildingTemplateFingerprint baseline =
        fingerprintCreativeWorldLayoutBuilding(source.normalizedLayout, 0U);
    CreativeWorldLayoutBuildingTemplateInstanceProvenance provenance;
    provenance.present = true;
    provenance.valid = baseline.valid && source.sourceFingerprint.valid;
    provenance.templateId = source.templateId;
    provenance.sourceFingerprint = source.sourceFingerprint.value;
    provenance.instanceBaselineFingerprint = baseline.value;
    provenance.orientation = source.orientation;
    provenance.anchor = request.anchor;
    if (!provenance.valid ||
        !setCreativeWorldLayoutBuildingTemplateInstanceProvenance(
            edited, newBuildingIndex, provenance)) {
      setEditFailure(
          result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
          "creative_world_layout_building_template_stamp_provenance_invalid");
      return result;
    }
  }

  if (!validCreativeWorldLayoutBuildingOwnership(edited)) {
    setEditFailure(
        result, CreativeWorldLayoutBuildingEditStatus::InvalidOwnership,
        "creative_world_layout_building_template_stamp_result_invalid");
    return result;
  }

  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutBuildingEditStatus::Ready;
  result.sourceBuildingIndex = 0U;
  result.resultBuildingIndex = newBuildingIndex;
  result.nextStableOrdinal = nextOrdinal;
  result.edited = std::move(edited);
  result.reasonCode = "creative_world_layout_building_template_stamp_ready";
  return result;
}

}  // namespace iggy3d::creative
