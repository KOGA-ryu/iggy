#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kHeader = "IGGY3D_WORLD_LAYOUT";

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

[[nodiscard]] bool legacyPoseUsesStart(
    CreativeBuildingOpeningPose pose) noexcept {
  return pose ==
             CreativeBuildingOpeningPose::OpenFromStartNegativeNormal ||
         pose ==
             CreativeBuildingOpeningPose::OpenFromStartPositiveNormal;
}

[[nodiscard]] CreativeDoorSettings migrateLegacyDoorSettings(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutOpening& opening,
    CreativeBuildingOpeningPose pose) noexcept {
  CreativeDoorSettings settings;
  if (opening.kind != CreativeBuildingOpeningKind::Door ||
      pose == CreativeBuildingOpeningPose::Closed) {
    return settings;
  }
  settings.initialState = CreativeDoorInitialState::Open;
  settings.swingSide =
      pose == CreativeBuildingOpeningPose::OpenFromStartNegativeNormal ||
              pose == CreativeBuildingOpeningPose::OpenFromEndNegativeNormal
          ? CreativeDoorSwingSide::NegativeNormal
          : CreativeDoorSwingSide::PositiveNormal;

  const CreativeWorldLayoutOpeningHostFrame host =
      resolveCreativeWorldLayoutOpeningHost(layout, opening);
  if (!host.accepted) {
    settings.hingeSide = legacyPoseUsesStart(pose)
                             ? CreativeDoorHingeSide::MinimumEdge
                             : CreativeDoorHingeSide::MaximumEdge;
    return settings;
  }
  const bool alongX = host.start.z == host.end.z;
  const std::int32_t start = alongX ? host.start.x : host.start.z;
  const std::int32_t end = alongX ? host.end.x : host.end.z;
  const bool startIsMinimum = start < end;
  settings.hingeSide = legacyPoseUsesStart(pose) == startIsMinimum
                           ? CreativeDoorHingeSide::MinimumEdge
                           : CreativeDoorHingeSide::MaximumEdge;
  return settings;
}

struct LegacyTerrainPathRecord {
  CreativeTerrainRecipeKind kind = CreativeTerrainRecipeKind::Road;
  std::size_t firstPointIndex = 0U;
  std::size_t pointCount = 0U;
  CreativeTerrainPathElevation elevation =
      CreativeTerrainPathElevation::Follow;
  std::uint16_t halfWidthCells = 1U;
  std::uint16_t amplitudeCells = 1U;
  bool paintSurface = true;
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Count;
};

[[nodiscard]] bool migrateLegacyTerrainPath(
    const LegacyTerrainPathRecord& legacy,
    std::span<const CreativeTerrainPathPoint> points,
    CreativeTerrainPathSourceRecipe& output) {
  if (legacy.firstPointIndex > points.size() || legacy.pointCount < 2U ||
      legacy.pointCount > kCreativeTerrainPathPointCapacity ||
      legacy.pointCount > points.size() - legacy.firstPointIndex) {
    return false;
  }
  output = {};
  switch (legacy.kind) {
    case CreativeTerrainRecipeKind::Road:
      output.kind = CreativeTerrainPathKind::Road;
      output.crossSection = CreativeTerrainPathCrossSection::Flat;
      break;
    case CreativeTerrainRecipeKind::River:
      output.kind = CreativeTerrainPathKind::River;
      output.crossSection = CreativeTerrainPathCrossSection::Channel;
      break;
    case CreativeTerrainRecipeKind::Ditch:
      output.kind = CreativeTerrainPathKind::Trench;
      output.crossSection = CreativeTerrainPathCrossSection::Cut;
      break;
    case CreativeTerrainRecipeKind::RidgeLine:
      output.kind = CreativeTerrainPathKind::Ridge;
      output.crossSection = CreativeTerrainPathCrossSection::Berm;
      break;
    default:
      return false;
  }
  output.elevation = legacy.elevation;
  output.curve = CreativeTerrainPathCurvePolicy::Linear;
  output.startJoin = CreativeTerrainPathEndpointJoin::Open;
  output.endJoin = CreativeTerrainPathEndpointJoin::Open;
  output.falloffCells = 2U;
  output.paintSurface = legacy.paintSurface;
  output.material = legacy.material;
  output.points.reserve(legacy.pointCount);
  for (std::size_t index = 0U; index < legacy.pointCount; ++index) {
    const CreativeTerrainPathPoint& point =
        points[legacy.firstPointIndex + index];
    output.points.push_back(
        {static_cast<CreativeTerrainPathSourcePointId>(index + 1U),
         point.coord,
         point.heightCells,
         legacy.halfWidthCells,
         legacy.amplitudeCells,
         0});
  }
  output.nextPointId = static_cast<CreativeTerrainPathSourcePointId>(
      output.points.size() + 1U);
  return isValidCreativeTerrainPathSourceRecipe(output);
}

}  // namespace

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
  std::size_t topologyVertexCount = 0U;
  std::size_t topologyEdgeCount = 0U;
  std::size_t roomBoundaryCount = 0U;
  std::size_t connectorCount = 0U;
  std::size_t boxCount = 0U;
  std::size_t wallCount = 0U;
  std::size_t openingCount = 0U;
  std::size_t objectCount = 0U;
  std::size_t profileCount = 0U;
  std::size_t pathCount = 0U;
  std::size_t pointCount = 0U;
  std::size_t crossingCount = 0U;
  std::size_t roofApertureCount = 0U;
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
  const bool topologyCountInvalid =
      codecVersion >= 12U &&
      (!layoutRecord.readSize(topologyVertexCount) ||
       !layoutRecord.readSize(topologyEdgeCount) ||
       !layoutRecord.readSize(roomBoundaryCount));
  const bool connectorCountInvalid =
      codecVersion >= 7U && !layoutRecord.readSize(connectorCount);
  const bool structuralCountInvalid =
      !layoutRecord.readSize(boxCount) || !layoutRecord.readSize(wallCount) ||
      !layoutRecord.readSize(openingCount);
  const bool objectCountInvalid =
      codecVersion >= 3U && !layoutRecord.readSize(objectCount);
  const bool terrainCountInvalid =
      !layoutRecord.readSize(profileCount) ||
      !layoutRecord.readSize(pathCount) ||
      !layoutRecord.readSize(pointCount);
  const bool crossingCountInvalid =
      codecVersion >= 24U && !layoutRecord.readSize(crossingCount);
  const bool roofApertureCountInvalid =
      codecVersion >= 20U && !layoutRecord.readSize(roofApertureCount);
  if (layoutPrefix || levelCountInvalid || roomCountInvalid ||
      topologyCountInvalid ||
      connectorCountInvalid || structuralCountInvalid || objectCountInvalid ||
      terrainCountInvalid || crossingCountInvalid || roofApertureCountInvalid ||
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
      buildingCount,       levelCount,        roomCount,
      topologyVertexCount, topologyEdgeCount, roomBoundaryCount,
      connectorCount,      boxCount,          wallCount,
      openingCount,        objectCount,       profileCount,
      pathCount,           pointCount,        crossingCount,
      roofApertureCount,
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
    std::uint8_t roofSlopeDirection = static_cast<std::uint8_t>(
        CreativeStructuralRoofSlopeDirection::PositiveZ);
    std::uint8_t roofMaterial =
        static_cast<std::uint8_t>(CreativeStructuralMaterial::Blockout);
    if (!reader.readUnsigned(roofStyle) ||
        !reader.readUnsigned(roofRidgeAxis) ||
        (codecVersion >= 19U &&
         !reader.readUnsigned(roofSlopeDirection)) ||
        !reader.readDouble(level.roofPitchDegrees) ||
        !reader.readDouble(level.roofOverhangCells) ||
        (codecVersion >= 19U && !reader.readUnsigned(roofMaterial))) {
      return false;
    }
    level.roofStyle = static_cast<CreativeStructuralRoofStyle>(roofStyle);
    level.roofRidgeAxis =
        static_cast<CreativeStructuralRoofRidgeAxis>(roofRidgeAxis);
    level.roofSlopeDirection =
        static_cast<CreativeStructuralRoofSlopeDirection>(roofSlopeDirection);
    level.roofMaterial = static_cast<CreativeStructuralMaterial>(roofMaterial);
    return validCreativeStructuralRoofSettings(
               level.roofStyle, level.roofRidgeAxis,
               level.roofSlopeDirection, level.roofPitchDegrees,
               level.roofOverhangCells, level.roofMaterial) &&
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
    const auto readRoom = [codecVersion](RecordReader& reader,
                                         CreativeWorldLayoutRoom& room) {
      std::uint8_t type = 0U;
      const bool base =
          reader.readLiteral("R") && reader.readSize(room.buildingIndex) &&
          reader.readSize(room.levelIndex) && reader.readHex(room.stableKey) &&
          reader.readHex(room.name) && readRect(reader, room.footprint) &&
          reader.readDouble(room.wallThicknessCells) &&
          std::isfinite(room.wallThicknessCells);
      const bool typed = codecVersion < 13U ||
                         (reader.readUnsigned(type) &&
                          type < static_cast<std::uint8_t>(
                                     CreativeWorldLayoutRoomType::Count));
      if (base && typed) {
        room.type = static_cast<CreativeWorldLayoutRoomType>(type);
      }
      return base && typed;
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
  if (codecVersion >= 12U) {
    const auto readTopologyVertex = [](
                                        RecordReader& reader,
                                        CreativeWorldLayoutTopologyVertex& value) {
      return reader.readLiteral("N") && reader.readSize(value.levelIndex) &&
             reader.readHex(value.stableKey) &&
             reader.readI32(value.position.x) &&
             reader.readI32(value.position.z);
    };
    if (!readTable(lines, lineIndex, topologyVertexCount,
                   result.layout.topologyVertices, readTopologyVertex,
                   result)) {
      return result;
    }
    const auto readTopologyEdge = [codecVersion](
                                      RecordReader& reader,
                                      CreativeWorldLayoutTopologyEdge& value) {
      std::uint8_t profile = 0U;
      std::uint8_t material = 0U;
      std::uint8_t joinStyle = 0U;
      const bool base =
          reader.readLiteral("E") && reader.readSize(value.levelIndex) &&
          reader.readHex(value.stableKey) &&
          reader.readSize(value.startVertexIndex) &&
          reader.readSize(value.endVertexIndex) &&
          reader.readDouble(value.wallThicknessCells) &&
          std::isfinite(value.wallThicknessCells);
      const bool attributes =
          codecVersion < 14U ||
          (reader.readUnsigned(value.wallHeightCells) &&
           reader.readUnsigned(profile) &&
           profile < enumValue(CreativeWorldLayoutWallProfile::Count) &&
           reader.readUnsigned(material) &&
           material < enumValue(CreativeStructuralMaterial::Count) &&
           reader.readUnsigned(joinStyle) &&
           joinStyle < enumValue(CreativeWorldLayoutWallJoinStyle::Count));
      if (base && attributes && codecVersion >= 14U) {
        value.profile = static_cast<CreativeWorldLayoutWallProfile>(profile);
        value.material = static_cast<CreativeStructuralMaterial>(material);
        value.joinStyle =
            static_cast<CreativeWorldLayoutWallJoinStyle>(joinStyle);
      }
      return base && attributes;
    };
    if (!readTable(lines, lineIndex, topologyEdgeCount,
                   result.layout.topologyEdges, readTopologyEdge, result)) {
      return result;
    }
    const auto readRoomBoundary = [](
                                      RecordReader& reader,
                                      CreativeWorldLayoutRoomBoundary& value) {
      return reader.readLiteral("U") && reader.readSize(value.roomIndex) &&
             reader.readSize(value.topologyEdgeIndex) &&
             reader.readSize(value.order) && reader.readBool(value.reversed);
    };
    if (!readTable(lines, lineIndex, roomBoundaryCount,
                   result.layout.roomBoundaries, readRoomBoundary, result)) {
      return result;
    }
  }
  if (codecVersion >= 7U) {
    const auto readConnector = [codecVersion](
                                   RecordReader& reader,
                                   CreativeWorldLayoutVerticalConnector& value) {
      std::uint8_t kind = 0U;
      std::uint8_t direction = 0U;
      std::uint8_t material = 0U;
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
          value.footprint.minimum.z >= value.footprint.maximum.z ||
          (codecVersion >= 18U &&
           (!reader.readUnsigned(material) ||
            material >= enumValue(CreativeStructuralMaterial::Count)))) {
        return false;
      }
      value.kind = static_cast<CreativeWorldLayoutVerticalConnectorKind>(kind);
      value.direction =
          static_cast<CreativeWorldLayoutVerticalDirection>(direction);
      if (codecVersion >= 18U) {
        value.material = static_cast<CreativeStructuralMaterial>(material);
      }
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
  const auto readWall = [codecVersion](RecordReader& reader,
                           CreativeWorldLayoutWall& wall) {
    std::uint8_t profile = 0U;
    std::uint8_t material = 0U;
    std::uint8_t joinStyle = 0U;
    const bool base =
        reader.readLiteral("W") && reader.readSize(wall.buildingIndex) &&
        reader.readHex(wall.stableKey) && reader.readHex(wall.name) &&
        reader.readI32(wall.start.x) && reader.readI32(wall.start.z) &&
        reader.readI32(wall.end.x) && reader.readI32(wall.end.z) &&
        reader.readDouble(wall.baseLayer) && std::isfinite(wall.baseLayer) &&
        reader.readUnsigned(wall.heightCells) &&
        reader.readDouble(wall.thicknessCells) &&
        std::isfinite(wall.thicknessCells);
    const bool attributes =
        codecVersion < 14U ||
        (reader.readUnsigned(profile) &&
         profile < enumValue(CreativeWorldLayoutWallProfile::Count) &&
         reader.readUnsigned(material) &&
         material < enumValue(CreativeStructuralMaterial::Count) &&
         reader.readUnsigned(joinStyle) &&
         joinStyle < enumValue(CreativeWorldLayoutWallJoinStyle::Count));
    if (base && attributes && codecVersion >= 14U) {
      wall.profile = static_cast<CreativeWorldLayoutWallProfile>(profile);
      wall.material = static_cast<CreativeStructuralMaterial>(material);
      wall.joinStyle =
          static_cast<CreativeWorldLayoutWallJoinStyle>(joinStyle);
    }
    return base && attributes;
  };
  if (!readTable(lines, lineIndex, wallCount, result.layout.walls, readWall,
                 result)) {
    return result;
  }
  const auto readOpening = [codecVersion, topologyEdgeCount, &layout = result.layout](
                               RecordReader& reader,
                               CreativeWorldLayoutOpening& opening) {
    std::uint8_t hostKind = 0U;
    std::uint8_t roomEdge = 0U;
    std::uint8_t kind = 0U;
    std::uint8_t pose = 0U;
    std::uint8_t facing =
        enumValue(CreativeBuildingOpeningFacing::PositiveNormal);
    std::uint8_t leafArrangement =
        enumValue(CreativeDoorLeafArrangement::Single);
    std::uint8_t hingeSide = enumValue(CreativeDoorHingeSide::MinimumEdge);
    std::uint8_t swingSide =
        enumValue(CreativeDoorSwingSide::PositiveNormal);
    std::uint8_t initialState = enumValue(CreativeDoorInitialState::Closed);
    bool gameplayLocked = false;
    double transitionSeconds = CreativeDoorSettings{}.transitionSeconds;
    std::uint8_t windowInsertKind =
        enumValue(CreativeWindowInsertKind::Glazing);
    const bool hostParsed =
        codecVersion == 1U
            ? reader.readLiteral("O") && reader.readSize(opening.wallIndex)
            : reader.readLiteral("O") && reader.readUnsigned(hostKind) &&
                  hostKind <
                      enumValue(CreativeWorldLayoutOpeningHostKind::Count) &&
                  reader.readSize(opening.wallIndex) &&
                  reader.readSize(opening.roomIndex) &&
                  reader.readUnsigned(roomEdge) &&
                  (roomEdge < enumValue(CreativeWorldLayoutRoomEdge::Count) ||
                   (codecVersion >= 12U &&
                    roomEdge == enumValue(
                                    CreativeWorldLayoutRoomEdge::Count)));
    bool parsed = hostParsed && reader.readUnsigned(kind) &&
                  kind <= enumValue(CreativeBuildingOpeningKind::Window);
    if (parsed && codecVersion < 16U) {
      parsed = reader.readUnsigned(pose) &&
               pose <= enumValue(
                           CreativeBuildingOpeningPose::
                               OpenFromEndPositiveNormal) &&
               (codecVersion < 15U ||
                (reader.readUnsigned(facing) &&
                 facing < enumValue(CreativeBuildingOpeningFacing::Count)));
    } else if (parsed) {
      parsed =
          reader.readUnsigned(facing) &&
          facing < enumValue(CreativeBuildingOpeningFacing::Count) &&
          reader.readUnsigned(leafArrangement) &&
          leafArrangement < enumValue(CreativeDoorLeafArrangement::Count) &&
          reader.readUnsigned(hingeSide) &&
          hingeSide < enumValue(CreativeDoorHingeSide::Count) &&
          reader.readUnsigned(swingSide) &&
          swingSide < enumValue(CreativeDoorSwingSide::Count) &&
          reader.readUnsigned(initialState) &&
          initialState < enumValue(CreativeDoorInitialState::Count) &&
          reader.readBool(gameplayLocked) &&
          reader.readDouble(transitionSeconds);
      if (parsed && codecVersion >= 17U) {
        parsed = reader.readUnsigned(windowInsertKind) &&
                 windowInsertKind <
                     enumValue(CreativeWindowInsertKind::Count);
      }
    }
    parsed = parsed && reader.readHex(opening.stableKey) &&
        reader.readHex(opening.name) &&
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
                   opening.insertAssetSourceBoundsMeters.max.z) &&
               (codecVersion < 12U ||
                reader.readSize(opening.roomTopologyEdgeIndex));
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
    const bool hasDirectRoomHost =
        codecVersion >= 12U &&
        hostKind == enumValue(CreativeWorldLayoutOpeningHostKind::RoomEdge) &&
        opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex &&
        opening.roomTopologyEdgeIndex < topologyEdgeCount;
    if (!parsed ||
        (roomEdge == enumValue(CreativeWorldLayoutRoomEdge::Count) &&
         !hasDirectRoomHost) ||
        (opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex &&
         !hasDirectRoomHost) ||
        !std::isfinite(opening.centerOffsetCells) ||
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
    opening.facing = static_cast<CreativeBuildingOpeningFacing>(facing);
    if (codecVersion < 16U) {
      opening.door = migrateLegacyDoorSettings(
          layout, opening, static_cast<CreativeBuildingOpeningPose>(pose));
    } else {
      opening.door.leafArrangement =
          static_cast<CreativeDoorLeafArrangement>(leafArrangement);
      opening.door.hingeSide = static_cast<CreativeDoorHingeSide>(hingeSide);
      opening.door.swingSide = static_cast<CreativeDoorSwingSide>(swingSide);
      opening.door.initialState =
          static_cast<CreativeDoorInitialState>(initialState);
      opening.door.gameplayLocked = gameplayLocked;
      opening.door.transitionSeconds = transitionSeconds;
    }
    opening.window.insertKind =
        static_cast<CreativeWindowInsertKind>(windowInsertKind);
    if (opening.kind == CreativeBuildingOpeningKind::Door &&
        !isValidCreativeDoorSettings(opening.door)) {
      return false;
    }
    if (opening.kind == CreativeBuildingOpeningKind::Window &&
        !isValidCreativeWindowSettings(opening.window)) {
      return false;
    }
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
    std::uint8_t bridgeAttachment = 0U;
    std::uint8_t bridgeSupportStyle = 0U;
    std::uint8_t bridgeDeckMaterial = 0U;
    std::uint8_t bridgeSupportMaterial = 0U;
    std::uint8_t bridgeRailMaterial = 0U;
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
    if (parsed && codecVersion >= 25U) {
      parsed = reader.readBool(object.usesBridgeRecipe) &&
               reader.readUnsigned(object.bridge.version) &&
               reader.readUnsigned(bridgeAttachment) &&
               bridgeAttachment <
                   enumValue(CreativeBridgeAttachmentKind::Count) &&
               reader.readHex(object.bridge.watercoursePathKey) &&
               reader.readUnsigned(object.bridge.crossingId) &&
               reader.readDouble(object.bridge.settings.deckWidthMeters) &&
               reader.readDouble(object.bridge.settings.deckThicknessMeters) &&
               reader.readDouble(
                   object.bridge.settings.deckElevationOffsetMeters) &&
               reader.readDouble(object.bridge.settings.maximumSpanMeters) &&
               reader.readDouble(
                   object.bridge.settings.minimumClearanceMeters) &&
               reader.readUnsigned(bridgeSupportStyle) &&
               bridgeSupportStyle <
                   enumValue(CreativeBridgeSupportStyle::Count) &&
               reader.readDouble(
                   object.bridge.settings.supportSpacingMeters) &&
               reader.readDouble(object.bridge.settings.supportWidthMeters) &&
               reader.readDouble(object.bridge.settings.supportDepthMeters) &&
               reader.readBool(object.bridge.settings.rails) &&
               reader.readDouble(object.bridge.settings.railHeightMeters) &&
               reader.readDouble(object.bridge.settings.railThicknessMeters) &&
               reader.readUnsigned(
                   object.bridge.settings.maximumApproachGradePermille) &&
               reader.readUnsigned(
                   object.bridge.settings.approachFalloffCells) &&
               reader.readUnsigned(bridgeDeckMaterial) &&
               bridgeDeckMaterial < enumValue(CreativeStructuralMaterial::Count) &&
               reader.readUnsigned(bridgeSupportMaterial) &&
               bridgeSupportMaterial <
                   enumValue(CreativeStructuralMaterial::Count) &&
               reader.readUnsigned(bridgeRailMaterial) &&
               bridgeRailMaterial < enumValue(CreativeStructuralMaterial::Count);
      if (parsed) {
        object.bridge.attachment =
            static_cast<CreativeBridgeAttachmentKind>(bridgeAttachment);
        object.bridge.settings.supportStyle =
            static_cast<CreativeBridgeSupportStyle>(bridgeSupportStyle);
        object.bridge.settings.materials.deck =
            static_cast<CreativeStructuralMaterial>(bridgeDeckMaterial);
        object.bridge.settings.materials.supports =
            static_cast<CreativeStructuralMaterial>(bridgeSupportMaterial);
        object.bridge.settings.materials.rails =
            static_cast<CreativeStructuralMaterial>(bridgeRailMaterial);
      }
    } else if (codecVersion < 25U) {
      object.usesBridgeRecipe = false;
      object.bridge = {};
    }
    if (parsed && codecVersion >= 27U) {
      parsed = reader.readHex(object.playerSpawn.playerProfileId) &&
               reader.readHex(object.playerSpawn.spawnGroup) &&
               reader.readDouble(object.playerSpawn.validationRadiusMeters) &&
               reader.readUnsigned(object.playerSpawn.fallbackPriority);
    } else if (codecVersion < 27U) {
      object.playerSpawn = {};
    }
    std::uint64_t npcTeam = 0U;
    std::uint64_t npcSpawnPolicy = 0U;
    if (parsed && codecVersion >= 28U) {
      parsed = reader.readHex(object.npcSpawn.behaviorProfileId) &&
               reader.readUnsigned(npcTeam) &&
               npcTeam < enumValue(CreativeNpcTeam::Count) &&
               reader.readUnsigned(object.npcSpawn.hitPoints) &&
               reader.readDouble(object.npcSpawn.initialAlertLevel) &&
               reader.readUnsigned(npcSpawnPolicy) &&
               npcSpawnPolicy < enumValue(CreativeNpcSpawnPolicy::Count);
      if (parsed) {
        object.npcSpawn.team = static_cast<CreativeNpcTeam>(npcTeam);
        object.npcSpawn.spawnPolicy =
            static_cast<CreativeNpcSpawnPolicy>(npcSpawnPolicy);
      }
    } else if (codecVersion < 28U) {
      object.npcSpawn = {};
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
    const bool validBridgeMode =
        !object.usesBridgeRecipe ||
        (static_cast<CreativeObjectKind>(kind) == CreativeObjectKind::Bridge &&
         placementMode == CreativeObjectLibraryPlacementMode::Bounds &&
         object.assetId.empty() && !object.hasAssetSourceBounds &&
         isValidCreativeBridgeSourceRecipe(object.bridge));
    const bool validPlayerSpawn =
        static_cast<CreativeObjectKind>(kind) == CreativeObjectKind::SpawnPoint
            ? isValidCreativePlayerSpawnSettings(object.playerSpawn)
            : object.playerSpawn == CreativePlayerSpawnSettings{};
    const bool npcActor =
        static_cast<CreativeObjectKind>(kind) == CreativeObjectKind::NpcSpawn ||
        static_cast<CreativeObjectKind>(kind) == CreativeObjectKind::EnemySpawn;
    const bool validNpcSpawn =
        npcActor ? isValidCreativeNpcSpawnSettings(object.npcSpawn)
                 : object.npcSpawn == CreativeNpcSpawnSettings{};
    if (!parsed || !finite || !validScale || !validAssetBounds ||
        !validBoundsModePose || !validBridgeMode || !validPlayerSpawn ||
        !validNpcSpawn) {
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
  const auto readProfile = [codecVersion](
                               RecordReader& reader,
                               CreativeWorldLayoutTerrainProfile& profile) {
    std::uint8_t kind = 0U;
    std::uint8_t blend = 0U;
    std::uint8_t rodPolicy = 0U;
    std::uint8_t direction = 0U;
    const std::uint8_t kindLimit =
        codecVersion < 22U
            ? static_cast<std::uint8_t>(CreativeTerrainRecipeKind::Plateau) +
                  1U
            : enumValue(CreativeTerrainRecipeKind::Count);
    if (!reader.readLiteral("P") || !reader.readHex(profile.stableKey) ||
        !reader.readUnsigned(kind) ||
        kind >= kindLimit ||
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
    if (codecVersion < 22U) {
      profile.usesLandformRecipe = false;
      return true;
    }

    std::uint8_t landformKind = 0U;
    std::uint8_t landformDirection = 0U;
    std::uint8_t landformEdge = 0U;
    std::uint8_t landformMaterial = 0U;
    std::uint8_t landformErosion = 0U;
    if (!reader.readBool(profile.usesLandformRecipe) ||
        !reader.readUnsigned(profile.landform.version) ||
        !reader.readUnsigned(landformKind) ||
        landformKind >= enumValue(CreativeTerrainLandformKind::Count) ||
        !reader.readI32(profile.landform.bounds.minimum.x) ||
        !reader.readI32(profile.landform.bounds.minimum.z) ||
        !reader.readUnsigned(profile.landform.bounds.widthCells) ||
        !reader.readUnsigned(profile.landform.bounds.depthCells) ||
        !reader.readUnsigned(profile.landform.baseHeightCells) ||
        !reader.readUnsigned(profile.landform.targetHeightCells) ||
        !reader.readUnsigned(profile.landform.terraceCount) ||
        !reader.readUnsigned(landformDirection) ||
        landformDirection >=
            enumValue(CreativeTerrainLandformDirection::Count) ||
        !reader.readUnsigned(landformEdge) ||
        landformEdge >= enumValue(CreativeTerrainLandformEdge::Count) ||
        !reader.readUnsigned(profile.landform.edgeWidthCells) ||
        !reader.readUnsigned(profile.landform.featherCells) ||
        !reader.readBool(profile.landform.paintSurface) ||
        !reader.readUnsigned(landformMaterial) ||
        landformMaterial >= enumValue(CreativeTerrainMaterial::Count) ||
        !reader.readUnsigned(landformErosion) ||
        landformErosion >=
            enumValue(CreativeTerrainLandformErosion::Count) ||
        !reader.readUnsigned(profile.landform.erosionReliefCells) ||
        !reader.readUnsigned(profile.landform.seed)) {
      return false;
    }
    profile.landform.kind =
        static_cast<CreativeTerrainLandformKind>(landformKind);
    profile.landform.direction =
        static_cast<CreativeTerrainLandformDirection>(landformDirection);
    profile.landform.edge =
        static_cast<CreativeTerrainLandformEdge>(landformEdge);
    profile.landform.material =
        static_cast<CreativeTerrainMaterial>(landformMaterial);
    profile.landform.erosion =
        static_cast<CreativeTerrainLandformErosion>(landformErosion);
    CreativeTerrainLandformKind expectedKind =
        CreativeTerrainLandformKind::Count;
    const bool validLandform =
        profile.usesLandformRecipe
            ? creativeTerrainRecipeLandformKind(profile.kind,
                                                expectedKind) &&
                  expectedKind == profile.landform.kind &&
                  isValidCreativeTerrainLandformRecipe(profile.landform)
            : profile.kind != CreativeTerrainRecipeKind::Terrace &&
                  profile.kind != CreativeTerrainRecipeKind::Cliff;
    if (!validLandform || codecVersion < 26U) {
      return validLandform;
    }

    std::uint8_t selection = 0U;
    std::uint8_t kit = 0U;
    std::uint8_t material = 0U;
    std::size_t transitionCount = 0U;
    if (!reader.readBool(profile.usesRetainingEdgeRecipe) ||
        !reader.readUnsigned(profile.retainingEdge.version) ||
        !reader.readHex(profile.retainingEdge.terrainProfileKey) ||
        !reader.readUnsigned(selection) ||
        selection >= enumValue(CreativeRetainingEdgeSelection::Count) ||
        !reader.readUnsigned(kit) ||
        kit >= enumValue(CreativeRetainingEdgeKit::Count) ||
        !reader.readDouble(
            profile.retainingEdge.settings.thicknessMeters) ||
        !reader.readDouble(
            profile.retainingEdge.settings.maximumHeightMeters) ||
        !reader.readBool(profile.retainingEdge.settings.closeCorners) ||
        !reader.readBool(profile.retainingEdge.settings.capEnds) ||
        !reader.readUnsigned(material) ||
        material >= enumValue(CreativeStructuralMaterial::Count) ||
        !reader.readSize(transitionCount) ||
        transitionCount > kCreativeRetainingEdgeTransitionCapacity) {
      return false;
    }
    profile.retainingEdge.settings.selection =
        static_cast<CreativeRetainingEdgeSelection>(selection);
    profile.retainingEdge.settings.kit =
        static_cast<CreativeRetainingEdgeKit>(kit);
    profile.retainingEdge.settings.material =
        static_cast<CreativeStructuralMaterial>(material);
    profile.retainingEdge.settings.transitionCount = transitionCount;
    for (std::size_t index = 0U; index < transitionCount; ++index) {
      CreativeRetainingEdgeTransition& transition =
          profile.retainingEdge.settings.transitions[index];
      std::uint8_t transitionKind = 0U;
      if (!reader.readI32(transition.edge.first.x) ||
          !reader.readI32(transition.edge.first.z) ||
          !reader.readI32(transition.edge.second.x) ||
          !reader.readI32(transition.edge.second.z) ||
          !reader.readUnsigned(transitionKind) ||
          transitionKind >=
              enumValue(CreativeRetainingEdgeTransitionKind::Count) ||
          !reader.readUnsigned(transition.runCells)) {
        return false;
      }
      transition.kind =
          static_cast<CreativeRetainingEdgeTransitionKind>(transitionKind);
    }
    const bool validRetainingData =
        profile.retainingEdge.version ==
            kCreativeRetainingEdgeRecipeVersion &&
        isValidCreativeRetainingEdgeSettings(
            profile.retainingEdge.settings);
    return validRetainingData &&
           (!profile.usesRetainingEdgeRecipe ||
            (profile.usesLandformRecipe &&
             profile.landform.edge ==
                 CreativeTerrainLandformEdge::Retaining &&
             profile.retainingEdge.terrainProfileKey == profile.stableKey &&
             isValidCreativeRetainingEdgeSourceRecipe(
                 profile.retainingEdge)));
  };
  if (!readTable(lines, lineIndex, profileCount, result.layout.terrainProfiles,
                 readProfile, result)) {
    return result;
  }
  std::vector<LegacyTerrainPathRecord> legacyTerrainPaths;
  std::vector<std::size_t> terrainPathPointCounts;
  std::vector<std::size_t> terrainPathCrossingCounts;
  legacyTerrainPaths.reserve(pathCount);
  terrainPathPointCounts.reserve(pathCount);
  terrainPathCrossingCounts.reserve(pathCount);
  const auto readPath = [codecVersion, &legacyTerrainPaths,
                         &terrainPathPointCounts,
                         &terrainPathCrossingCounts](
                            RecordReader& reader,
                            CreativeWorldLayoutTerrainPath& path) {
    if (!reader.readLiteral("T") || !reader.readHex(path.stableKey)) {
      return false;
    }
    if (codecVersion < 21U) {
      LegacyTerrainPathRecord legacy;
      std::uint8_t kind = 0U;
      std::uint8_t elevation = 0U;
      std::uint8_t material = 0U;
      if (!reader.readUnsigned(kind) ||
          kind >= enumValue(CreativeTerrainRecipeKind::Count) ||
          !reader.readSize(legacy.firstPointIndex) ||
          !reader.readSize(legacy.pointCount) ||
          !reader.readUnsigned(elevation) ||
          elevation >= enumValue(CreativeTerrainPathElevation::Count) ||
          !reader.readUnsigned(legacy.halfWidthCells) ||
          !reader.readUnsigned(legacy.amplitudeCells) ||
          !reader.readBool(legacy.paintSurface) ||
          !reader.readUnsigned(material) ||
          material > enumValue(CreativeTerrainMaterial::Count)) {
        return false;
      }
      legacy.kind = static_cast<CreativeTerrainRecipeKind>(kind);
      legacy.elevation =
          static_cast<CreativeTerrainPathElevation>(elevation);
      legacy.material = static_cast<CreativeTerrainMaterial>(material);
      legacyTerrainPaths.push_back(legacy);
      terrainPathPointCounts.push_back(legacy.pointCount);
      terrainPathCrossingCounts.push_back(0U);
      return true;
    }

    std::uint8_t kind = 0U;
    std::uint8_t elevation = 0U;
    std::uint8_t curve = 0U;
    std::uint8_t crossSection = 0U;
    std::uint8_t startJoin = 0U;
    std::uint8_t endJoin = 0U;
    std::uint8_t material = 0U;
    std::uint8_t roadEdgeTreatment = 0U;
    std::uint8_t roadEdgeMaterial = 0U;
    std::uint8_t drainageDirection = 0U;
    std::uint8_t surfacePolicy = 0U;
    std::size_t recipePointCount = 0U;
    std::size_t recipeCrossingCount = 0U;
    if (!reader.readUnsigned(path.recipe.version) ||
        !reader.readUnsigned(kind) ||
        kind >= enumValue(CreativeTerrainPathKind::Count) ||
        !reader.readUnsigned(elevation) ||
        elevation >= enumValue(CreativeTerrainPathElevation::Count) ||
        !reader.readUnsigned(curve) ||
        curve >= enumValue(CreativeTerrainPathCurvePolicy::Count) ||
        !reader.readUnsigned(crossSection) ||
        crossSection >= enumValue(CreativeTerrainPathCrossSection::Count) ||
        !reader.readUnsigned(startJoin) ||
        startJoin >= enumValue(CreativeTerrainPathEndpointJoin::Count) ||
        !reader.readUnsigned(endJoin) ||
        endJoin >= enumValue(CreativeTerrainPathEndpointJoin::Count) ||
        !reader.readUnsigned(path.recipe.falloffCells) ||
        !reader.readBool(path.recipe.paintSurface) ||
        !reader.readUnsigned(material) ||
        material > enumValue(CreativeTerrainMaterial::Count)) {
      return false;
    }
    if (codecVersion >= 23U &&
        (!reader.readUnsigned(path.recipe.road.shoulderWidthCells) ||
         !reader.readUnsigned(path.recipe.road.maximumGradePermille) ||
         !reader.readUnsigned(roadEdgeTreatment) ||
         roadEdgeTreatment >=
             enumValue(CreativeTerrainRoadEdgeTreatment::Count) ||
         !reader.readDouble(path.recipe.road.edgeWidthMeters) ||
         !reader.readDouble(path.recipe.road.edgeHeightMeters) ||
         !reader.readUnsigned(roadEdgeMaterial) ||
         roadEdgeMaterial >= enumValue(CreativeStructuralMaterial::Count))) {
      return false;
    }
    if (codecVersion >= 24U &&
        (!reader.readUnsigned(path.recipe.watercourse.bankSlopeCells) ||
         !reader.readUnsigned(drainageDirection) ||
         drainageDirection >=
             enumValue(CreativeTerrainWatercourseDrainageDirection::Count) ||
         !reader.readUnsigned(surfacePolicy) ||
         surfacePolicy >= enumValue(CreativeTerrainWaterSurfacePolicy::Count) ||
         !reader.readUnsigned(path.recipe.watercourse.surfaceInsetCells) ||
         !reader.readUnsigned(path.recipe.watercourse.nextCrossingId) ||
         !reader.readSize(recipeCrossingCount) ||
         recipeCrossingCount >
             kCreativeTerrainWatercourseCrossingCapacity)) {
      return false;
    }
    if (!reader.readUnsigned(path.recipe.nextPointId) ||
        !reader.readSize(recipePointCount) || recipePointCount < 2U ||
        recipePointCount > kCreativeTerrainPathPointCapacity) {
      return false;
    }
    if (codecVersion < 24U) {
      const std::uint32_t expectedPathVersion =
          codecVersion < 23U ? 1U : 2U;
      if (path.recipe.version != expectedPathVersion) {
        return false;
      }
      path.recipe.version = kCreativeTerrainPathSourceVersion;
    }
    if (codecVersion >= 23U) {
      path.recipe.road.edgeTreatment =
          static_cast<CreativeTerrainRoadEdgeTreatment>(roadEdgeTreatment);
      path.recipe.road.edgeMaterial =
          static_cast<CreativeStructuralMaterial>(roadEdgeMaterial);
    }
    if (codecVersion >= 24U) {
      path.recipe.watercourse.drainageDirection =
          static_cast<CreativeTerrainWatercourseDrainageDirection>(
              drainageDirection);
      path.recipe.watercourse.surfacePolicy =
          static_cast<CreativeTerrainWaterSurfacePolicy>(surfacePolicy);
    }
    path.recipe.kind = static_cast<CreativeTerrainPathKind>(kind);
    path.recipe.elevation =
        static_cast<CreativeTerrainPathElevation>(elevation);
    path.recipe.curve = static_cast<CreativeTerrainPathCurvePolicy>(curve);
    path.recipe.crossSection =
        static_cast<CreativeTerrainPathCrossSection>(crossSection);
    path.recipe.startJoin =
        static_cast<CreativeTerrainPathEndpointJoin>(startJoin);
    path.recipe.endJoin =
        static_cast<CreativeTerrainPathEndpointJoin>(endJoin);
    path.recipe.material = static_cast<CreativeTerrainMaterial>(material);
    terrainPathPointCounts.push_back(recipePointCount);
    terrainPathCrossingCounts.push_back(recipeCrossingCount);
    return true;
  };
  if (!readTable(lines, lineIndex, pathCount, result.layout.terrainPaths,
                 readPath, result)) {
    return result;
  }
  if (codecVersion < 21U) {
    std::vector<CreativeTerrainPathPoint> legacyPoints;
    const auto readLegacyPoint = [](RecordReader& reader,
                                    CreativeTerrainPathPoint& point) {
      return reader.readLiteral("Q") && reader.readI32(point.coord.x) &&
             reader.readI32(point.coord.z) &&
             reader.readUnsigned(point.heightCells);
    };
    if (!readTable(lines, lineIndex, pointCount, legacyPoints,
                   readLegacyPoint, result)) {
      return result;
    }
    if (legacyTerrainPaths.size() != result.layout.terrainPaths.size()) {
      result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
      result.failedLine = lineIndex;
      result.reasonCode = "creative_world_layout_decode_path_shape_invalid";
      return result;
    }
    for (std::size_t index = 0U; index < result.layout.terrainPaths.size();
         ++index) {
      if (!migrateLegacyTerrainPath(legacyTerrainPaths[index], legacyPoints,
                                    result.layout.terrainPaths[index].recipe)) {
        result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
        result.failedLine = lineIndex;
        result.reasonCode = "creative_world_layout_decode_path_migration_failed";
        return result;
      }
    }
  } else {
    std::vector<CreativeTerrainPathSourcePoint> decodedPoints;
    const auto readPoint = [](RecordReader& reader,
                              CreativeTerrainPathSourcePoint& point) {
      return reader.readLiteral("Q") && reader.readUnsigned(point.id) &&
             reader.readI32(point.coord.x) && reader.readI32(point.coord.z) &&
             reader.readUnsigned(point.heightCells) &&
             reader.readUnsigned(point.halfWidthCells) &&
             reader.readUnsigned(point.amplitudeCells) &&
             reader.readI32(point.bankPermille);
    };
    if (!readTable(lines, lineIndex, pointCount, decodedPoints, readPoint,
                   result)) {
      return result;
    }
    std::size_t pointOffset = 0U;
    for (std::size_t index = 0U; index < result.layout.terrainPaths.size();
         ++index) {
      const std::size_t count = terrainPathPointCounts[index];
      if (count > decodedPoints.size() - pointOffset) {
        result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
        result.failedLine = lineIndex;
        result.reasonCode = "creative_world_layout_decode_path_shape_invalid";
        return result;
      }
      CreativeTerrainPathSourceRecipe& recipe =
          result.layout.terrainPaths[index].recipe;
      recipe.points.assign(decodedPoints.begin() +
                               static_cast<std::ptrdiff_t>(pointOffset),
                           decodedPoints.begin() +
                               static_cast<std::ptrdiff_t>(pointOffset + count));
      pointOffset += count;
      if (codecVersion < 24U &&
          !isValidCreativeTerrainPathSourceRecipe(recipe)) {
        result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
        result.failedLine = lineIndex;
        result.reasonCode = "creative_world_layout_decode_path_invalid";
        return result;
      }
    }
    if (pointOffset != decodedPoints.size()) {
      result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
      result.failedLine = lineIndex;
      result.reasonCode = "creative_world_layout_decode_path_shape_invalid";
      return result;
    }
  }
  if (codecVersion >= 24U) {
    std::vector<CreativeTerrainWatercourseCrossing> decodedCrossings;
    const auto readCrossing = [](
                                  RecordReader& reader,
                                  CreativeTerrainWatercourseCrossing& crossing) {
      return reader.readLiteral("K") && reader.readUnsigned(crossing.id) &&
             reader.readUnsigned(crossing.pointId) &&
             reader.readUnsigned(crossing.bankClearanceCells) &&
             reader.readUnsigned(crossing.deckClearanceCells) &&
             reader.readUnsigned(crossing.approachLengthCells);
    };
    if (!readTable(lines, lineIndex, crossingCount, decodedCrossings,
                   readCrossing, result)) {
      return result;
    }
    if (terrainPathCrossingCounts.size() !=
        result.layout.terrainPaths.size()) {
      result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
      result.failedLine = lineIndex;
      result.reasonCode =
          "creative_world_layout_decode_crossing_shape_invalid";
      return result;
    }
    std::size_t crossingOffset = 0U;
    for (std::size_t index = 0U; index < result.layout.terrainPaths.size();
         ++index) {
      const std::size_t count = terrainPathCrossingCounts[index];
      if (count > decodedCrossings.size() - crossingOffset) {
        result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
        result.failedLine = lineIndex;
        result.reasonCode =
            "creative_world_layout_decode_crossing_shape_invalid";
        return result;
      }
      CreativeTerrainPathSourceRecipe& recipe =
          result.layout.terrainPaths[index].recipe;
      recipe.watercourse.crossings.assign(
          decodedCrossings.begin() +
              static_cast<std::ptrdiff_t>(crossingOffset),
          decodedCrossings.begin() +
              static_cast<std::ptrdiff_t>(crossingOffset + count));
      crossingOffset += count;
      if (!isValidCreativeTerrainPathSourceRecipe(recipe)) {
        result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
        result.failedLine = lineIndex;
        result.reasonCode = "creative_world_layout_decode_path_invalid";
        return result;
      }
    }
    if (crossingOffset != decodedCrossings.size()) {
      result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
      result.failedLine = lineIndex;
      result.reasonCode =
          "creative_world_layout_decode_crossing_shape_invalid";
      return result;
    }
  }
  std::vector<std::size_t> roofApertureCounts(result.layout.levels.size(), 0U);
  const auto readRoofAperture = [&result, &roofApertureCounts](
                                    RecordReader& reader,
                                    CreativeWorldLayoutRoofAperture& aperture) {
    std::uint8_t kind = 0U;
    if (!reader.readLiteral("A") ||
        !reader.readSize(aperture.levelIndex) ||
        aperture.levelIndex >= result.layout.levels.size() ||
        !reader.readUnsigned(kind) ||
        kind >= enumValue(CreativeStructuralRoofApertureKind::Count) ||
        !reader.readHex(aperture.stableKey) || aperture.stableKey.empty() ||
        !reader.readHex(aperture.name) || aperture.name.empty() ||
        !reader.readDouble(aperture.minimumXCells) ||
        !reader.readDouble(aperture.maximumXCells) ||
        !reader.readDouble(aperture.minimumZCells) ||
        !reader.readDouble(aperture.maximumZCells) ||
        !std::isfinite(aperture.minimumXCells) ||
        !std::isfinite(aperture.maximumXCells) ||
        !std::isfinite(aperture.minimumZCells) ||
        !std::isfinite(aperture.maximumZCells) ||
        aperture.minimumXCells >= aperture.maximumXCells ||
        aperture.minimumZCells >= aperture.maximumZCells ||
        ++roofApertureCounts[aperture.levelIndex] >
            kCreativeStructuralRoofApertureCapacity) {
      return false;
    }
    aperture.kind = static_cast<CreativeStructuralRoofApertureKind>(kind);
    return true;
  };
  if (!readTable(lines, lineIndex, roofApertureCount,
                 result.layout.roofApertures, readRoofAperture, result)) {
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
