#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

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
      layout.rooms.size(),
      layout.boxes.size(),
      layout.walls.size(),
      layout.openings.size(),
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
        building.tags.size() > kCreativeWorldLayoutCodecMaxRecords ||
        !std::all_of(building.tags.begin(), building.tags.end(), validString)) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
                 "creative_world_layout_encode_invalid_building"};
      return false;
    }
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (!validKeyName(room.stableKey, room.name) ||
        !validRect(room.footprint) ||
        !std::isfinite(room.wallThicknessCells)) {
      failure = {std::isfinite(room.wallThicknessCells)
                     ? CreativeWorldLayoutCodecStatus::InvalidRecord
                     : CreativeWorldLayoutCodecStatus::NonFiniteValue,
                 "creative_world_layout_encode_invalid_room"};
      return false;
    }
  }
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    if (!validKeyName(box.stableKey, box.name) || !validRect(box.footprint) ||
        enumValue(box.kind) >= enumValue(CreativeObjectKind::Count)) {
      failure = {CreativeWorldLayoutCodecStatus::InvalidRecord,
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
                        std::isfinite(opening.insertThicknessCells);
    if (!validKeyName(opening.stableKey, opening.name) || !finite ||
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
         << ' ' << layout.buildings.size() << ' ' << layout.rooms.size() << ' '
         << layout.boxes.size() << ' ' << layout.walls.size() << ' '
         << layout.openings.size() << ' '
         << layout.terrainProfiles.size() << ' ' << layout.terrainPaths.size()
         << ' ' << layout.terrainPathPoints.size() << '\n';
  for (const CreativeWorldLayoutBuilding& building : layout.buildings) {
    output << "B " << hexString(building.stableKey) << ' '
           << hexString(building.name) << ' '
           << static_cast<unsigned>(enumValue(building.rootMode));
    writeRect(output, building.rootFootprint);
    output << ' ' << building.rootBaseLayer << ' ' << building.rootHeightCells
           << ' ' << (building.visible ? 1 : 0) << ' ' << building.tags.size();
    for (const std::string& tag : building.tags) {
      output << ' ' << hexString(tag);
    }
    output << '\n';
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    output << "R " << room.buildingIndex << ' '
           << hexString(room.stableKey) << ' ' << hexString(room.name);
    writeRect(output, room.footprint);
    output << ' ' << room.baseLayer << ' ' << room.wallHeightCells << ' '
           << room.wallThicknessCells << ' ' << room.floorThicknessCells
           << '\n';
  }
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    output << "X " << box.buildingIndex << ' '
           << static_cast<unsigned>(enumValue(box.kind)) << ' '
           << hexString(box.stableKey) << ' ' << hexString(box.name);
    writeRect(output, box.footprint);
    output << ' ' << box.baseLayer << ' ' << box.heightCells << '\n';
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
           << opening.insertThicknessCells << '\n';
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
  std::size_t roomCount = 0U;
  std::size_t boxCount = 0U;
  std::size_t wallCount = 0U;
  std::size_t openingCount = 0U;
  std::size_t profileCount = 0U;
  std::size_t pathCount = 0U;
  std::size_t pointCount = 0U;
  const bool layoutPrefix =
      !layoutRecord.readLiteral("L") ||
      !layoutRecord.readUnsigned(result.layout.schemaVersion) ||
      !layoutRecord.readHex(result.layout.stableKey) ||
      !layoutRecord.readUnsigned(ownership) || ownership > 1U ||
      !layoutRecord.readSize(buildingCount);
  const bool roomCountInvalid =
      codecVersion >= 2U && !layoutRecord.readSize(roomCount);
  if (layoutPrefix || roomCountInvalid ||
      !layoutRecord.readSize(boxCount) || !layoutRecord.readSize(wallCount) ||
      !layoutRecord.readSize(openingCount) ||
      !layoutRecord.readSize(profileCount) ||
      !layoutRecord.readSize(pathCount) || !layoutRecord.readSize(pointCount) ||
      !layoutRecord.finished()) {
    result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
    result.failedLine = 2U;
    result.reasonCode = "creative_world_layout_decode_invalid_layout_record";
    return result;
  }
  const std::uint32_t expectedSchemaVersion =
      codecVersion == 1U ? 1U : kCreativeWorldLayoutSchemaVersion;
  if (result.layout.schemaVersion != expectedSchemaVersion) {
    result.status = CreativeWorldLayoutCodecStatus::InvalidRecord;
    result.failedLine = 2U;
    result.reasonCode = "creative_world_layout_decode_schema_mismatch";
    return result;
  }
  std::size_t declaredRecords = 3U;
  const std::size_t declaredCounts[] = {
      buildingCount, roomCount, boxCount, wallCount, openingCount,
      profileCount, pathCount, pointCount,
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
  const auto readBuilding = [&](RecordReader& reader,
                                CreativeWorldLayoutBuilding& building) {
    std::uint8_t rootMode = 0U;
    std::size_t tagCount = 0U;
    if (!reader.readLiteral("B") || !reader.readHex(building.stableKey) ||
        !reader.readHex(building.name) || !reader.readUnsigned(rootMode) ||
        rootMode > enumValue(CreativeBuildingRootMode::ExistingRoom) ||
        !readRect(reader, building.rootFootprint) ||
        !reader.readI32(building.rootBaseLayer) ||
        !reader.readUnsigned(building.rootHeightCells) ||
        !reader.readBool(building.visible) || !reader.readSize(tagCount) ||
        tagCount > kCreativeWorldLayoutCodecMaxRecords - totalTagCount) {
      return false;
    }
    totalTagCount += tagCount;
    building.rootMode = static_cast<CreativeBuildingRootMode>(rootMode);
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
  const auto readRoom = [](RecordReader& reader,
                           CreativeWorldLayoutRoom& room) {
    return reader.readLiteral("R") && reader.readSize(room.buildingIndex) &&
           reader.readHex(room.stableKey) && reader.readHex(room.name) &&
           readRect(reader, room.footprint) &&
           reader.readI32(room.baseLayer) &&
           reader.readUnsigned(room.wallHeightCells) &&
           reader.readDouble(room.wallThicknessCells) &&
           std::isfinite(room.wallThicknessCells) &&
           reader.readUnsigned(room.floorThicknessCells);
  };
  if (!readTable(lines, lineIndex, roomCount, result.layout.rooms, readRoom,
                 result)) {
    return result;
  }
  const auto readBox = [](RecordReader& reader, CreativeWorldLayoutBox& box) {
    std::uint16_t kind = 0U;
    if (!reader.readLiteral("X") || !reader.readSize(box.buildingIndex) ||
        !reader.readUnsigned(kind) ||
        kind >= enumValue(CreativeObjectKind::Count) ||
        !reader.readHex(box.stableKey) || !reader.readHex(box.name) ||
        !readRect(reader, box.footprint) || !reader.readI32(box.baseLayer) ||
        !reader.readUnsigned(box.heightCells)) {
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
           reader.readI32(wall.baseLayer) &&
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
    const bool parsed = hostParsed &&
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
    if (!parsed || !std::isfinite(opening.centerOffsetCells) ||
        !std::isfinite(opening.widthCells) ||
        !std::isfinite(opening.cutoutBottomCells) ||
        !std::isfinite(opening.cutoutHeightCells) ||
        !std::isfinite(opening.insertBottomCells) ||
        !std::isfinite(opening.insertHeightCells) ||
        !std::isfinite(opening.insertWidthCells) ||
        !std::isfinite(opening.insertThicknessCells)) {
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

  // Version 1 had no room table and every opening referenced an explicit wall.
  // Decoding migrates it to current in-memory truth before callers compile it.
  result.layout.schemaVersion = kCreativeWorldLayoutSchemaVersion;

  result.accepted = true;
  result.status = CreativeWorldLayoutCodecStatus::Ready;
  result.reasonCode = "creative_world_layout_decoded";
  return result;
}

}  // namespace iggy3d::creative
