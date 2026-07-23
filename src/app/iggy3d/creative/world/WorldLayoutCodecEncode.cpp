#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kHeader = "IGGY3D_WORLD_LAYOUT";

struct DecodeFailure {
  CreativeWorldLayoutCodecStatus status =
      CreativeWorldLayoutCodecStatus::InvalidRecord;
  std::string_view reason = "creative_world_layout_decode_invalid_record";
};

template <typename Enum>
constexpr auto enumValue(Enum value) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(value);
}

std::string hexString(std::string_view input) {
  if (input.empty()) {
    return "-";
  }
  constexpr char kDigits[] = "0123456789abcdef";
  std::string output;
  output.reserve(input.size() * 2U);
  for (const char character : input) {
    const auto value = static_cast<unsigned char>(character);
    output.push_back(kDigits[value >> 4U]);
    output.push_back(kDigits[value & 0x0FU]);
  }
  return output;
}

bool validString(std::string_view value) noexcept {
  return value.size() <= kCreativeWorldLayoutCodecMaxStringBytes;
}

bool validRect(const CreativeWorldLayoutRect& rect) noexcept {
  (void)rect;
  return true;
}

bool boundedRecordCount(const CreativeWorldLayout& layout,
                        std::size_t& output) noexcept {
  output = 3U;  // Header, layout record, and END.
  std::size_t terrainPathPointCount = 0U;
  std::size_t terrainPathCrossingCount = 0U;
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    if (path.recipe.points.size() >
        kCreativeWorldLayoutCodecMaxRecords - terrainPathPointCount) {
      return false;
    }
    terrainPathPointCount += path.recipe.points.size();
    if (path.recipe.watercourse.crossings.size() >
        kCreativeWorldLayoutCodecMaxRecords - terrainPathCrossingCount) {
      return false;
    }
    terrainPathCrossingCount += path.recipe.watercourse.crossings.size();
  }
  const std::size_t counts[] = {
      layout.buildings.size(),
      layout.levels.size(),
      layout.rooms.size(),
      layout.topologyVertices.size(),
      layout.topologyEdges.size(),
      layout.roomBoundaries.size(),
      layout.verticalConnectors.size(),
      layout.boxes.size(),
      layout.walls.size(),
      layout.openings.size(),
      layout.roofApertures.size(),
      layout.objects.size(),
      layout.terrainProfiles.size(),
      layout.terrainPaths.size(),
      terrainPathPointCount,
      terrainPathCrossingCount,
  };
  for (const std::size_t count : counts) {
    if (count > kCreativeWorldLayoutCodecMaxRecords - output) {
      return false;
    }
    output += count;
  }
  return true;
}

bool validateForEncoding(const CreativeWorldLayout& layout,
                         DecodeFailure& failure) {
  std::size_t records = 0U;
  if (!boundedRecordCount(layout, records)) {
    failure = {CreativeWorldLayoutCodecStatus::CapacityExceeded,
               "creative_world_layout_encode_capacity_exceeded"};
    return false;
  }
  if (layout.schemaVersion != kCreativeWorldLayoutSchemaVersion) {
    failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
               "creative_world_layout_encode_schema_mismatch"};
    return false;
  }
  if (!validString(layout.stableKey)) {
    failure = {CreativeWorldLayoutCodecStatus::InvalidString,
               "creative_world_layout_encode_string_too_long"};
    return false;
  }
  if (enumValue(layout.terrainOwnership) >
      enumValue(CreativeWorldLayoutTerrainOwnership::ReplaceAll)) {
    failure = {CreativeWorldLayoutCodecStatus::InvalidEnum,
               "creative_world_layout_encode_invalid_terrain_ownership"};
    return false;
  }
  const auto validKeyName = [&](std::string_view key, std::string_view name) {
    return validString(key) && validString(name);
  };
  std::size_t totalTagCount = 0U;
  for (const CreativeWorldLayoutBuilding& building : layout.buildings) {
    if (building.tags.size() >
        kCreativeWorldLayoutCodecMaxRecords - totalTagCount) {
      failure = {CreativeWorldLayoutCodecStatus::CapacityExceeded,
                 "creative_world_layout_encode_tag_capacity_exceeded"};
      return false;
    }
    totalTagCount += building.tags.size();
    if (!validKeyName(building.stableKey, building.name) ||
        !validRect(building.rootFootprint) ||
        enumValue(building.rootMode) >
            enumValue(CreativeBuildingRootMode::ExistingRoom) ||
        enumValue(building.groundingMode) >=
            enumValue(CreativeWorldLayoutGroundingMode::Count) ||
        building.tags.size() > kCreativeWorldLayoutCodecMaxRecords ||
        !std::all_of(building.tags.begin(), building.tags.end(), validString)) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
                 "creative_world_layout_encode_invalid_building"};
      return false;
    }
  }
  for (const CreativeWorldLayoutLevel& level : layout.levels) {
    const bool finite = std::isfinite(level.floorTopLayer) &&
                        std::isfinite(level.roofPitchDegrees) &&
                        std::isfinite(level.roofOverhangCells);
    if (!validKeyName(level.stableKey, level.name) || !finite ||
        level.buildingIndex >= layout.buildings.size() ||
        level.wallHeightCells == 0U || level.floorThicknessLayers == 0U ||
        level.ceilingThicknessLayers == 0U ||
        level.roofThicknessLayers == 0U ||
        !validCreativeStructuralRoofSettings(
            level.roofStyle, level.roofRidgeAxis, level.roofSlopeDirection,
            level.roofPitchDegrees, level.roofOverhangCells,
            level.roofMaterial) ||
        level.roofOverhangCells >
            kMaximumCreativeWorldLayoutRoofOverhangCells) {
      failure = {finite ? CreativeWorldLayoutCodecStatus::InvalidRecord
                        : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_level"};
      return false;
    }
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (!validKeyName(room.stableKey, room.name) ||
        !validRect(room.footprint) ||
        room.buildingIndex >= layout.buildings.size() ||
        room.levelIndex >= layout.levels.size() ||
        layout.levels[room.levelIndex].buildingIndex != room.buildingIndex ||
        !std::isfinite(room.wallThicknessCells) ||
        enumValue(room.type) >= enumValue(CreativeWorldLayoutRoomType::Count)) {
      failure = {std::isfinite(room.wallThicknessCells)
                     ? CreativeWorldLayoutCodecStatus::InvalidRecord
                     : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_room"};
      return false;
    }
  }
  const bool anyTopology = !layout.topologyVertices.empty() ||
                           !layout.topologyEdges.empty() ||
                           !layout.roomBoundaries.empty();
  const bool completeTopology = !layout.topologyVertices.empty() &&
                                !layout.topologyEdges.empty() &&
                                !layout.roomBoundaries.empty();
  if (anyTopology != completeTopology) {
    failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
               "creative_world_layout_encode_incomplete_room_topology"};
    return false;
  }
  for (const CreativeWorldLayoutTopologyVertex& vertex :
       layout.topologyVertices) {
    if (vertex.levelIndex >= layout.levels.size() ||
        !validString(vertex.stableKey) || vertex.stableKey.empty()) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
                 "creative_world_layout_encode_invalid_topology_vertex"};
      return false;
    }
  }
  for (const CreativeWorldLayoutTopologyEdge& edge : layout.topologyEdges) {
    if (edge.levelIndex >= layout.levels.size() ||
        !validString(edge.stableKey) || edge.stableKey.empty() ||
        edge.startVertexIndex >= layout.topologyVertices.size() ||
        edge.endVertexIndex >= layout.topologyVertices.size() ||
        !std::isfinite(edge.wallThicknessCells) ||
        edge.wallThicknessCells <= 0.0 ||
        enumValue(edge.profile) >=
            enumValue(CreativeWorldLayoutWallProfile::Count) ||
        enumValue(edge.material) >=
            enumValue(CreativeStructuralMaterial::Count) ||
        enumValue(edge.joinStyle) >=
            enumValue(CreativeWorldLayoutWallJoinStyle::Count)) {
      failure = {std::isfinite(edge.wallThicknessCells)
                     ? CreativeWorldLayoutCodecStatus::InvalidRecord
                     : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_topology_edge"};
      return false;
    }
  }
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       layout.roomBoundaries) {
    if (boundary.roomIndex >= layout.rooms.size() ||
        boundary.topologyEdgeIndex >= layout.topologyEdges.size()) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
                 "creative_world_layout_encode_invalid_room_boundary"};
      return false;
    }
  }
  for (const CreativeWorldLayoutVerticalConnector& connector :
       layout.verticalConnectors) {
    if (!validKeyName(connector.stableKey, connector.name) ||
        connector.buildingIndex >= layout.buildings.size() ||
        connector.lowerRoomIndex >= layout.rooms.size() ||
        connector.upperRoomIndex >= layout.rooms.size() ||
        connector.lowerRoomIndex == connector.upperRoomIndex ||
        layout.rooms[connector.lowerRoomIndex].buildingIndex !=
            connector.buildingIndex ||
        layout.rooms[connector.upperRoomIndex].buildingIndex !=
            connector.buildingIndex ||
        connector.footprint.minimum.x >= connector.footprint.maximum.x ||
        connector.footprint.minimum.z >= connector.footprint.maximum.z ||
        enumValue(connector.kind) >=
            enumValue(CreativeWorldLayoutVerticalConnectorKind::Count) ||
        enumValue(connector.direction) >=
            enumValue(CreativeWorldLayoutVerticalDirection::Count) ||
        enumValue(connector.material) >=
            enumValue(CreativeStructuralMaterial::Count)) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
                 "creative_world_layout_encode_invalid_vertical_connector"};
      return false;
    }
  }
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    if (!validKeyName(box.stableKey, box.name) || !validRect(box.footprint) ||
        !std::isfinite(box.anchorLayer) || box.layerCount == 0U ||
        enumValue(box.kind) >= enumValue(CreativeObjectKind::Count)) {
      failure = {std::isfinite(box.anchorLayer)
                     ? CreativeWorldLayoutCodecStatus::InvalidRecord
                     : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_box"};
      return false;
    }
  }
  for (const CreativeWorldLayoutWall& wall : layout.walls) {
    if (!validKeyName(wall.stableKey, wall.name) ||
        !std::isfinite(wall.thicknessCells) ||
        enumValue(wall.profile) >=
            enumValue(CreativeWorldLayoutWallProfile::Count) ||
        enumValue(wall.material) >=
            enumValue(CreativeStructuralMaterial::Count) ||
        enumValue(wall.joinStyle) >=
            enumValue(CreativeWorldLayoutWallJoinStyle::Count)) {
      failure = {std::isfinite(wall.thicknessCells)
                     ? CreativeWorldLayoutCodecStatus::InvalidRecord
                     : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_wall"};
      return false;
    }
  }
  for (const CreativeWorldLayoutOpening& opening : layout.openings) {
    const bool finite = std::isfinite(opening.centerOffsetCells) &&
                        std::isfinite(opening.widthCells) &&
                        std::isfinite(opening.cutoutBottomCells) &&
                        std::isfinite(opening.cutoutHeightCells) &&
                        std::isfinite(opening.insertBottomCells) &&
                        std::isfinite(opening.insertHeightCells) &&
                        std::isfinite(opening.insertWidthCells) &&
                        std::isfinite(opening.insertThicknessCells) &&
                        std::isfinite(
                            opening.insertAssetSourceBoundsMeters.min.x) &&
                        std::isfinite(
                            opening.insertAssetSourceBoundsMeters.min.y) &&
                        std::isfinite(
                            opening.insertAssetSourceBoundsMeters.min.z) &&
                        std::isfinite(
                            opening.insertAssetSourceBoundsMeters.max.x) &&
                        std::isfinite(
                            opening.insertAssetSourceBoundsMeters.max.y) &&
                        std::isfinite(
                            opening.insertAssetSourceBoundsMeters.max.z);
    const CreativeBoundsMetrics assetSource =
        measureCreativeBounds(opening.insertAssetSourceBoundsMeters);
    const bool validAsset =
        opening.hasInsertAssetSourceBounds
            ? !opening.insertAssetId.empty() && assetSource.valid &&
                  isPositiveCreativeVec3(assetSource.size)
            : opening.insertAssetId.empty();
    const bool hasDirectRoomHost =
        opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex &&
        opening.roomTopologyEdgeIndex < layout.topologyEdges.size();
    const bool validRoomEdge =
        enumValue(opening.roomEdge) <
            enumValue(CreativeWorldLayoutRoomEdge::Count) ||
        (opening.roomEdge == CreativeWorldLayoutRoomEdge::Count &&
         hasDirectRoomHost);
    if (!validKeyName(opening.stableKey, opening.name) || !finite ||
        !validString(opening.insertAssetId) || !validAsset ||
        enumValue(opening.hostKind) >=
            enumValue(CreativeWorldLayoutOpeningHostKind::Count) ||
        !validRoomEdge ||
        enumValue(opening.kind) >
            enumValue(CreativeBuildingOpeningKind::Window) ||
        enumValue(opening.facing) >=
            enumValue(CreativeBuildingOpeningFacing::Count) ||
        (opening.kind == CreativeBuildingOpeningKind::Door &&
         !isValidCreativeDoorSettings(opening.door)) ||
        (opening.kind == CreativeBuildingOpeningKind::Window &&
         !isValidCreativeWindowSettings(opening.window)) ||
        (opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex &&
         !hasDirectRoomHost)) {
      failure = {finite ? CreativeWorldLayoutCodecStatus::InvalidRecord
                        : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_opening"};
      return false;
    }
  }
  std::vector<std::size_t> roofApertureCounts(layout.levels.size(), 0U);
  for (const CreativeWorldLayoutRoofAperture& aperture :
       layout.roofApertures) {
    const bool finite = std::isfinite(aperture.minimumXCells) &&
                        std::isfinite(aperture.maximumXCells) &&
                        std::isfinite(aperture.minimumZCells) &&
                        std::isfinite(aperture.maximumZCells);
    if (!validKeyName(aperture.stableKey, aperture.name) || !finite ||
        aperture.levelIndex >= layout.levels.size() ||
        aperture.kind >= CreativeStructuralRoofApertureKind::Count ||
        aperture.minimumXCells >= aperture.maximumXCells ||
        aperture.minimumZCells >= aperture.maximumZCells) {
      failure = {finite ? CreativeWorldLayoutCodecStatus::InvalidRecord
                        : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_roof_aperture"};
      return false;
    }
    if (++roofApertureCounts[aperture.levelIndex] >
        kCreativeStructuralRoofApertureCapacity) {
      failure = {CreativeWorldLayoutCodecStatus::CapacityExceeded,
                 "creative_world_layout_encode_roof_aperture_capacity"};
      return false;
    }
  }
  for (const CreativeWorldLayoutObject& object : layout.objects) {
    const bool finite = std::isfinite(object.boundsCells.min.x) &&
                        std::isfinite(object.boundsCells.min.y) &&
                        std::isfinite(object.boundsCells.min.z) &&
                        std::isfinite(object.boundsCells.max.x) &&
                        std::isfinite(object.boundsCells.max.y) &&
                        std::isfinite(object.boundsCells.max.z) &&
                        std::isfinite(object.pointCells.x) &&
                        std::isfinite(object.pointCells.y) &&
                        std::isfinite(object.pointCells.z) &&
                        std::isfinite(object.assetSourceBoundsMeters.min.x) &&
                        std::isfinite(object.assetSourceBoundsMeters.min.y) &&
                        std::isfinite(object.assetSourceBoundsMeters.min.z) &&
                        std::isfinite(object.assetSourceBoundsMeters.max.x) &&
                        std::isfinite(object.assetSourceBoundsMeters.max.y) &&
                        std::isfinite(object.assetSourceBoundsMeters.max.z) &&
                        std::isfinite(object.yawRadians) &&
                        std::isfinite(object.scale.x) &&
                        std::isfinite(object.scale.y) &&
                        std::isfinite(object.scale.z);
    const bool validScale = object.scale.x > 0.0 && object.scale.y > 0.0 &&
                            object.scale.z > 0.0;
    const bool validAssetBounds =
        !object.hasAssetSourceBounds ||
        (object.mode == CreativeObjectLibraryPlacementMode::Point &&
         object.assetSourceBoundsMeters.max.x >
             object.assetSourceBoundsMeters.min.x &&
         object.assetSourceBoundsMeters.max.y >
             object.assetSourceBoundsMeters.min.y &&
         object.assetSourceBoundsMeters.max.z >
             object.assetSourceBoundsMeters.min.z);
    const bool validBoundsModePose =
        object.mode != CreativeObjectLibraryPlacementMode::Bounds ||
        (!object.hasAssetSourceBounds && object.yawRadians == 0.0 &&
         object.scale.x == 1.0 && object.scale.y == 1.0 &&
         object.scale.z == 1.0);
    const bool validBridgeMode =
        !object.usesBridgeRecipe ||
        (object.kind == CreativeObjectKind::Bridge &&
         object.mode == CreativeObjectLibraryPlacementMode::Bounds &&
         object.assetId.empty() && !object.hasAssetSourceBounds &&
         isValidCreativeBridgeSourceRecipe(object.bridge));
    const bool validPlayerSpawn =
        object.kind == CreativeObjectKind::SpawnPoint
            ? isValidCreativePlayerSpawnSettings(object.playerSpawn)
            : object.playerSpawn == CreativePlayerSpawnSettings{};
    const bool npcActor = object.kind == CreativeObjectKind::NpcSpawn ||
                          object.kind == CreativeObjectKind::EnemySpawn;
    const bool validNpcSpawn =
        npcActor ? isValidCreativeNpcSpawnSettings(object.npcSpawn)
                 : object.npcSpawn == CreativeNpcSpawnSettings{};
    if (object.tags.size() >
        kCreativeWorldLayoutCodecMaxRecords - totalTagCount) {
      failure = {CreativeWorldLayoutCodecStatus::CapacityExceeded,
                 "creative_world_layout_encode_tag_capacity_exceeded"};
      return false;
    }
    totalTagCount += object.tags.size();
    if (!validKeyName(object.stableKey, object.name) ||
        !validString(object.assetId) || !finite ||
        enumValue(object.kind) == enumValue(CreativeObjectKind::Unknown) ||
        enumValue(object.kind) >= enumValue(CreativeObjectKind::Count) ||
        enumValue(object.mode) >=
            enumValue(CreativeObjectLibraryPlacementMode::Count) ||
        !validScale || !validAssetBounds || !validBoundsModePose ||
        !validBridgeMode || !validPlayerSpawn || !validNpcSpawn ||
        !std::all_of(object.tags.begin(), object.tags.end(), validString)) {
      failure = {finite ? CreativeWorldLayoutCodecStatus::InvalidRecord
                        : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_object"};
      return false;
    }
  }
  for (const CreativeWorldLayoutTerrainProfile& profile :
       layout.terrainProfiles) {
    CreativeTerrainLandformKind expectedLandformKind =
        CreativeTerrainLandformKind::Count;
    const bool mappedLandformKind = creativeTerrainRecipeLandformKind(
        profile.kind, expectedLandformKind);
    const bool validLandformMode =
        profile.usesLandformRecipe
            ? mappedLandformKind &&
                  expectedLandformKind == profile.landform.kind &&
                  isValidCreativeTerrainLandformRecipe(profile.landform)
            : profile.kind != CreativeTerrainRecipeKind::Terrace &&
                  profile.kind != CreativeTerrainRecipeKind::Cliff;
    const bool validRetainingData =
        profile.retainingEdge.version ==
            kCreativeRetainingEdgeRecipeVersion &&
        validString(profile.retainingEdge.terrainProfileKey) &&
        isValidCreativeRetainingEdgeSettings(
            profile.retainingEdge.settings);
    const bool validRetainingMode =
        !profile.usesRetainingEdgeRecipe ||
        (profile.usesLandformRecipe &&
         profile.landform.edge == CreativeTerrainLandformEdge::Retaining &&
         profile.retainingEdge.terrainProfileKey == profile.stableKey &&
         isValidCreativeRetainingEdgeSourceRecipe(profile.retainingEdge));
    if (!validString(profile.stableKey) ||
        enumValue(profile.kind) >=
            enumValue(CreativeTerrainRecipeKind::Count) ||
        enumValue(profile.blend) >=
            enumValue(CreativeTerrainProfileBlend::Count) ||
        enumValue(profile.rodPolicy) >=
            enumValue(CreativeTerrainProfileRodPolicy::Count) ||
        enumValue(profile.direction) >=
            enumValue(CreativeTerrainProfileDirection::Count) ||
        !validLandformMode || !validRetainingData ||
        !validRetainingMode) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
                 "creative_world_layout_encode_invalid_profile"};
      return false;
    }
  }
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    if (!validString(path.stableKey) ||
        !isValidCreativeTerrainPathSourceRecipe(path.recipe)) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
                 "creative_world_layout_encode_invalid_path"};
      return false;
    }
  }
  return true;
}

void writeRect(std::ostream& output, const CreativeWorldLayoutRect& rect) {
  output << ' ' << rect.minimum.x << ' ' << rect.minimum.z << ' '
         << rect.maximum.x << ' ' << rect.maximum.z;
}

}  // namespace

CreativeWorldLayoutEncodeResult encodeCreativeWorldLayout(
    const CreativeWorldLayout& layout) {
  CreativeWorldLayoutEncodeResult result;
  DecodeFailure failure;
  if (!validateForEncoding(layout, failure)) {
    result.status = failure.status;
    result.reasonCode = std::string{failure.reason};
    return result;
  }

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(std::numeric_limits<double>::max_digits10);
  output << kHeader << ' ' << kCreativeWorldLayoutCodecVersion << '\n';
  std::size_t terrainPathPointCount = 0U;
  std::size_t terrainPathCrossingCount = 0U;
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    terrainPathPointCount += path.recipe.points.size();
    terrainPathCrossingCount += path.recipe.watercourse.crossings.size();
  }
  output << "L " << layout.schemaVersion << ' ' << hexString(layout.stableKey)
         << ' ' << static_cast<unsigned>(enumValue(layout.terrainOwnership))
         << ' ' << layout.buildings.size() << ' ' << layout.levels.size() << ' '
         << layout.rooms.size() << ' ' << layout.topologyVertices.size() << ' '
         << layout.topologyEdges.size() << ' ' << layout.roomBoundaries.size()
         << ' ' << layout.verticalConnectors.size()
         << ' ' << layout.boxes.size() << ' ' << layout.walls.size() << ' '
         << layout.openings.size() << ' ' << layout.objects.size() << ' '
         << layout.terrainProfiles.size() << ' ' << layout.terrainPaths.size()
         << ' ' << terrainPathPointCount << ' ' << terrainPathCrossingCount
         << ' ' << layout.roofApertures.size() << '\n';
  for (const CreativeWorldLayoutBuilding& building : layout.buildings) {
    output << "B " << hexString(building.stableKey) << ' '
           << hexString(building.name) << ' '
           << static_cast<unsigned>(enumValue(building.rootMode));
    writeRect(output, building.rootFootprint);
    output << ' ' << building.rootBaseLayer << ' ' << building.rootHeightCells
           << ' ' << static_cast<unsigned>(enumValue(building.groundingMode))
           << ' ' << building.maximumGroundReliefCells << ' '
           << (building.visible ? 1 : 0) << ' ' << building.tags.size();
    for (const std::string& tag : building.tags) {
      output << ' ' << hexString(tag);
    }
    output << '\n';
  }
  for (const CreativeWorldLayoutLevel& level : layout.levels) {
    output << "V " << level.buildingIndex << ' '
           << hexString(level.stableKey) << ' ' << hexString(level.name) << ' '
           << level.floorTopLayer << ' ' << level.wallHeightCells << ' '
           << level.floorThicknessLayers << ' '
           << level.ceilingThicknessLayers << ' '
           << level.roofThicknessLayers << ' '
           << static_cast<unsigned>(enumValue(level.roofStyle)) << ' '
           << static_cast<unsigned>(enumValue(level.roofRidgeAxis)) << ' '
           << static_cast<unsigned>(enumValue(level.roofSlopeDirection)) << ' '
           << level.roofPitchDegrees << ' ' << level.roofOverhangCells
           << ' ' << static_cast<unsigned>(enumValue(level.roofMaterial))
           << '\n';
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    output << "R " << room.buildingIndex << ' ' << room.levelIndex << ' '
           << hexString(room.stableKey) << ' ' << hexString(room.name);
    writeRect(output, room.footprint);
    output << ' ' << room.wallThicknessCells << ' '
           << static_cast<unsigned>(enumValue(room.type)) << '\n';
  }
  for (const CreativeWorldLayoutTopologyVertex& vertex :
       layout.topologyVertices) {
    output << "N " << vertex.levelIndex << ' ' << hexString(vertex.stableKey)
           << ' ' << vertex.position.x << ' ' << vertex.position.z << '\n';
  }
  for (const CreativeWorldLayoutTopologyEdge& edge : layout.topologyEdges) {
    output << "E " << edge.levelIndex << ' ' << hexString(edge.stableKey) << ' '
           << edge.startVertexIndex << ' ' << edge.endVertexIndex << ' '
           << edge.wallThicknessCells << ' ' << edge.wallHeightCells << ' '
           << static_cast<unsigned>(enumValue(edge.profile)) << ' '
           << static_cast<unsigned>(enumValue(edge.material)) << ' '
           << static_cast<unsigned>(enumValue(edge.joinStyle)) << '\n';
  }
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       layout.roomBoundaries) {
    output << "U " << boundary.roomIndex << ' '
           << boundary.topologyEdgeIndex << ' ' << boundary.order << ' '
           << (boundary.reversed ? 1 : 0) << '\n';
  }
  for (const CreativeWorldLayoutVerticalConnector& connector :
       layout.verticalConnectors) {
    output << "C " << connector.buildingIndex << ' ' << connector.lowerRoomIndex
           << ' ' << connector.upperRoomIndex << ' '
           << static_cast<unsigned>(enumValue(connector.kind)) << ' '
           << static_cast<unsigned>(enumValue(connector.direction)) << ' '
           << hexString(connector.stableKey) << ' '
           << hexString(connector.name);
    writeRect(output, connector.footprint);
    output << ' ' << static_cast<unsigned>(enumValue(connector.material))
           << '\n';
  }
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    output << "X " << box.buildingIndex << ' '
           << static_cast<unsigned>(enumValue(box.kind)) << ' '
           << hexString(box.stableKey) << ' ' << hexString(box.name);
    writeRect(output, box.footprint);
    output << ' ' << box.anchorLayer << ' ' << box.layerCount << '\n';
  }
  for (const CreativeWorldLayoutWall& wall : layout.walls) {
    output << "W " << wall.buildingIndex << ' ' << hexString(wall.stableKey)
           << ' ' << hexString(wall.name) << ' ' << wall.start.x << ' '
           << wall.start.z << ' ' << wall.end.x << ' ' << wall.end.z << ' '
           << wall.baseLayer << ' ' << wall.heightCells << ' '
           << wall.thicknessCells << ' '
           << static_cast<unsigned>(enumValue(wall.profile)) << ' '
           << static_cast<unsigned>(enumValue(wall.material)) << ' '
           << static_cast<unsigned>(enumValue(wall.joinStyle)) << '\n';
  }
  for (const CreativeWorldLayoutOpening& opening : layout.openings) {
    output << "O " << static_cast<unsigned>(enumValue(opening.hostKind)) << ' '
           << opening.wallIndex << ' ' << opening.roomIndex << ' '
           << static_cast<unsigned>(enumValue(opening.roomEdge)) << ' '
           << static_cast<unsigned>(enumValue(opening.kind)) << ' '
           << static_cast<unsigned>(enumValue(opening.facing)) << ' '
           << static_cast<unsigned>(enumValue(opening.door.leafArrangement))
           << ' ' << static_cast<unsigned>(enumValue(opening.door.hingeSide))
           << ' ' << static_cast<unsigned>(enumValue(opening.door.swingSide))
           << ' ' << static_cast<unsigned>(enumValue(opening.door.initialState))
           << ' ' << (opening.door.gameplayLocked ? 1 : 0) << ' '
           << opening.door.transitionSeconds << ' '
           << static_cast<unsigned>(enumValue(opening.window.insertKind))
           << ' '
           << hexString(opening.stableKey) << ' ' << hexString(opening.name)
           << ' ' << opening.centerOffsetCells << ' ' << opening.widthCells
           << ' ' << opening.cutoutBottomCells << ' '
           << opening.cutoutHeightCells << ' '
           << (opening.includeInsert ? 1 : 0) << ' '
           << opening.insertBottomCells << ' ' << opening.insertHeightCells
           << ' ' << opening.insertWidthCells << ' '
           << opening.insertThicknessCells << ' '
           << hexString(opening.insertAssetId) << ' '
           << (opening.hasInsertAssetSourceBounds ? 1 : 0) << ' '
           << opening.insertAssetSourceBoundsMeters.min.x << ' '
           << opening.insertAssetSourceBoundsMeters.min.y << ' '
           << opening.insertAssetSourceBoundsMeters.min.z << ' '
           << opening.insertAssetSourceBoundsMeters.max.x << ' '
           << opening.insertAssetSourceBoundsMeters.max.y << ' '
           << opening.insertAssetSourceBoundsMeters.max.z << ' '
           << opening.roomTopologyEdgeIndex << '\n';
  }
  for (const CreativeWorldLayoutObject& object : layout.objects) {
    output << "Y " << static_cast<unsigned>(enumValue(object.kind)) << ' '
           << static_cast<unsigned>(enumValue(object.mode)) << ' '
           << hexString(object.stableKey) << ' ' << hexString(object.name)
           << ' ' << hexString(object.assetId) << ' '
           << (object.visible ? 1 : 0) << ' ' << object.boundsCells.min.x << ' '
           << object.boundsCells.min.y << ' ' << object.boundsCells.min.z << ' '
           << object.boundsCells.max.x << ' ' << object.boundsCells.max.y << ' '
           << object.boundsCells.max.z << ' ' << object.pointCells.x << ' '
           << object.pointCells.y << ' ' << object.pointCells.z << ' '
           << (object.hasAssetSourceBounds ? 1 : 0) << ' '
           << object.assetSourceBoundsMeters.min.x << ' '
           << object.assetSourceBoundsMeters.min.y << ' '
           << object.assetSourceBoundsMeters.min.z << ' '
           << object.assetSourceBoundsMeters.max.x << ' '
           << object.assetSourceBoundsMeters.max.y << ' '
           << object.assetSourceBoundsMeters.max.z << ' ' << object.yawRadians
           << ' ' << object.scale.x << ' ' << object.scale.y << ' '
           << object.scale.z << ' '
           << (object.usesBridgeRecipe ? 1 : 0) << ' '
           << object.bridge.version << ' '
           << static_cast<unsigned>(enumValue(object.bridge.attachment)) << ' '
           << hexString(object.bridge.watercoursePathKey) << ' '
           << object.bridge.crossingId << ' '
           << object.bridge.settings.deckWidthMeters << ' '
           << object.bridge.settings.deckThicknessMeters << ' '
           << object.bridge.settings.deckElevationOffsetMeters << ' '
           << object.bridge.settings.maximumSpanMeters << ' '
           << object.bridge.settings.minimumClearanceMeters << ' '
           << static_cast<unsigned>(
                  enumValue(object.bridge.settings.supportStyle))
           << ' ' << object.bridge.settings.supportSpacingMeters << ' '
           << object.bridge.settings.supportWidthMeters << ' '
           << object.bridge.settings.supportDepthMeters << ' '
           << (object.bridge.settings.rails ? 1 : 0) << ' '
           << object.bridge.settings.railHeightMeters << ' '
           << object.bridge.settings.railThicknessMeters << ' '
           << object.bridge.settings.maximumApproachGradePermille << ' '
           << object.bridge.settings.approachFalloffCells << ' '
           << static_cast<unsigned>(
                  enumValue(object.bridge.settings.materials.deck))
           << ' '
           << static_cast<unsigned>(
                  enumValue(object.bridge.settings.materials.supports))
           << ' '
           << static_cast<unsigned>(
                  enumValue(object.bridge.settings.materials.rails))
           << ' ' << hexString(object.playerSpawn.playerProfileId) << ' '
           << hexString(object.playerSpawn.spawnGroup) << ' '
           << object.playerSpawn.validationRadiusMeters << ' '
           << object.playerSpawn.fallbackPriority << ' '
           << hexString(object.npcSpawn.behaviorProfileId) << ' '
           << static_cast<unsigned>(enumValue(object.npcSpawn.team)) << ' '
           << object.npcSpawn.hitPoints << ' '
           << object.npcSpawn.initialAlertLevel << ' '
           << static_cast<unsigned>(enumValue(object.npcSpawn.spawnPolicy))
           << ' ' << object.tags.size();
    for (const std::string& tag : object.tags) {
      output << ' ' << hexString(tag);
    }
    output << '\n';
  }
  for (const CreativeWorldLayoutTerrainProfile& profile :
       layout.terrainProfiles) {
    output << "P " << hexString(profile.stableKey) << ' '
           << static_cast<unsigned>(enumValue(profile.kind)) << ' '
           << profile.center.x << ' ' << profile.center.z << ' '
           << profile.baseHeightCells << ' ' << profile.radiusCells << ' '
           << profile.amplitudeCells << ' ' << profile.spacingCells << ' '
           << static_cast<unsigned>(enumValue(profile.blend)) << ' '
           << static_cast<unsigned>(enumValue(profile.rodPolicy)) << ' '
           << static_cast<unsigned>(enumValue(profile.direction)) << ' '
           << static_cast<unsigned>(profile.frequency) << ' '
           << (profile.usesLandformRecipe ? 1 : 0) << ' '
           << profile.landform.version << ' '
           << static_cast<unsigned>(enumValue(profile.landform.kind)) << ' '
           << profile.landform.bounds.minimum.x << ' '
           << profile.landform.bounds.minimum.z << ' '
           << profile.landform.bounds.widthCells << ' '
           << profile.landform.bounds.depthCells << ' '
           << profile.landform.baseHeightCells << ' '
           << profile.landform.targetHeightCells << ' '
           << static_cast<unsigned>(profile.landform.terraceCount) << ' '
           << static_cast<unsigned>(enumValue(profile.landform.direction))
           << ' ' << static_cast<unsigned>(enumValue(profile.landform.edge))
           << ' ' << profile.landform.edgeWidthCells << ' '
           << profile.landform.featherCells << ' '
           << (profile.landform.paintSurface ? 1 : 0) << ' '
           << static_cast<unsigned>(enumValue(profile.landform.material))
           << ' '
           << static_cast<unsigned>(enumValue(profile.landform.erosion))
           << ' ' << profile.landform.erosionReliefCells << ' '
           << profile.landform.seed << ' '
           << (profile.usesRetainingEdgeRecipe ? 1 : 0) << ' '
           << profile.retainingEdge.version << ' '
           << hexString(profile.retainingEdge.terrainProfileKey) << ' '
           << static_cast<unsigned>(
                  enumValue(profile.retainingEdge.settings.selection))
           << ' '
           << static_cast<unsigned>(
                  enumValue(profile.retainingEdge.settings.kit))
           << ' ' << profile.retainingEdge.settings.thicknessMeters << ' '
           << profile.retainingEdge.settings.maximumHeightMeters << ' '
           << (profile.retainingEdge.settings.closeCorners ? 1 : 0) << ' '
           << (profile.retainingEdge.settings.capEnds ? 1 : 0) << ' '
           << static_cast<unsigned>(
                  enumValue(profile.retainingEdge.settings.material))
           << ' ' << profile.retainingEdge.settings.transitionCount;
    for (std::size_t index = 0U;
         index < profile.retainingEdge.settings.transitionCount; ++index) {
      const CreativeRetainingEdgeTransition& transition =
          profile.retainingEdge.settings.transitions[index];
      output << ' ' << transition.edge.first.x << ' '
             << transition.edge.first.z << ' ' << transition.edge.second.x
             << ' ' << transition.edge.second.z << ' '
             << static_cast<unsigned>(enumValue(transition.kind)) << ' '
             << transition.runCells;
    }
    output << '\n';
  }
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    output << "T " << hexString(path.stableKey) << ' '
           << path.recipe.version << ' '
           << static_cast<unsigned>(enumValue(path.recipe.kind)) << ' '
           << static_cast<unsigned>(enumValue(path.recipe.elevation)) << ' '
           << static_cast<unsigned>(enumValue(path.recipe.curve)) << ' '
           << static_cast<unsigned>(enumValue(path.recipe.crossSection)) << ' '
           << static_cast<unsigned>(enumValue(path.recipe.startJoin)) << ' '
           << static_cast<unsigned>(enumValue(path.recipe.endJoin)) << ' '
           << path.recipe.falloffCells << ' '
           << (path.recipe.paintSurface ? 1 : 0) << ' '
           << static_cast<unsigned>(enumValue(path.recipe.material)) << ' '
           << path.recipe.road.shoulderWidthCells << ' '
           << path.recipe.road.maximumGradePermille << ' '
           << static_cast<unsigned>(
                  enumValue(path.recipe.road.edgeTreatment))
           << ' ' << path.recipe.road.edgeWidthMeters << ' '
           << path.recipe.road.edgeHeightMeters << ' '
           << static_cast<unsigned>(enumValue(path.recipe.road.edgeMaterial))
           << ' ' << path.recipe.watercourse.bankSlopeCells << ' '
           << static_cast<unsigned>(
                  enumValue(path.recipe.watercourse.drainageDirection))
           << ' '
           << static_cast<unsigned>(
                  enumValue(path.recipe.watercourse.surfacePolicy))
           << ' ' << path.recipe.watercourse.surfaceInsetCells << ' '
           << path.recipe.watercourse.nextCrossingId << ' '
           << path.recipe.watercourse.crossings.size() << ' '
           << path.recipe.nextPointId << ' ' << path.recipe.points.size()
           << '\n';
  }
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    for (const CreativeTerrainPathSourcePoint& point : path.recipe.points) {
      output << "Q " << point.id << ' ' << point.coord.x << ' '
             << point.coord.z << ' ' << point.heightCells << ' '
             << point.halfWidthCells << ' ' << point.amplitudeCells << ' '
             << point.bankPermille << '\n';
    }
  }
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    for (const CreativeTerrainWatercourseCrossing& crossing :
         path.recipe.watercourse.crossings) {
      output << "K " << crossing.id << ' ' << crossing.pointId << ' '
             << crossing.bankClearanceCells << ' '
             << crossing.deckClearanceCells << ' '
             << crossing.approachLengthCells << '\n';
    }
  }
  for (const CreativeWorldLayoutRoofAperture& aperture :
       layout.roofApertures) {
    output << "A " << aperture.levelIndex << ' '
           << static_cast<unsigned>(enumValue(aperture.kind)) << ' '
           << hexString(aperture.stableKey) << ' '
           << hexString(aperture.name) << ' ' << aperture.minimumXCells << ' '
           << aperture.maximumXCells << ' ' << aperture.minimumZCells << ' '
           << aperture.maximumZCells << '\n';
  }
  output << "END\n";

  result.encodedText = output.str();
  if (result.encodedText.size() > kCreativeWorldLayoutCodecMaxEncodedBytes) {
    result.encodedText.clear();
    result.status = CreativeWorldLayoutCodecStatus::EncodedSizeExceeded;
    result.reasonCode = "creative_world_layout_encode_size_exceeded";
    return result;
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutCodecStatus::Ready;
  result.reasonCode = "creative_world_layout_encoded";
  return result;
}

}  // namespace iggy3d::creative
