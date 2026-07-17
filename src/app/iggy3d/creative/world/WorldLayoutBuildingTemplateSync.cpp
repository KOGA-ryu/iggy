#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kProvenancePrefix =
    "iggy3d.world_layout.building_template.";
constexpr std::string_view kTemplateIdPrefix =
    "iggy3d.world_layout.building_template.id=";
constexpr std::string_view kSourceFingerprintPrefix =
    "iggy3d.world_layout.building_template.source=";
constexpr std::string_view kInstanceBaselinePrefix =
    "iggy3d.world_layout.building_template.baseline=";
constexpr std::string_view kOrientationPrefix =
    "iggy3d.world_layout.building_template.orientation=";
constexpr std::string_view kAnchorXPrefix =
    "iggy3d.world_layout.building_template.anchor_x=";
constexpr std::string_view kAnchorZPrefix =
    "iggy3d.world_layout.building_template.anchor_z=";
constexpr std::uint64_t kFingerprintOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFingerprintPrime = 1099511628211ULL;
constexpr long double kFingerprintQuantization = 1'000'000.0L;

struct FingerprintBuilder {
  std::uint64_t value = kFingerprintOffsetBasis;
  bool valid = true;

  void appendByte(std::uint8_t byte) noexcept {
    value ^= byte;
    value *= kFingerprintPrime;
  }

  void appendUnsigned(std::uint64_t item) noexcept {
    for (std::size_t index = 0U; index < sizeof(item); ++index) {
      appendByte(static_cast<std::uint8_t>(item & 0xffU));
      item >>= 8U;
    }
  }

  void appendSigned(std::int64_t item) noexcept {
    appendUnsigned(static_cast<std::uint64_t>(item));
  }

  void appendBool(bool item) noexcept { appendByte(item ? 1U : 0U); }

  void appendString(std::string_view item) noexcept {
    appendUnsigned(item.size());
    for (char character : item) {
      appendByte(static_cast<std::uint8_t>(character));
    }
  }

  void appendDouble(double item) noexcept {
    const long double widened = static_cast<long double>(item);
    const long double limit =
        static_cast<long double>(std::numeric_limits<std::int64_t>::max()) /
        kFingerprintQuantization;
    if (!std::isfinite(item) || std::abs(widened) > limit) {
      valid = false;
      return;
    }
    appendSigned(static_cast<std::int64_t>(
        std::llround(widened * kFingerprintQuantization)));
  }
};

struct OrientationBasis {
  std::int8_t xx = 1;
  std::int8_t xz = 0;
  std::int8_t zx = 0;
  std::int8_t zz = 1;

  friend bool operator==(OrientationBasis, OrientationBasis) = default;
};

OrientationBasis basisForOrientation(
    CreativeWorldLayoutBuildingTemplateOrientation orientation) noexcept {
  switch (orientation) {
    case CreativeWorldLayoutBuildingTemplateOrientation::Identity:
      return {1, 0, 0, 1};
    case CreativeWorldLayoutBuildingTemplateOrientation::RotateRight90:
      return {0, -1, 1, 0};
    case CreativeWorldLayoutBuildingTemplateOrientation::Rotate180:
      return {-1, 0, 0, -1};
    case CreativeWorldLayoutBuildingTemplateOrientation::RotateLeft90:
      return {0, 1, -1, 0};
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorX:
      return {-1, 0, 0, 1};
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorZ:
      return {1, 0, 0, -1};
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorDiagonal:
      return {0, 1, 1, 0};
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorAntiDiagonal:
      return {0, -1, -1, 0};
    case CreativeWorldLayoutBuildingTemplateOrientation::Count:
      break;
  }
  return {};
}

OrientationBasis basisForOperation(
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  switch (operation) {
    case CreativeWorldLayoutBuildingTransformOperation::RotateLeft90:
      return {0, 1, -1, 0};
    case CreativeWorldLayoutBuildingTransformOperation::RotateRight90:
      return {0, -1, 1, 0};
    case CreativeWorldLayoutBuildingTransformOperation::MirrorX:
      return {-1, 0, 0, 1};
    case CreativeWorldLayoutBuildingTransformOperation::MirrorZ:
      return {1, 0, 0, -1};
    case CreativeWorldLayoutBuildingTransformOperation::Count:
      break;
  }
  return {};
}

OrientationBasis multiply(OrientationBasis left,
                          OrientationBasis right) noexcept {
  return {
      static_cast<std::int8_t>(left.xx * right.xx + left.xz * right.zx),
      static_cast<std::int8_t>(left.xx * right.xz + left.xz * right.zz),
      static_cast<std::int8_t>(left.zx * right.xx + left.zz * right.zx),
      static_cast<std::int8_t>(left.zx * right.xz + left.zz * right.zz),
  };
}

CreativeWorldLayoutBuildingTemplateOrientation orientationForBasis(
    OrientationBasis basis) noexcept {
  for (std::uint8_t value = 0U;
       value < static_cast<std::uint8_t>(
                   CreativeWorldLayoutBuildingTemplateOrientation::Count);
       ++value) {
    const auto orientation =
        static_cast<CreativeWorldLayoutBuildingTemplateOrientation>(value);
    if (basisForOrientation(orientation) == basis) {
      return orientation;
    }
  }
  return CreativeWorldLayoutBuildingTemplateOrientation::Count;
}

bool parseHex64(std::string_view text, std::uint64_t& output) noexcept {
  if (text.size() != 16U) {
    return false;
  }
  output = 0U;
  for (char character : text) {
    output <<= 4U;
    if (character >= '0' && character <= '9') {
      output |= static_cast<std::uint64_t>(character - '0');
    } else if (character >= 'a' && character <= 'f') {
      output |= static_cast<std::uint64_t>(character - 'a' + 10);
    } else {
      return false;
    }
  }
  return true;
}

std::string hex64(std::uint64_t value) {
  constexpr std::string_view digits = "0123456789abcdef";
  std::string output(16U, '0');
  for (std::size_t index = 0U; index < output.size(); ++index) {
    const std::size_t shift = (output.size() - index - 1U) * 4U;
    output[index] = digits[(value >> shift) & 0x0fU];
  }
  return output;
}

template <typename Integer>
bool parseInteger(std::string_view text, Integer& output) noexcept {
  if (text.empty() || text.front() == '+') {
    return false;
  }
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), output);
  return parsed.ec == std::errc{} &&
         parsed.ptr == text.data() + text.size();
}

bool openingOwnedByBuilding(const CreativeWorldLayout& layout,
                            const CreativeWorldLayoutOpening& opening,
                            std::size_t buildingIndex) noexcept {
  return opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge
             ? layout.rooms[opening.roomIndex].buildingIndex == buildingIndex
             : layout.walls[opening.wallIndex].buildingIndex == buildingIndex;
}

std::size_t ownedRoomOrdinal(const CreativeWorldLayout& layout,
                             std::size_t buildingIndex,
                             std::size_t roomIndex) noexcept {
  std::size_t ordinal = 0U;
  for (std::size_t index = 0U; index < roomIndex; ++index) {
    ordinal += layout.rooms[index].buildingIndex == buildingIndex ? 1U : 0U;
  }
  return ordinal;
}

std::size_t ownedLevelOrdinal(const CreativeWorldLayout& layout,
                              std::size_t buildingIndex,
                              std::size_t levelIndex) noexcept {
  std::size_t ordinal = 0U;
  for (std::size_t index = 0U; index < levelIndex; ++index) {
    ordinal += layout.levels[index].buildingIndex == buildingIndex ? 1U : 0U;
  }
  return ordinal;
}

std::size_t ownedWallOrdinal(const CreativeWorldLayout& layout,
                             std::size_t buildingIndex,
                             std::size_t wallIndex) noexcept {
  std::size_t ordinal = 0U;
  for (std::size_t index = 0U; index < wallIndex; ++index) {
    ordinal += layout.walls[index].buildingIndex == buildingIndex ? 1U : 0U;
  }
  return ordinal;
}

void appendRelativeCoord(FingerprintBuilder& builder,
                         CreativeTerrainCoord2 coord,
                         CreativeTerrainCoord2 anchor) noexcept {
  builder.appendSigned(static_cast<std::int64_t>(coord.x) - anchor.x);
  builder.appendSigned(static_cast<std::int64_t>(coord.z) - anchor.z);
}

void appendRelativeRect(FingerprintBuilder& builder,
                        CreativeWorldLayoutRect rect,
                        CreativeTerrainCoord2 anchor) noexcept {
  appendRelativeCoord(builder, rect.minimum, anchor);
  appendRelativeCoord(builder, rect.maximum, anchor);
}

template <typename Range, typename Predicate>
std::size_t countIf(const Range& range, Predicate predicate) noexcept {
  return static_cast<std::size_t>(std::count_if(range.begin(), range.end(),
                                                std::move(predicate)));
}

bool setProvenanceTags(
    CreativeWorldLayoutBuilding& building,
    const CreativeWorldLayoutBuildingTemplateInstanceProvenance& provenance) {
  if (!provenance.valid ||
      !validCreativeWorldLayoutStableKey(provenance.templateId) ||
      provenance.orientation >=
          CreativeWorldLayoutBuildingTemplateOrientation::Count) {
    return false;
  }
  std::erase_if(building.tags, [](const std::string& tag) {
    return isCreativeWorldLayoutBuildingTemplateProvenanceTag(tag);
  });
  building.tags.push_back(std::string{kTemplateIdPrefix} +
                          provenance.templateId);
  building.tags.push_back(std::string{kSourceFingerprintPrefix} +
                          hex64(provenance.sourceFingerprint));
  building.tags.push_back(std::string{kInstanceBaselinePrefix} +
                          hex64(provenance.instanceBaselineFingerprint));
  building.tags.push_back(
      std::string{kOrientationPrefix} +
      std::to_string(static_cast<std::uint8_t>(provenance.orientation)));
  building.tags.push_back(std::string{kAnchorXPrefix} +
                          std::to_string(provenance.anchor.x));
  building.tags.push_back(std::string{kAnchorZPrefix} +
                          std::to_string(provenance.anchor.z));
  return true;
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutBuildingTemplateOrientation orientation) noexcept {
  switch (orientation) {
    case CreativeWorldLayoutBuildingTemplateOrientation::Identity:
      return "Identity";
    case CreativeWorldLayoutBuildingTemplateOrientation::RotateRight90:
      return "RotateRight90";
    case CreativeWorldLayoutBuildingTemplateOrientation::Rotate180:
      return "Rotate180";
    case CreativeWorldLayoutBuildingTemplateOrientation::RotateLeft90:
      return "RotateLeft90";
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorX:
      return "MirrorX";
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorZ:
      return "MirrorZ";
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorDiagonal:
      return "MirrorDiagonal";
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorAntiDiagonal:
      return "MirrorAntiDiagonal";
    case CreativeWorldLayoutBuildingTemplateOrientation::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(
    CreativeWorldLayoutBuildingTemplateSyncState state) noexcept {
  switch (state) {
    case CreativeWorldLayoutBuildingTemplateSyncState::Unlinked:
      return "Unlinked";
    case CreativeWorldLayoutBuildingTemplateSyncState::Current:
      return "Current";
    case CreativeWorldLayoutBuildingTemplateSyncState::SourceChanged:
      return "SourceChanged";
    case CreativeWorldLayoutBuildingTemplateSyncState::LocallyModified:
      return "LocallyModified";
    case CreativeWorldLayoutBuildingTemplateSyncState::Conflict:
      return "Conflict";
    case CreativeWorldLayoutBuildingTemplateSyncState::SourceMissing:
      return "SourceMissing";
  }
  return "Invalid";
}

std::string_view toString(
    CreativeWorldLayoutBuildingTemplateRefreshMode mode) noexcept {
  switch (mode) {
    case CreativeWorldLayoutBuildingTemplateRefreshMode::SelectedInstance:
      return "SelectedInstance";
    case CreativeWorldLayoutBuildingTemplateRefreshMode::SafeInstances:
      return "SafeInstances";
    case CreativeWorldLayoutBuildingTemplateRefreshMode::ForceAll:
      return "ForceAll";
    case CreativeWorldLayoutBuildingTemplateRefreshMode::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(
    CreativeWorldLayoutBuildingTemplateRefreshStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutBuildingTemplateRefreshStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutBuildingTemplateRefreshStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutBuildingTemplateRefreshStatus::InvalidOwnership:
      return "InvalidOwnership";
    case CreativeWorldLayoutBuildingTemplateRefreshStatus::NoMatchingInstances:
      return "NoMatchingInstances";
    case CreativeWorldLayoutBuildingTemplateRefreshStatus::NoEligibleInstances:
      return "NoEligibleInstances";
    case CreativeWorldLayoutBuildingTemplateRefreshStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeWorldLayoutBuildingTemplateRefreshStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

bool isCreativeWorldLayoutBuildingTemplateProvenanceTag(
    std::string_view tag) noexcept {
  return tag.starts_with(kProvenancePrefix);
}

CreativeWorldLayoutBuildingTemplateFingerprint
fingerprintCreativeWorldLayoutBuilding(const CreativeWorldLayout& layout,
                                       std::size_t buildingIndex) noexcept {
  CreativeWorldLayoutBuildingTemplateFingerprint result;
  CreativeWorldLayoutBuildingBounds bounds;
  if (buildingIndex >= layout.buildings.size() ||
      !validCreativeWorldLayoutBuildingOwnership(layout) ||
      !measureCreativeWorldLayoutBuildingBounds(layout, buildingIndex,
                                                bounds)) {
    return result;
  }

  FingerprintBuilder builder;
  const CreativeWorldLayoutBuilding& building = layout.buildings[buildingIndex];
  builder.appendUnsigned(static_cast<std::uint8_t>(building.rootMode));
  builder.appendString(building.name);
  if (building.rootMode != CreativeBuildingRootMode::None) {
    appendRelativeRect(builder, building.rootFootprint, bounds.minimum);
  }
  builder.appendSigned(building.rootBaseLayer);
  builder.appendUnsigned(building.rootHeightCells);
  builder.appendBool(building.visible);
  const std::size_t tagCount = countIf(
      building.tags, [](const std::string& tag) {
        return !isCreativeWorldLayoutBuildingTemplateProvenanceTag(tag);
      });
  builder.appendUnsigned(tagCount);
  for (const std::string& tag : building.tags) {
    if (!isCreativeWorldLayoutBuildingTemplateProvenanceTag(tag)) {
      builder.appendString(tag);
    }
  }

  builder.appendUnsigned(
      countIf(layout.levels, [buildingIndex](const auto& level) {
        return level.buildingIndex == buildingIndex;
      }));
  for (const CreativeWorldLayoutLevel& level : layout.levels) {
    if (level.buildingIndex != buildingIndex) {
      continue;
    }
    builder.appendString(level.name);
    builder.appendDouble(level.floorTopLayer);
    builder.appendUnsigned(level.wallHeightCells);
    builder.appendUnsigned(level.floorThicknessLayers);
    builder.appendUnsigned(level.ceilingThicknessLayers);
    builder.appendUnsigned(level.roofThicknessLayers);
  }

  builder.appendUnsigned(countIf(layout.rooms, [buildingIndex](const auto& room) {
    return room.buildingIndex == buildingIndex;
  }));
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex != buildingIndex) {
      continue;
    }
    builder.appendString(room.name);
    builder.appendUnsigned(
        ownedLevelOrdinal(layout, buildingIndex, room.levelIndex));
    appendRelativeRect(builder, room.footprint, bounds.minimum);
    builder.appendDouble(room.wallThicknessCells);
  }

  builder.appendUnsigned(countIf(
      layout.verticalConnectors, [buildingIndex](const auto& connector) {
        return connector.buildingIndex == buildingIndex;
      }));
  for (const CreativeWorldLayoutVerticalConnector& connector :
       layout.verticalConnectors) {
    if (connector.buildingIndex != buildingIndex) {
      continue;
    }
    builder.appendUnsigned(
        ownedRoomOrdinal(layout, buildingIndex, connector.lowerRoomIndex));
    builder.appendUnsigned(
        ownedRoomOrdinal(layout, buildingIndex, connector.upperRoomIndex));
    builder.appendUnsigned(static_cast<std::uint8_t>(connector.kind));
    builder.appendUnsigned(static_cast<std::uint8_t>(connector.direction));
    builder.appendString(connector.name);
    appendRelativeRect(builder, connector.footprint, bounds.minimum);
  }

  builder.appendUnsigned(
      countIf(layout.boxes, [buildingIndex](const auto& box) {
        return box.buildingIndex == buildingIndex;
      }));
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex != buildingIndex) {
      continue;
    }
    builder.appendUnsigned(static_cast<std::uint16_t>(box.kind));
    builder.appendString(box.name);
    appendRelativeRect(builder, box.footprint, bounds.minimum);
    builder.appendDouble(box.anchorLayer);
    builder.appendUnsigned(box.layerCount);
  }

  builder.appendUnsigned(countIf(layout.walls, [buildingIndex](const auto& wall) {
    return wall.buildingIndex == buildingIndex;
  }));
  for (const CreativeWorldLayoutWall& wall : layout.walls) {
    if (wall.buildingIndex != buildingIndex) {
      continue;
    }
    builder.appendString(wall.name);
    appendRelativeCoord(builder, wall.start, bounds.minimum);
    appendRelativeCoord(builder, wall.end, bounds.minimum);
    builder.appendDouble(wall.baseLayer);
    builder.appendUnsigned(wall.heightCells);
    builder.appendDouble(wall.thicknessCells);
  }

  builder.appendUnsigned(countIf(
      layout.openings, [&](const CreativeWorldLayoutOpening& opening) {
        return openingOwnedByBuilding(layout, opening, buildingIndex);
      }));
  for (const CreativeWorldLayoutOpening& opening : layout.openings) {
    if (!openingOwnedByBuilding(layout, opening, buildingIndex)) {
      continue;
    }
    builder.appendUnsigned(static_cast<std::uint8_t>(opening.hostKind));
    builder.appendUnsigned(
        opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge
            ? ownedRoomOrdinal(layout, buildingIndex, opening.roomIndex)
            : ownedWallOrdinal(layout, buildingIndex, opening.wallIndex));
    builder.appendUnsigned(static_cast<std::uint8_t>(opening.roomEdge));
    builder.appendUnsigned(static_cast<std::uint8_t>(opening.kind));
    builder.appendUnsigned(static_cast<std::uint8_t>(opening.pose));
    builder.appendString(opening.name);
    builder.appendDouble(opening.centerOffsetCells);
    builder.appendDouble(opening.widthCells);
    builder.appendDouble(opening.cutoutBottomCells);
    builder.appendDouble(opening.cutoutHeightCells);
    builder.appendBool(opening.includeInsert);
    builder.appendDouble(opening.insertBottomCells);
    builder.appendDouble(opening.insertHeightCells);
    builder.appendDouble(opening.insertWidthCells);
    builder.appendDouble(opening.insertThicknessCells);
  }

  result.valid = builder.valid;
  result.value = builder.valid ? builder.value : 0U;
  return result;
}

CreativeWorldLayoutBuildingTemplateInstanceProvenance
creativeWorldLayoutBuildingTemplateInstanceProvenance(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex) {
  CreativeWorldLayoutBuildingTemplateInstanceProvenance result;
  if (buildingIndex >= layout.buildings.size()) {
    return result;
  }
  std::uint8_t seen = 0U;
  constexpr std::uint8_t kExpected = 0x3fU;
  for (const std::string& ownedTag : layout.buildings[buildingIndex].tags) {
    const std::string_view tag = ownedTag;
    if (!isCreativeWorldLayoutBuildingTemplateProvenanceTag(tag)) {
      continue;
    }
    result.present = true;
    if (tag.starts_with(kTemplateIdPrefix) && (seen & 0x01U) == 0U) {
      result.templateId = tag.substr(kTemplateIdPrefix.size());
      seen |= 0x01U;
    } else if (tag.starts_with(kSourceFingerprintPrefix) &&
               (seen & 0x02U) == 0U &&
               parseHex64(tag.substr(kSourceFingerprintPrefix.size()),
                          result.sourceFingerprint)) {
      seen |= 0x02U;
    } else if (tag.starts_with(kInstanceBaselinePrefix) &&
               (seen & 0x04U) == 0U &&
               parseHex64(tag.substr(kInstanceBaselinePrefix.size()),
                          result.instanceBaselineFingerprint)) {
      seen |= 0x04U;
    } else if (tag.starts_with(kOrientationPrefix) &&
               (seen & 0x08U) == 0U) {
      std::uint8_t orientation = 0U;
      if (!parseInteger(tag.substr(kOrientationPrefix.size()), orientation) ||
          orientation >= static_cast<std::uint8_t>(
                             CreativeWorldLayoutBuildingTemplateOrientation::
                                 Count)) {
        return result;
      }
      result.orientation =
          static_cast<CreativeWorldLayoutBuildingTemplateOrientation>(
              orientation);
      seen |= 0x08U;
    } else if (tag.starts_with(kAnchorXPrefix) && (seen & 0x10U) == 0U &&
               parseInteger(tag.substr(kAnchorXPrefix.size()),
                            result.anchor.x)) {
      seen |= 0x10U;
    } else if (tag.starts_with(kAnchorZPrefix) && (seen & 0x20U) == 0U &&
               parseInteger(tag.substr(kAnchorZPrefix.size()),
                            result.anchor.z)) {
      seen |= 0x20U;
    } else {
      return result;
    }
  }
  result.valid = result.present && seen == kExpected &&
                 validCreativeWorldLayoutStableKey(result.templateId);
  return result;
}

bool setCreativeWorldLayoutBuildingTemplateInstanceProvenance(
    CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    const CreativeWorldLayoutBuildingTemplateInstanceProvenance& provenance) {
  return buildingIndex < layout.buildings.size() &&
         setProvenanceTags(layout.buildings[buildingIndex], provenance);
}

CreativeWorldLayoutBuildingTemplateOrientation
composeCreativeWorldLayoutBuildingTemplateOrientation(
    CreativeWorldLayoutBuildingTemplateOrientation current,
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  if (current >= CreativeWorldLayoutBuildingTemplateOrientation::Count ||
      operation >= CreativeWorldLayoutBuildingTransformOperation::Count) {
    return CreativeWorldLayoutBuildingTemplateOrientation::Count;
  }
  return orientationForBasis(
      multiply(basisForOperation(operation), basisForOrientation(current)));
}

CreativeWorldLayoutBuildingTemplateOrientation
inverseCreativeWorldLayoutBuildingTemplateOrientation(
    CreativeWorldLayoutBuildingTemplateOrientation orientation) noexcept {
  switch (orientation) {
    case CreativeWorldLayoutBuildingTemplateOrientation::RotateRight90:
      return CreativeWorldLayoutBuildingTemplateOrientation::RotateLeft90;
    case CreativeWorldLayoutBuildingTemplateOrientation::RotateLeft90:
      return CreativeWorldLayoutBuildingTemplateOrientation::RotateRight90;
    case CreativeWorldLayoutBuildingTemplateOrientation::Identity:
    case CreativeWorldLayoutBuildingTemplateOrientation::Rotate180:
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorX:
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorZ:
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorDiagonal:
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorAntiDiagonal:
      return orientation;
    case CreativeWorldLayoutBuildingTemplateOrientation::Count:
      break;
  }
  return CreativeWorldLayoutBuildingTemplateOrientation::Count;
}

CreativeWorldLayoutBuildingTemplateResult
orientCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayoutBuildingTemplate& source,
    CreativeWorldLayoutBuildingTemplateOrientation orientation) {
  CreativeWorldLayoutBuildingTemplateResult result;
  result.requested = true;
  if (!validCreativeWorldLayoutBuildingTemplate(source) ||
      orientation >= CreativeWorldLayoutBuildingTemplateOrientation::Count) {
    result.status = CreativeWorldLayoutBuildingTemplateStatus::InvalidRequest;
    result.reasonCode =
        "creative_world_layout_building_template_orientation_invalid";
    return result;
  }
  result.value = source;
  const OrientationBasis deltaBasis = multiply(
      basisForOrientation(orientation),
      basisForOrientation(
          inverseCreativeWorldLayoutBuildingTemplateOrientation(
              source.orientation)));
  const CreativeWorldLayoutBuildingTemplateOrientation delta =
      orientationForBasis(deltaBasis);
  std::array<CreativeWorldLayoutBuildingTransformOperation, 2U> operations{};
  std::size_t operationCount = 0U;
  switch (delta) {
    case CreativeWorldLayoutBuildingTemplateOrientation::Identity:
      break;
    case CreativeWorldLayoutBuildingTemplateOrientation::RotateRight90:
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
      break;
    case CreativeWorldLayoutBuildingTemplateOrientation::Rotate180:
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
      break;
    case CreativeWorldLayoutBuildingTemplateOrientation::RotateLeft90:
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::RotateLeft90;
      break;
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorX:
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::MirrorX;
      break;
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorZ:
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::MirrorZ;
      break;
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorDiagonal:
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::MirrorX;
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::RotateLeft90;
      break;
    case CreativeWorldLayoutBuildingTemplateOrientation::MirrorAntiDiagonal:
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::MirrorX;
      operations[operationCount++] =
          CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
      break;
    case CreativeWorldLayoutBuildingTemplateOrientation::Count:
      result.value = {};
      result.status = CreativeWorldLayoutBuildingTemplateStatus::InvalidRequest;
      result.reasonCode =
          "creative_world_layout_building_template_orientation_invalid";
      return result;
  }
  for (std::size_t index = 0U; index < operationCount; ++index) {
    result = transformCreativeWorldLayoutBuildingTemplate(result.value,
                                                          operations[index]);
    if (!result.accepted) {
      return result;
    }
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutBuildingTemplateStatus::Ready;
  result.reasonCode =
      "creative_world_layout_building_template_orientation_ready";
  return result;
}

CreativeWorldLayoutBuildingTemplateSyncReceipt
inspectCreativeWorldLayoutBuildingTemplateSync(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    const CreativeWorldLayoutBuildingTemplate* sourceTemplate) {
  CreativeWorldLayoutBuildingTemplateSyncReceipt result;
  result.requested = true;
  result.buildingIndex = buildingIndex;
  if (buildingIndex >= layout.buildings.size()) {
    result.state = CreativeWorldLayoutBuildingTemplateSyncState::Conflict;
    result.reasonCode =
        "creative_world_layout_building_template_sync_building_invalid";
    return result;
  }
  result.provenance =
      creativeWorldLayoutBuildingTemplateInstanceProvenance(layout,
                                                            buildingIndex);
  if (!result.provenance.present) {
    result.accepted = true;
    result.state = CreativeWorldLayoutBuildingTemplateSyncState::Unlinked;
    result.reasonCode =
        "creative_world_layout_building_template_sync_unlinked";
    return result;
  }
  result.instanceFingerprint =
      fingerprintCreativeWorldLayoutBuilding(layout, buildingIndex);
  if (!result.provenance.valid || !result.instanceFingerprint.valid) {
    result.accepted = true;
    result.state = CreativeWorldLayoutBuildingTemplateSyncState::Conflict;
    result.reasonCode =
        "creative_world_layout_building_template_sync_provenance_invalid";
    return result;
  }
  if (sourceTemplate == nullptr ||
      sourceTemplate->templateId != result.provenance.templateId) {
    result.accepted = true;
    result.state = CreativeWorldLayoutBuildingTemplateSyncState::SourceMissing;
    result.reasonCode =
        "creative_world_layout_building_template_sync_source_missing";
    return result;
  }
  result.sourceFingerprint = sourceTemplate->sourceFingerprint;
  if (!validCreativeWorldLayoutBuildingTemplate(*sourceTemplate) ||
      !result.sourceFingerprint.valid) {
    result.accepted = true;
    result.state = CreativeWorldLayoutBuildingTemplateSyncState::Conflict;
    result.reasonCode =
        "creative_world_layout_building_template_sync_source_invalid";
    return result;
  }

  const bool sourceChanged = result.provenance.sourceFingerprint !=
                             result.sourceFingerprint.value;
  const bool instanceChanged =
      result.provenance.instanceBaselineFingerprint !=
      result.instanceFingerprint.value;
  result.accepted = true;
  if (!sourceChanged && !instanceChanged) {
    result.state = CreativeWorldLayoutBuildingTemplateSyncState::Current;
    result.reasonCode =
        "creative_world_layout_building_template_sync_current";
  } else if (sourceChanged && !instanceChanged) {
    result.state = CreativeWorldLayoutBuildingTemplateSyncState::SourceChanged;
    result.reasonCode =
        "creative_world_layout_building_template_sync_source_changed";
  } else if (!sourceChanged) {
    result.state =
        CreativeWorldLayoutBuildingTemplateSyncState::LocallyModified;
    result.reasonCode =
        "creative_world_layout_building_template_sync_locally_modified";
  } else {
    result.state = CreativeWorldLayoutBuildingTemplateSyncState::Conflict;
    result.reasonCode =
        "creative_world_layout_building_template_sync_conflict";
  }
  return result;
}

}  // namespace iggy3d::creative
