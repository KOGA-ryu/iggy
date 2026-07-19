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
  const std::size_t counts[] = {
      layout.buildings.size(),
      layout.levels.size(),
      layout.rooms.size(),
      layout.verticalConnectors.size(),
      layout.boxes.size(),
      layout.walls.size(),
      layout.openings.size(),
      layout.objects.size(),
      layout.terrainProfiles.size(),
      layout.terrainPaths.size(),
      layout.terrainPathPoints.size(),
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
            level.roofStyle, level.roofRidgeAxis,
            level.roofPitchDegrees, level.roofOverhangCells) ||
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
        !std::isfinite(room.wallThicknessCells)) {
      failure = {std::isfinite(room.wallThicknessCells)
                     ? CreativeWorldLayoutCodecStatus::InvalidRecord
                     : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_room"};
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
            enumValue(CreativeWorldLayoutVerticalDirection::Count)) {
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
        !std::isfinite(wall.thicknessCells)) {
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
    if (!validKeyName(opening.stableKey, opening.name) || !finite ||
        !validString(opening.insertAssetId) || !validAsset ||
        enumValue(opening.hostKind) >=
            enumValue(CreativeWorldLayoutOpeningHostKind::Count) ||
        enumValue(opening.roomEdge) >=
            enumValue(CreativeWorldLayoutRoomEdge::Count) ||
        enumValue(opening.kind) >
            enumValue(CreativeBuildingOpeningKind::Window) ||
        enumValue(opening.pose) >
            enumValue(CreativeBuildingOpeningPose::OpenFromEndPositiveNormal)) {
      failure = {finite ? CreativeWorldLayoutCodecStatus::InvalidRecord
                        : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_opening"};
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
        !std::all_of(object.tags.begin(), object.tags.end(), validString)) {
      failure = {finite ? CreativeWorldLayoutCodecStatus::InvalidRecord
                        : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_object"};
      return false;
    }
  }
  for (const CreativeWorldLayoutTerrainProfile& profile :
       layout.terrainProfiles) {
    if (!validString(profile.stableKey) ||
        enumValue(profile.kind) >=
            enumValue(CreativeTerrainRecipeKind::Count) ||
        enumValue(profile.blend) >=
            enumValue(CreativeTerrainProfileBlend::Count) ||
        enumValue(profile.rodPolicy) >=
            enumValue(CreativeTerrainProfileRodPolicy::Count) ||
        enumValue(profile.direction) >=
            enumValue(CreativeTerrainProfileDirection::Count)) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
                 "creative_world_layout_encode_invalid_profile"};
      return false;
    }
  }
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    if (!validString(path.stableKey) ||
        enumValue(path.kind) >= enumValue(CreativeTerrainRecipeKind::Count) ||
        enumValue(path.elevation) >=
            enumValue(CreativeTerrainPathElevation::Count) ||
        enumValue(path.material) > enumValue(CreativeTerrainMaterial::Count)) {
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
  output << "L " << layout.schemaVersion << ' ' << hexString(layout.stableKey)
         << ' ' << static_cast<unsigned>(enumValue(layout.terrainOwnership))
         << ' ' << layout.buildings.size() << ' ' << layout.levels.size() << ' '
         << layout.rooms.size() << ' ' << layout.verticalConnectors.size()
         << ' ' << layout.boxes.size() << ' ' << layout.walls.size() << ' '
         << layout.openings.size() << ' ' << layout.objects.size() << ' '
         << layout.terrainProfiles.size() << ' ' << layout.terrainPaths.size()
         << ' ' << layout.terrainPathPoints.size() << '\n';
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
           << level.roofPitchDegrees << ' ' << level.roofOverhangCells
           << '\n';
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    output << "R " << room.buildingIndex << ' ' << room.levelIndex << ' '
           << hexString(room.stableKey) << ' ' << hexString(room.name);
    writeRect(output, room.footprint);
    output << ' ' << room.wallThicknessCells << '\n';
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
    output << '\n';
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
           << wall.thicknessCells << '\n';
  }
  for (const CreativeWorldLayoutOpening& opening : layout.openings) {
    output << "O " << static_cast<unsigned>(enumValue(opening.hostKind)) << ' '
           << opening.wallIndex << ' ' << opening.roomIndex << ' '
           << static_cast<unsigned>(enumValue(opening.roomEdge)) << ' '
           << static_cast<unsigned>(enumValue(opening.kind)) << ' '
           << static_cast<unsigned>(enumValue(opening.pose)) << ' '
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
           << opening.insertAssetSourceBoundsMeters.max.z << '\n';
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
           << object.scale.z << ' ' << object.tags.size();
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
           << static_cast<unsigned>(profile.frequency) << '\n';
  }
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    output << "T " << hexString(path.stableKey) << ' '
           << static_cast<unsigned>(enumValue(path.kind)) << ' '
           << path.firstPointIndex << ' ' << path.pointCount << ' '
           << static_cast<unsigned>(enumValue(path.elevation)) << ' '
           << path.halfWidthCells << ' ' << path.amplitudeCells << ' '
           << (path.paintSurface ? 1 : 0) << ' '
           << static_cast<unsigned>(enumValue(path.material)) << '\n';
  }
  for (const CreativeTerrainPathPoint& point : layout.terrainPathPoints) {
    output << "Q " << point.coord.x << ' ' << point.coord.z << ' '
           << point.heightCells << '\n';
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
