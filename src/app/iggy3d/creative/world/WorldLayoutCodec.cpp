#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
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

class RecordReader {
 public:
  explicit RecordReader(std::string_view line) {
    std::size_t begin = 0U;
    while (begin < line.size()) {
      while (begin < line.size() && line[begin] == ' ') {
        ++begin;
      }
      if (begin == line.size()) {
        break;
      }
      const std::size_t end = line.find(' ', begin);
      tokens_.push_back(line.substr(begin, end == std::string_view::npos
                                               ? line.size() - begin
                                               : end - begin));
      begin = end == std::string_view::npos ? line.size() : end + 1U;
    }
  }

  bool readLiteral(std::string_view expected) {
    std::string_view token;
    return next(token) && token == expected;
  }

  template <typename T>
  bool readUnsigned(T& output) {
    static_assert(std::is_unsigned_v<T>);
    std::string_view token;
    if (!next(token) || token.empty() || token.front() == '+') {
      return false;
    }
    std::uint64_t parsed = 0U;
    const auto conversion =
        std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (conversion.ec != std::errc{} ||
        conversion.ptr != token.data() + token.size() ||
        parsed > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) {
      return false;
    }
    output = static_cast<T>(parsed);
    return true;
  }

  bool readSize(std::size_t& output) {
    std::uint64_t parsed = 0U;
    if (!readUnsigned(parsed) ||
        parsed > static_cast<std::uint64_t>(
                     std::numeric_limits<std::size_t>::max())) {
      return false;
    }
    output = static_cast<std::size_t>(parsed);
    return true;
  }

  bool readI32(std::int32_t& output) {
    std::string_view token;
    if (!next(token) || token.empty() || token.front() == '+') {
      return false;
    }
    std::int64_t parsed = 0;
    const auto conversion =
        std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (conversion.ec != std::errc{} ||
        conversion.ptr != token.data() + token.size() ||
        parsed < std::numeric_limits<std::int32_t>::min() ||
        parsed > std::numeric_limits<std::int32_t>::max()) {
      return false;
    }
    output = static_cast<std::int32_t>(parsed);
    return true;
  }

  bool readDouble(double& output) {
    std::string_view token;
    if (!next(token) || token.empty()) {
      return false;
    }
    std::string owned{token};
    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(owned.c_str(), &end);
    if (errno == ERANGE || end != owned.c_str() + owned.size()) {
      return false;
    }
    output = parsed;
    return true;
  }

  bool readBool(bool& output) {
    std::uint8_t value = 0U;
    if (!readUnsigned(value) || value > 1U) {
      return false;
    }
    output = value == 1U;
    return true;
  }

  bool readHex(std::string& output) {
    std::string_view token;
    if (!next(token)) {
      return false;
    }
    if (token == "-") {
      output.clear();
      return true;
    }
    if (token.empty() || token.size() % 2U != 0U ||
        token.size() / 2U > kCreativeWorldLayoutCodecMaxStringBytes) {
      return false;
    }
    output.clear();
    output.reserve(token.size() / 2U);
    for (std::size_t index = 0U; index < token.size(); index += 2U) {
      const int high = hexValue(token[index]);
      const int low = hexValue(token[index + 1U]);
      if (high < 0 || low < 0) {
        return false;
      }
      output.push_back(static_cast<char>((high << 4) | low));
    }
    return true;
  }

  [[nodiscard]] bool finished() const noexcept {
    return cursor_ == tokens_.size();
  }

 private:
  static int hexValue(char value) noexcept {
    if (value >= '0' && value <= '9') {
      return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
      return 10 + value - 'a';
    }
    return -1;
  }

  bool next(std::string_view& output) {
    if (cursor_ >= tokens_.size()) {
      return false;
    }
    output = tokens_[cursor_++];
    return true;
  }

  std::vector<std::string_view> tokens_;
  std::size_t cursor_ = 0U;
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

bool readRect(RecordReader& reader, CreativeWorldLayoutRect& rect) {
  return reader.readI32(rect.minimum.x) && reader.readI32(rect.minimum.z) &&
         reader.readI32(rect.maximum.x) && reader.readI32(rect.maximum.z);
}

template <typename Enum>
bool readEnum(RecordReader& reader, Enum& output,
              std::underlying_type_t<Enum> maximumInclusive) {
  using Underlying = std::underlying_type_t<Enum>;
  static_assert(std::is_unsigned_v<Underlying>);
  Underlying parsed = 0;
  if (!reader.readUnsigned(parsed) || parsed > maximumInclusive) {
    return false;
  }
  output = static_cast<Enum>(parsed);
  return true;
}

template <typename T, typename ReadRecord>
bool readTable(std::vector<std::string_view>& lines, std::size_t& lineIndex,
               std::size_t count, std::vector<T>& output, ReadRecord readRecord,
               CreativeWorldLayoutDecodeResult& result) {
  output.clear();
  output.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    if (lineIndex >= lines.size()) {
      result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
      result.failedLine = lineIndex + 1U;
      result.reasonCode = "creative_world_layout_decode_truncated";
      return false;
    }
    RecordReader reader(lines[lineIndex]);
    T item;
    if (!readRecord(reader, item) || !reader.finished()) {
      result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
      result.failedLine = lineIndex + 1U;
      result.reasonCode = "creative_world_layout_decode_invalid_record";
      return false;
    }
    output.push_back(std::move(item));
    ++lineIndex;
  }
  return true;
}

std::vector<std::string_view> splitLines(std::string_view text) {
  std::vector<std::string_view> lines;
  std::size_t begin = 0U;
  while (begin < text.size()) {
    const std::size_t end = text.find('\n', begin);
    std::string_view line =
        text.substr(begin, end == std::string_view::npos ? text.size() - begin
                                                         : end - begin);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1U);
    }
    lines.push_back(line);
    begin = end == std::string_view::npos ? text.size() : end + 1U;
  }
  return lines;
}

}  // namespace

std::string_view toString(CreativeWorldLayoutCodecStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutCodecStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutCodecStatus::Ready:
      return "Ready";
    case CreativeWorldLayoutCodecStatus::EmptyInput:
      return "EmptyInput";
    case CreativeWorldLayoutCodecStatus::EncodedSizeExceeded:
      return "EncodedSizeExceeded";
    case CreativeWorldLayoutCodecStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeWorldLayoutCodecStatus::InvalidHeader:
      return "InvalidHeader";
    case CreativeWorldLayoutCodecStatus::InvalidRecord:
      return "InvalidRecord";
    case CreativeWorldLayoutCodecStatus::InvalidNumber:
      return "InvalidNumber";
    case CreativeWorldLayoutCodecStatus::InvalidEnum:
      return "InvalidEnum";
    case CreativeWorldLayoutCodecStatus::InvalidString:
      return "InvalidString";
    case CreativeWorldLayoutCodecStatus::NonFiniteValue:
      return "NonFiniteValue";
    case CreativeWorldLayoutCodecStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeWorldLayoutCodecStatus::TrailingData:
      return "TrailingData";
  }
  return "Unknown";
}

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

CreativeWorldLayoutDecodeResult decodeCreativeWorldLayout(
    std::string_view encodedText) {
  CreativeWorldLayoutDecodeResult result;
  if (encodedText.empty()) {
    result.status = CreativeWorldLayoutCodecStatus::EmptyInput;
    result.reasonCode = "creative_world_layout_decode_empty";
    return result;
  }
  if (encodedText.size() > kCreativeWorldLayoutCodecMaxEncodedBytes) {
    result.status = CreativeWorldLayoutCodecStatus::EncodedSizeExceeded;
    result.reasonCode = "creative_world_layout_decode_size_exceeded";
    return result;
  }
  std::vector<std::string_view> lines = splitLines(encodedText);
  if (lines.size() < 3U || lines.size() > kCreativeWorldLayoutCodecMaxRecords) {
    result.status = CreativeWorldLayoutCodecStatus::CapacityExceeded;
    result.reasonCode = "creative_world_layout_decode_line_capacity";
    return result;
  }

  RecordReader header(lines[0]);
  std::uint32_t codecVersion = 0U;
  if (!header.readLiteral(kHeader) || !header.readUnsigned(codecVersion) ||
      !header.finished()) {
    result.status = CreativeWorldLayoutCodecStatus::InvalidHeader;
    result.failedLine = 1U;
    result.reasonCode = "creative_world_layout_decode_invalid_header";
    return result;
  }
  if (codecVersion == 0U || codecVersion > kCreativeWorldLayoutCodecVersion) {
    result.status = CreativeWorldLayoutCodecStatus::UnsupportedVersion;
    result.failedLine = 1U;
    result.reasonCode = "creative_world_layout_decode_unsupported_version";
    return result;
  }

  RecordReader layoutRecord(lines[1]);
  std::uint8_t ownership = 0U;
  std::size_t buildingCount = 0U;
  std::size_t levelCount = 0U;
  std::size_t roomCount = 0U;
  std::size_t connectorCount = 0U;
  std::size_t boxCount = 0U;
  std::size_t wallCount = 0U;
  std::size_t openingCount = 0U;
  std::size_t objectCount = 0U;
  std::size_t profileCount = 0U;
  std::size_t pathCount = 0U;
  std::size_t pointCount = 0U;
  const bool layoutPrefix =
      !layoutRecord.readLiteral("L") ||
      !layoutRecord.readUnsigned(result.layout.schemaVersion) ||
      !layoutRecord.readHex(result.layout.stableKey) ||
      !layoutRecord.readUnsigned(ownership) || ownership > 1U ||
      !layoutRecord.readSize(buildingCount);
  const bool levelCountInvalid =
      codecVersion >= 6U && !layoutRecord.readSize(levelCount);
  const bool roomCountInvalid =
      codecVersion >= 2U && !layoutRecord.readSize(roomCount);
  const bool connectorCountInvalid =
      codecVersion >= 7U && !layoutRecord.readSize(connectorCount);
  const bool structuralCountInvalid =
      !layoutRecord.readSize(boxCount) || !layoutRecord.readSize(wallCount) ||
      !layoutRecord.readSize(openingCount);
  const bool objectCountInvalid =
      codecVersion >= 3U && !layoutRecord.readSize(objectCount);
  if (layoutPrefix || levelCountInvalid || roomCountInvalid ||
      connectorCountInvalid || structuralCountInvalid || objectCountInvalid ||
      !layoutRecord.readSize(profileCount) ||
      !layoutRecord.readSize(pathCount) || !layoutRecord.readSize(pointCount) ||
      !layoutRecord.finished()) {
    result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
    result.failedLine = 2U;
    result.reasonCode = "creative_world_layout_decode_invalid_layout_record";
    return result;
  }
  const std::uint32_t expectedSchemaVersion = codecVersion;
  if (result.layout.schemaVersion != expectedSchemaVersion) {
    result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
    result.failedLine = 2U;
    result.reasonCode = "creative_world_layout_decode_schema_mismatch";
    return result;
  }
  std::size_t declaredRecords = 3U;
  const std::size_t declaredCounts[] = {
      buildingCount, levelCount, roomCount,    connectorCount,
      boxCount,      wallCount,  openingCount, objectCount,
      profileCount,  pathCount,  pointCount,
  };
  for (const std::size_t count : declaredCounts) {
    if (count > kCreativeWorldLayoutCodecMaxRecords - declaredRecords) {
      result.status = CreativeWorldLayoutCodecStatus::CapacityExceeded;
      result.failedLine = 2U;
      result.reasonCode = "creative_world_layout_decode_declared_capacity";
      return result;
    }
    declaredRecords += count;
  }
  if (declaredRecords != lines.size()) {
    result.status = CreativeWorldLayoutCodecStatus::CapacityExceeded;
    result.failedLine = 2U;
    result.reasonCode = "creative_world_layout_decode_declared_capacity";
    return result;
  }
  result.layout.terrainOwnership =
      static_cast<CreativeWorldLayoutTerrainOwnership>(ownership);

  std::size_t lineIndex = 2U;
  std::size_t totalTagCount = 0U;
  const auto readBuilding = [codecVersion, &totalTagCount](
                                RecordReader& reader,
                                CreativeWorldLayoutBuilding& building) {
    std::uint8_t rootMode = 0U;
    std::uint8_t groundingMode = 0U;
    std::size_t tagCount = 0U;
    if (!reader.readLiteral("B") || !reader.readHex(building.stableKey) ||
        !reader.readHex(building.name) || !reader.readUnsigned(rootMode) ||
        rootMode > enumValue(CreativeBuildingRootMode::ExistingRoom) ||
        !readRect(reader, building.rootFootprint) ||
        !reader.readI32(building.rootBaseLayer) ||
        !reader.readUnsigned(building.rootHeightCells) ||
        (codecVersion >= 11U &&
         (!reader.readUnsigned(groundingMode) ||
          groundingMode >=
              enumValue(CreativeWorldLayoutGroundingMode::Count) ||
          !reader.readUnsigned(building.maximumGroundReliefCells))) ||
        !reader.readBool(building.visible) || !reader.readSize(tagCount) ||
        tagCount > kCreativeWorldLayoutCodecMaxRecords - totalTagCount) {
      return false;
    }
    totalTagCount += tagCount;
    building.rootMode = static_cast<CreativeBuildingRootMode>(rootMode);
    building.groundingMode =
        static_cast<CreativeWorldLayoutGroundingMode>(groundingMode);
    building.tags.resize(tagCount);
    for (std::string& tag : building.tags) {
      if (!reader.readHex(tag)) {
        return false;
      }
    }
    return true;
  };
  if (!readTable(lines, lineIndex, buildingCount, result.layout.buildings,
                 readBuilding, result)) {
    return result;
  }
  const auto readLevel = [codecVersion](RecordReader& reader,
                                        CreativeWorldLayoutLevel& level) {
    const bool base =
        reader.readLiteral("V") && reader.readSize(level.buildingIndex) &&
        reader.readHex(level.stableKey) && reader.readHex(level.name) &&
        reader.readDouble(level.floorTopLayer) &&
        std::isfinite(level.floorTopLayer) &&
        reader.readUnsigned(level.wallHeightCells) &&
        reader.readUnsigned(level.floorThicknessLayers) &&
        reader.readUnsigned(level.ceilingThicknessLayers) &&
        reader.readUnsigned(level.roofThicknessLayers) &&
        level.wallHeightCells > 0U && level.floorThicknessLayers > 0U &&
        level.ceilingThicknessLayers > 0U &&
        level.roofThicknessLayers > 0U;
    if (!base || codecVersion < 8U) {
      return base;
    }
    std::uint8_t roofStyle = 0U;
    std::uint8_t roofRidgeAxis = 0U;
    if (!reader.readUnsigned(roofStyle) ||
        !reader.readUnsigned(roofRidgeAxis) ||
        !reader.readDouble(level.roofPitchDegrees) ||
        !reader.readDouble(level.roofOverhangCells)) {
      return false;
    }
    level.roofStyle = static_cast<CreativeStructuralRoofStyle>(roofStyle);
    level.roofRidgeAxis =
        static_cast<CreativeStructuralRoofRidgeAxis>(roofRidgeAxis);
    return validCreativeStructuralRoofSettings(
               level.roofStyle, level.roofRidgeAxis,
               level.roofPitchDegrees, level.roofOverhangCells) &&
           level.roofOverhangCells <=
               kMaximumCreativeWorldLayoutRoofOverhangCells;
  };
  if (!readTable(lines, lineIndex, levelCount, result.layout.levels, readLevel,
                 result)) {
    return result;
  }

  struct LegacyRoomGeometry {
    double floorTopLayer = 0.0;
    std::uint16_t wallHeightCells = 0U;
    std::uint16_t floorThicknessLayers = 0U;
  };
  struct LegacyRoomRecord {
    CreativeWorldLayoutRoom room;
    LegacyRoomGeometry geometry;
  };
  std::vector<LegacyRoomGeometry> legacyRoomGeometry;
  if (codecVersion >= 6U) {
    const auto readRoom = [](RecordReader& reader,
                             CreativeWorldLayoutRoom& room) {
      return reader.readLiteral("R") && reader.readSize(room.buildingIndex) &&
             reader.readSize(room.levelIndex) &&
             reader.readHex(room.stableKey) && reader.readHex(room.name) &&
             readRect(reader, room.footprint) &&
             reader.readDouble(room.wallThicknessCells) &&
             std::isfinite(room.wallThicknessCells);
    };
    if (!readTable(lines, lineIndex, roomCount, result.layout.rooms, readRoom,
                   result)) {
      return result;
    }
  } else {
    const auto readLegacyRoom = [codecVersion](RecordReader& reader,
                                                LegacyRoomRecord& record) {
      std::int32_t legacyBaseLayer = 0;
      const bool prefix = reader.readLiteral("R") &&
                          reader.readSize(record.room.buildingIndex) &&
                          reader.readHex(record.room.stableKey) &&
                          reader.readHex(record.room.name) &&
                          readRect(reader, record.room.footprint);
      const bool elevation =
          codecVersion >= 4U
              ? reader.readDouble(record.geometry.floorTopLayer)
              : reader.readI32(legacyBaseLayer);
      const bool suffix =
          reader.readUnsigned(record.geometry.wallHeightCells) &&
          reader.readDouble(record.room.wallThicknessCells) &&
          std::isfinite(record.room.wallThicknessCells) &&
          reader.readUnsigned(record.geometry.floorThicknessLayers);
      if (!prefix || !elevation || !suffix ||
          !std::isfinite(record.geometry.floorTopLayer)) {
        return false;
      }
      if (codecVersion < 4U) {
        record.geometry.floorTopLayer =
            static_cast<double>(legacyBaseLayer) +
            static_cast<double>(record.geometry.floorThicknessLayers) * 0.5;
      }
      return true;
    };
    std::vector<LegacyRoomRecord> legacyRooms;
    if (!readTable(lines, lineIndex, roomCount, legacyRooms, readLegacyRoom,
                   result)) {
      return result;
    }
    result.layout.rooms.reserve(legacyRooms.size());
    legacyRoomGeometry.reserve(legacyRooms.size());
    for (LegacyRoomRecord& record : legacyRooms) {
      result.layout.rooms.push_back(std::move(record.room));
      legacyRoomGeometry.push_back(record.geometry);
    }
  }
  if (codecVersion >= 7U) {
    const auto readConnector = [](RecordReader& reader,
                                  CreativeWorldLayoutVerticalConnector& value) {
      std::uint8_t kind = 0U;
      std::uint8_t direction = 0U;
      if (!reader.readLiteral("C") || !reader.readSize(value.buildingIndex) ||
          !reader.readSize(value.lowerRoomIndex) ||
          !reader.readSize(value.upperRoomIndex) ||
          !reader.readUnsigned(kind) ||
          kind >= enumValue(CreativeWorldLayoutVerticalConnectorKind::Count) ||
          !reader.readUnsigned(direction) ||
          direction >= enumValue(CreativeWorldLayoutVerticalDirection::Count) ||
          !reader.readHex(value.stableKey) || !reader.readHex(value.name) ||
          !readRect(reader, value.footprint) ||
          value.footprint.minimum.x >= value.footprint.maximum.x ||
          value.footprint.minimum.z >= value.footprint.maximum.z) {
        return false;
      }
      value.kind = static_cast<CreativeWorldLayoutVerticalConnectorKind>(kind);
      value.direction =
          static_cast<CreativeWorldLayoutVerticalDirection>(direction);
      return true;
    };
    if (!readTable(lines, lineIndex, connectorCount,
                   result.layout.verticalConnectors, readConnector, result)) {
      return result;
    }
  }
  const auto readBox = [codecVersion](RecordReader& reader,
                                      CreativeWorldLayoutBox& box) {
    std::uint16_t kind = 0U;
    std::int32_t legacyBaseLayer = 0;
    const bool prefix =
        reader.readLiteral("X") && reader.readSize(box.buildingIndex) &&
        reader.readUnsigned(kind) &&
        kind < enumValue(CreativeObjectKind::Count) &&
        reader.readHex(box.stableKey) && reader.readHex(box.name) &&
        readRect(reader, box.footprint);
    const bool anchor = codecVersion >= 5U
                            ? reader.readDouble(box.anchorLayer)
                            : reader.readI32(legacyBaseLayer);
    if (!prefix || !anchor || !reader.readUnsigned(box.layerCount)) {
      return false;
    }
    if (codecVersion < 5U) {
      box.anchorLayer = static_cast<double>(legacyBaseLayer);
    }
    if (!std::isfinite(box.anchorLayer) || box.layerCount == 0U) {
      return false;
    }
    box.kind = static_cast<CreativeObjectKind>(kind);
    return true;
  };
  if (!readTable(lines, lineIndex, boxCount, result.layout.boxes, readBox,
                 result)) {
    return result;
  }
  const auto readWall = [](RecordReader& reader,
                           CreativeWorldLayoutWall& wall) {
    return reader.readLiteral("W") && reader.readSize(wall.buildingIndex) &&
           reader.readHex(wall.stableKey) && reader.readHex(wall.name) &&
           reader.readI32(wall.start.x) && reader.readI32(wall.start.z) &&
           reader.readI32(wall.end.x) && reader.readI32(wall.end.z) &&
           reader.readDouble(wall.baseLayer) &&
           std::isfinite(wall.baseLayer) &&
           reader.readUnsigned(wall.heightCells) &&
           reader.readDouble(wall.thicknessCells) &&
           std::isfinite(wall.thicknessCells);
  };
  if (!readTable(lines, lineIndex, wallCount, result.layout.walls, readWall,
                 result)) {
    return result;
  }
  const auto readOpening = [codecVersion](
                               RecordReader& reader,
                               CreativeWorldLayoutOpening& opening) {
    std::uint8_t hostKind = 0U;
    std::uint8_t roomEdge = 0U;
    std::uint8_t kind = 0U;
    std::uint8_t pose = 0U;
    const bool hostParsed =
        codecVersion == 1U
            ? reader.readLiteral("O") && reader.readSize(opening.wallIndex)
            : reader.readLiteral("O") && reader.readUnsigned(hostKind) &&
                  hostKind <
                      enumValue(CreativeWorldLayoutOpeningHostKind::Count) &&
                  reader.readSize(opening.wallIndex) &&
                  reader.readSize(opening.roomIndex) &&
                  reader.readUnsigned(roomEdge) &&
                  roomEdge < enumValue(CreativeWorldLayoutRoomEdge::Count);
    bool parsed = hostParsed &&
        reader.readUnsigned(kind) &&
        kind <= enumValue(CreativeBuildingOpeningKind::Window) &&
        reader.readUnsigned(pose) &&
        pose <=
            enumValue(CreativeBuildingOpeningPose::OpenFromEndPositiveNormal) &&
        reader.readHex(opening.stableKey) && reader.readHex(opening.name) &&
        reader.readDouble(opening.centerOffsetCells) &&
        reader.readDouble(opening.widthCells) &&
        reader.readDouble(opening.cutoutBottomCells) &&
        reader.readDouble(opening.cutoutHeightCells) &&
        reader.readBool(opening.includeInsert) &&
        reader.readDouble(opening.insertBottomCells) &&
        reader.readDouble(opening.insertHeightCells) &&
        reader.readDouble(opening.insertWidthCells) &&
        reader.readDouble(opening.insertThicknessCells);
    if (parsed && codecVersion >= 10U) {
      parsed = reader.readHex(opening.insertAssetId) &&
               reader.readBool(opening.hasInsertAssetSourceBounds) &&
               reader.readDouble(
                   opening.insertAssetSourceBoundsMeters.min.x) &&
               reader.readDouble(
                   opening.insertAssetSourceBoundsMeters.min.y) &&
               reader.readDouble(
                   opening.insertAssetSourceBoundsMeters.min.z) &&
               reader.readDouble(
                   opening.insertAssetSourceBoundsMeters.max.x) &&
               reader.readDouble(
                   opening.insertAssetSourceBoundsMeters.max.y) &&
               reader.readDouble(
                   opening.insertAssetSourceBoundsMeters.max.z);
    } else if (codecVersion < 10U) {
      opening.insertAssetId.clear();
      opening.insertAssetSourceBoundsMeters = {};
      opening.hasInsertAssetSourceBounds = false;
    }
    const CreativeBoundsMetrics assetSource =
        measureCreativeBounds(opening.insertAssetSourceBoundsMeters);
    const bool validAsset =
        opening.hasInsertAssetSourceBounds
            ? !opening.insertAssetId.empty() && assetSource.valid &&
                  isPositiveCreativeVec3(assetSource.size)
            : opening.insertAssetId.empty();
    if (!parsed || !std::isfinite(opening.centerOffsetCells) ||
        !std::isfinite(opening.widthCells) ||
        !std::isfinite(opening.cutoutBottomCells) ||
        !std::isfinite(opening.cutoutHeightCells) ||
        !std::isfinite(opening.insertBottomCells) ||
        !std::isfinite(opening.insertHeightCells) ||
        !std::isfinite(opening.insertWidthCells) ||
        !std::isfinite(opening.insertThicknessCells) ||
        !std::isfinite(opening.insertAssetSourceBoundsMeters.min.x) ||
        !std::isfinite(opening.insertAssetSourceBoundsMeters.min.y) ||
        !std::isfinite(opening.insertAssetSourceBoundsMeters.min.z) ||
        !std::isfinite(opening.insertAssetSourceBoundsMeters.max.x) ||
        !std::isfinite(opening.insertAssetSourceBoundsMeters.max.y) ||
        !std::isfinite(opening.insertAssetSourceBoundsMeters.max.z) ||
        !validAsset) {
      return false;
    }
    opening.hostKind =
        static_cast<CreativeWorldLayoutOpeningHostKind>(hostKind);
    opening.roomEdge = static_cast<CreativeWorldLayoutRoomEdge>(roomEdge);
    opening.kind = static_cast<CreativeBuildingOpeningKind>(kind);
    opening.pose = static_cast<CreativeBuildingOpeningPose>(pose);
    return true;
  };
  if (!readTable(lines, lineIndex, openingCount, result.layout.openings,
                 readOpening, result)) {
    return result;
  }
  const auto readObject = [&, codecVersion](RecordReader& reader,
                                            CreativeWorldLayoutObject& object) {
    std::uint16_t kind = 0U;
    std::uint8_t mode = 0U;
    std::size_t tagCount = 0U;
    bool parsed =
        reader.readLiteral("Y") && reader.readUnsigned(kind) &&
        kind > enumValue(CreativeObjectKind::Unknown) &&
        kind < enumValue(CreativeObjectKind::Count) &&
        reader.readUnsigned(mode) &&
        mode < enumValue(CreativeObjectLibraryPlacementMode::Count) &&
        reader.readHex(object.stableKey) && reader.readHex(object.name) &&
        reader.readHex(object.assetId) && reader.readBool(object.visible) &&
        reader.readDouble(object.boundsCells.min.x) &&
        reader.readDouble(object.boundsCells.min.y) &&
        reader.readDouble(object.boundsCells.min.z) &&
        reader.readDouble(object.boundsCells.max.x) &&
        reader.readDouble(object.boundsCells.max.y) &&
        reader.readDouble(object.boundsCells.max.z) &&
        reader.readDouble(object.pointCells.x) &&
        reader.readDouble(object.pointCells.y) &&
        reader.readDouble(object.pointCells.z);
    if (parsed && codecVersion >= 9U) {
      parsed = reader.readBool(object.hasAssetSourceBounds) &&
               reader.readDouble(object.assetSourceBoundsMeters.min.x) &&
               reader.readDouble(object.assetSourceBoundsMeters.min.y) &&
               reader.readDouble(object.assetSourceBoundsMeters.min.z) &&
               reader.readDouble(object.assetSourceBoundsMeters.max.x) &&
               reader.readDouble(object.assetSourceBoundsMeters.max.y) &&
               reader.readDouble(object.assetSourceBoundsMeters.max.z) &&
               reader.readDouble(object.yawRadians) &&
               reader.readDouble(object.scale.x) &&
               reader.readDouble(object.scale.y) &&
               reader.readDouble(object.scale.z);
    } else if (codecVersion < 9U) {
      object.assetSourceBoundsMeters = {};
      object.hasAssetSourceBounds = false;
      object.yawRadians = 0.0;
      object.scale = {1.0, 1.0, 1.0};
    }
    parsed = parsed && reader.readSize(tagCount) &&
             tagCount <= kCreativeWorldLayoutCodecMaxRecords - totalTagCount;
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
    const auto placementMode =
        static_cast<CreativeObjectLibraryPlacementMode>(mode);
    const bool validScale = object.scale.x > 0.0 && object.scale.y > 0.0 &&
                            object.scale.z > 0.0;
    const bool validAssetBounds =
        !object.hasAssetSourceBounds ||
        (placementMode == CreativeObjectLibraryPlacementMode::Point &&
         object.assetSourceBoundsMeters.max.x >
             object.assetSourceBoundsMeters.min.x &&
         object.assetSourceBoundsMeters.max.y >
             object.assetSourceBoundsMeters.min.y &&
         object.assetSourceBoundsMeters.max.z >
             object.assetSourceBoundsMeters.min.z);
    const bool validBoundsModePose =
        placementMode != CreativeObjectLibraryPlacementMode::Bounds ||
        (!object.hasAssetSourceBounds && object.yawRadians == 0.0 &&
         object.scale.x == 1.0 && object.scale.y == 1.0 &&
         object.scale.z == 1.0);
    if (!parsed || !finite || !validScale || !validAssetBounds ||
        !validBoundsModePose) {
      return false;
    }
    object.kind = static_cast<CreativeObjectKind>(kind);
    object.mode = placementMode;
    totalTagCount += tagCount;
    object.tags.resize(tagCount);
    for (std::string& tag : object.tags) {
      if (!reader.readHex(tag)) {
        return false;
      }
    }
    return true;
  };
  if (!readTable(lines, lineIndex, objectCount, result.layout.objects,
                 readObject, result)) {
    return result;
  }
  const auto readProfile = [](RecordReader& reader,
                              CreativeWorldLayoutTerrainProfile& profile) {
    std::uint8_t kind = 0U;
    std::uint8_t blend = 0U;
    std::uint8_t rodPolicy = 0U;
    std::uint8_t direction = 0U;
    if (!reader.readLiteral("P") || !reader.readHex(profile.stableKey) ||
        !reader.readUnsigned(kind) ||
        kind >= enumValue(CreativeTerrainRecipeKind::Count) ||
        !reader.readI32(profile.center.x) ||
        !reader.readI32(profile.center.z) ||
        !reader.readUnsigned(profile.baseHeightCells) ||
        !reader.readUnsigned(profile.radiusCells) ||
        !reader.readUnsigned(profile.amplitudeCells) ||
        !reader.readUnsigned(profile.spacingCells) ||
        !reader.readUnsigned(blend) ||
        blend >= enumValue(CreativeTerrainProfileBlend::Count) ||
        !reader.readUnsigned(rodPolicy) ||
        rodPolicy >= enumValue(CreativeTerrainProfileRodPolicy::Count) ||
        !reader.readUnsigned(direction) ||
        direction >= enumValue(CreativeTerrainProfileDirection::Count) ||
        !reader.readUnsigned(profile.frequency)) {
      return false;
    }
    profile.kind = static_cast<CreativeTerrainRecipeKind>(kind);
    profile.blend = static_cast<CreativeTerrainProfileBlend>(blend);
    profile.rodPolicy = static_cast<CreativeTerrainProfileRodPolicy>(rodPolicy);
    profile.direction = static_cast<CreativeTerrainProfileDirection>(direction);
    return true;
  };
  if (!readTable(lines, lineIndex, profileCount, result.layout.terrainProfiles,
                 readProfile, result)) {
    return result;
  }
  const auto readPath = [](RecordReader& reader,
                           CreativeWorldLayoutTerrainPath& path) {
    std::uint8_t kind = 0U;
    std::uint8_t elevation = 0U;
    std::uint8_t material = 0U;
    if (!reader.readLiteral("T") || !reader.readHex(path.stableKey) ||
        !reader.readUnsigned(kind) ||
        kind >= enumValue(CreativeTerrainRecipeKind::Count) ||
        !reader.readSize(path.firstPointIndex) ||
        !reader.readSize(path.pointCount) || !reader.readUnsigned(elevation) ||
        elevation >= enumValue(CreativeTerrainPathElevation::Count) ||
        !reader.readUnsigned(path.halfWidthCells) ||
        !reader.readUnsigned(path.amplitudeCells) ||
        !reader.readBool(path.paintSurface) || !reader.readUnsigned(material) ||
        material > enumValue(CreativeTerrainMaterial::Count)) {
      return false;
    }
    path.kind = static_cast<CreativeTerrainRecipeKind>(kind);
    path.elevation = static_cast<CreativeTerrainPathElevation>(elevation);
    path.material = static_cast<CreativeTerrainMaterial>(material);
    return true;
  };
  if (!readTable(lines, lineIndex, pathCount, result.layout.terrainPaths,
                 readPath, result)) {
    return result;
  }
  const auto readPoint = [](RecordReader& reader,
                            CreativeTerrainPathPoint& point) {
    return reader.readLiteral("Q") && reader.readI32(point.coord.x) &&
           reader.readI32(point.coord.z) &&
           reader.readUnsigned(point.heightCells);
  };
  if (!readTable(lines, lineIndex, pointCount, result.layout.terrainPathPoints,
                 readPoint, result)) {
    return result;
  }

  if (lineIndex >= lines.size() || lines[lineIndex] != "END") {
    result.status = CreativeWorldLayoutCodecStatus::TrailingData;
    result.failedLine = lineIndex + 1U;
    result.reasonCode = "creative_world_layout_decode_missing_end";
    return result;
  }
  ++lineIndex;
  if (lineIndex != lines.size()) {
    result.status = CreativeWorldLayoutCodecStatus::TrailingData;
    result.failedLine = lineIndex + 1U;
    result.reasonCode = "creative_world_layout_decode_trailing_data";
    return result;
  }

  // Versions 1-5 stored vertical geometry on every room. Normalize equal
  // building/elevation tuples into one level before callers observe the data.
  if (codecVersion < 6U) {
    std::uint64_t nextLevelOrdinal = 1U;
    for (std::size_t roomIndex = 0U;
         roomIndex < result.layout.rooms.size(); ++roomIndex) {
      CreativeWorldLayoutRoom& room = result.layout.rooms[roomIndex];
      const LegacyRoomGeometry geometry = legacyRoomGeometry[roomIndex];
      std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
      for (std::size_t index = 0U; index < result.layout.levels.size();
           ++index) {
        const CreativeWorldLayoutLevel& level = result.layout.levels[index];
        if (level.buildingIndex == room.buildingIndex &&
            std::fabs(level.floorTopLayer - geometry.floorTopLayer) <=
                1.0e-9 &&
            level.wallHeightCells == geometry.wallHeightCells &&
            level.floorThicknessLayers == geometry.floorThicknessLayers) {
          levelIndex = index;
          break;
        }
      }
      if (levelIndex == kInvalidCreativeWorldLayoutIndex) {
        CreativeWorldLayoutLevel level;
        level.buildingIndex = room.buildingIndex;
        level.stableKey = mintCreativeWorldLayoutStableKey(
            result.layout, nextLevelOrdinal, "level_migrated");
        std::size_t buildingLevelOrdinal = 1U;
        for (const CreativeWorldLayoutLevel& existing : result.layout.levels) {
          buildingLevelOrdinal +=
              existing.buildingIndex == room.buildingIndex ? 1U : 0U;
        }
        level.name = "Level " + std::to_string(buildingLevelOrdinal);
        level.floorTopLayer = geometry.floorTopLayer;
        level.wallHeightCells = geometry.wallHeightCells;
        level.floorThicknessLayers = geometry.floorThicknessLayers;
        levelIndex = result.layout.levels.size();
        result.layout.levels.push_back(std::move(level));
      }
      room.levelIndex = levelIndex;
    }
  }

  // Version 1 had no room table and every opening referenced an explicit wall.
  // Every older source now leaves the codec as current in-memory truth.
  result.layout.schemaVersion = kCreativeWorldLayoutSchemaVersion;

  result.accepted = true;
  result.status = CreativeWorldLayoutCodecStatus::Ready;
  result.reasonCode = "creative_world_layout_decoded";
  return result;
}

}  // namespace iggy3d::creative
