#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

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
#include <string_view>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kProvenancePrefix =
    "iggy3d.world_layout.building_blockout.";
constexpr std::string_view kVersionPrefix =
    "iggy3d.world_layout.building_blockout.version=";
constexpr std::string_view kBaselinePrefix =
    "iggy3d.world_layout.building_blockout.baseline=";
constexpr std::string_view kMinimumXPrefix =
    "iggy3d.world_layout.building_blockout.minimum_x=";
constexpr std::string_view kMinimumZPrefix =
    "iggy3d.world_layout.building_blockout.minimum_z=";
constexpr std::string_view kMaximumXPrefix =
    "iggy3d.world_layout.building_blockout.maximum_x=";
constexpr std::string_view kMaximumZPrefix =
    "iggy3d.world_layout.building_blockout.maximum_z=";
constexpr std::string_view kPatternPrefix =
    "iggy3d.world_layout.building_blockout.pattern=";
constexpr std::string_view kWallThicknessPrefix =
    "iggy3d.world_layout.building_blockout.wall_thickness=";
constexpr std::string_view kConnectRoomsPrefix =
    "iggy3d.world_layout.building_blockout.connect_rooms=";
constexpr std::string_view kIncludeEntrancePrefix =
    "iggy3d.world_layout.building_blockout.include_entrance=";
constexpr std::string_view kEntranceEdgePrefix =
    "iggy3d.world_layout.building_blockout.entrance_edge=";
constexpr std::string_view kEntranceOffsetPrefix =
    "iggy3d.world_layout.building_blockout.entrance_offset=";
constexpr std::string_view kIncludeWindowsPrefix =
    "iggy3d.world_layout.building_blockout.include_windows=";
constexpr std::string_view kWallHeightPrefix =
    "iggy3d.world_layout.building_blockout.wall_height=";
constexpr std::string_view kStoreyCountPrefix =
    "iggy3d.world_layout.building_blockout.storey_count=";
constexpr std::string_view kConnectStoreysPrefix =
    "iggy3d.world_layout.building_blockout.connect_storeys=";
constexpr std::string_view kConnectorKindPrefix =
    "iggy3d.world_layout.building_blockout.connector_kind=";
constexpr std::string_view kPreferredDirectionPrefix =
    "iggy3d.world_layout.building_blockout.preferred_direction=";
constexpr std::string_view kFloorTopPrefix =
    "iggy3d.world_layout.building_blockout.floor_top=";
constexpr std::string_view kFloorThicknessPrefix =
    "iggy3d.world_layout.building_blockout.floor_thickness=";
constexpr std::string_view kRoofThicknessPrefix =
    "iggy3d.world_layout.building_blockout.roof_thickness=";
constexpr std::string_view kRoofStylePrefix =
    "iggy3d.world_layout.building_blockout.roof_style=";
constexpr std::string_view kRoofRidgeAxisPrefix =
    "iggy3d.world_layout.building_blockout.roof_ridge_axis=";
constexpr std::string_view kRoofPitchPrefix =
    "iggy3d.world_layout.building_blockout.roof_pitch=";
constexpr std::string_view kRoofOverhangPrefix =
    "iggy3d.world_layout.building_blockout.roof_overhang=";

constexpr std::uint32_t kExpectedFields = (1U << 25U) - 1U;

template <typename Integer>
bool parseInteger(std::string_view text, Integer& output) noexcept {
  if (text.empty() || text.front() == '+') {
    return false;
  }
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), output);
  return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

bool parseBool(std::string_view text, bool& output) noexcept {
  std::uint8_t value = 0U;
  if (!parseInteger(text, value) || value > 1U) {
    return false;
  }
  output = value == 1U;
  return true;
}

bool parseDouble(std::string_view text, double& output) noexcept {
  if (text.empty()) {
    return false;
  }
  std::string owned{text};
  char* end = nullptr;
  errno = 0;
  const double parsed = std::strtod(owned.c_str(), &end);
  if (errno == ERANGE || end != owned.c_str() + owned.size()) {
    return false;
  }
  output = parsed;
  return true;
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

std::string encodeDouble(double value) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(std::numeric_limits<double>::max_digits10)
         << value;
  return output.str();
}

template <typename Enum>
std::string enumValue(Enum value) {
  return std::to_string(static_cast<std::uint8_t>(value));
}

bool claim(std::uint32_t& seen, std::uint32_t bit) noexcept {
  if ((seen & bit) != 0U) {
    return false;
  }
  seen |= bit;
  return true;
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutSyncState state) noexcept {
  switch (state) {
    case CreativeWorldLayoutBuildingBlockoutSyncState::Unlinked:
      return "Unlinked";
    case CreativeWorldLayoutBuildingBlockoutSyncState::Current:
      return "Current";
    case CreativeWorldLayoutBuildingBlockoutSyncState::LocallyModified:
      return "LocallyModified";
    case CreativeWorldLayoutBuildingBlockoutSyncState::Invalid:
      return "Invalid";
  }
  return "Invalid";
}

bool validCreativeWorldLayoutBuildingBlockoutRecipe(
    const CreativeWorldLayoutBuildingBlockoutRecipe& recipe) noexcept {
  if (recipe.version != kCreativeWorldLayoutBuildingBlockoutRecipeVersion ||
      !std::isfinite(recipe.floorTopLayer) ||
      recipe.floorThicknessLayers == 0U || recipe.roofThicknessLayers == 0U ||
      !validCreativeStructuralRoofSettings(
          recipe.roofStyle, recipe.roofRidgeAxis, recipe.roofPitchDegrees,
          recipe.roofOverhangCells) ||
      recipe.roofOverhangCells > kMaximumCreativeWorldLayoutRoofOverhangCells) {
    return false;
  }
  const CreativeWorldLayoutBuildingBlockoutPlan plan =
      planCreativeWorldLayoutBuildingBlockout(recipe.request);
  if (!plan.accepted) {
    return false;
  }
  const long double top =
      static_cast<long double>(recipe.floorTopLayer) +
      static_cast<long double>(recipe.request.wallHeightCells) *
          plan.storeyCount;
  return std::isfinite(top) &&
         top >= -static_cast<long double>(std::numeric_limits<double>::max()) &&
         top <= static_cast<long double>(std::numeric_limits<double>::max()) &&
         static_cast<double>(top) > recipe.floorTopLayer;
}

bool isCreativeWorldLayoutBuildingBlockoutProvenanceTag(
    std::string_view tag) noexcept {
  return tag.starts_with(kProvenancePrefix);
}

CreativeWorldLayoutBuildingBlockoutFingerprint
fingerprintCreativeWorldLayoutBuildingBlockout(
    const CreativeWorldLayout& layout, std::size_t buildingIndex) {
  CreativeWorldLayoutBuildingBlockoutFingerprint result;
  const CreativeWorldLayoutBuildingTemplateFingerprint fingerprint =
      fingerprintCreativeWorldLayoutBuilding(layout, buildingIndex);
  result.valid = fingerprint.valid;
  result.value = fingerprint.value;
  return result;
}

CreativeWorldLayoutBuildingBlockoutProvenance
creativeWorldLayoutBuildingBlockoutProvenance(const CreativeWorldLayout& layout,
                                              std::size_t buildingIndex) {
  CreativeWorldLayoutBuildingBlockoutProvenance result;
  if (buildingIndex >= layout.buildings.size()) {
    return result;
  }

  std::uint32_t seen = 0U;
  for (const std::string& ownedTag : layout.buildings[buildingIndex].tags) {
    const std::string_view tag = ownedTag;
    if (!isCreativeWorldLayoutBuildingBlockoutProvenanceTag(tag)) {
      continue;
    }
    result.present = true;
    if (tag.starts_with(kVersionPrefix) && claim(seen, 1U << 0U) &&
        parseInteger(tag.substr(kVersionPrefix.size()),
                     result.recipe.version)) {
      continue;
    }
    if (tag.starts_with(kBaselinePrefix) && claim(seen, 1U << 1U) &&
        parseHex64(tag.substr(kBaselinePrefix.size()),
                   result.instanceBaselineFingerprint)) {
      continue;
    }
    if (tag.starts_with(kMinimumXPrefix) && claim(seen, 1U << 2U) &&
        parseInteger(tag.substr(kMinimumXPrefix.size()),
                     result.recipe.request.footprint.minimum.x)) {
      continue;
    }
    if (tag.starts_with(kMinimumZPrefix) && claim(seen, 1U << 3U) &&
        parseInteger(tag.substr(kMinimumZPrefix.size()),
                     result.recipe.request.footprint.minimum.z)) {
      continue;
    }
    if (tag.starts_with(kMaximumXPrefix) && claim(seen, 1U << 4U) &&
        parseInteger(tag.substr(kMaximumXPrefix.size()),
                     result.recipe.request.footprint.maximum.x)) {
      continue;
    }
    if (tag.starts_with(kMaximumZPrefix) && claim(seen, 1U << 5U) &&
        parseInteger(tag.substr(kMaximumZPrefix.size()),
                     result.recipe.request.footprint.maximum.z)) {
      continue;
    }
    if (tag.starts_with(kPatternPrefix) && claim(seen, 1U << 6U)) {
      std::uint8_t value = 0U;
      if (parseInteger(tag.substr(kPatternPrefix.size()), value)) {
        result.recipe.request.pattern =
            static_cast<CreativeWorldLayoutBuildingBlockoutPattern>(value);
        continue;
      }
    }
    if (tag.starts_with(kWallThicknessPrefix) && claim(seen, 1U << 7U) &&
        parseDouble(tag.substr(kWallThicknessPrefix.size()),
                    result.recipe.request.wallThicknessCells)) {
      continue;
    }
    if (tag.starts_with(kConnectRoomsPrefix) && claim(seen, 1U << 8U) &&
        parseBool(tag.substr(kConnectRoomsPrefix.size()),
                  result.recipe.request.connectRooms)) {
      continue;
    }
    if (tag.starts_with(kIncludeEntrancePrefix) && claim(seen, 1U << 9U) &&
        parseBool(tag.substr(kIncludeEntrancePrefix.size()),
                  result.recipe.request.facade.includeEntrance)) {
      continue;
    }
    if (tag.starts_with(kEntranceEdgePrefix) && claim(seen, 1U << 10U)) {
      std::uint8_t value = 0U;
      if (parseInteger(tag.substr(kEntranceEdgePrefix.size()), value)) {
        result.recipe.request.facade.entranceEdge =
            static_cast<CreativeWorldLayoutRoomEdge>(value);
        continue;
      }
    }
    if (tag.starts_with(kEntranceOffsetPrefix) && claim(seen, 1U << 11U) &&
        parseDouble(tag.substr(kEntranceOffsetPrefix.size()),
                    result.recipe.request.facade.entranceOffsetCells)) {
      continue;
    }
    if (tag.starts_with(kIncludeWindowsPrefix) && claim(seen, 1U << 12U) &&
        parseBool(tag.substr(kIncludeWindowsPrefix.size()),
                  result.recipe.request.facade.includeExteriorWindows)) {
      continue;
    }
    if (tag.starts_with(kWallHeightPrefix) && claim(seen, 1U << 13U) &&
        parseInteger(tag.substr(kWallHeightPrefix.size()),
                     result.recipe.request.wallHeightCells)) {
      continue;
    }
    if (tag.starts_with(kStoreyCountPrefix) && claim(seen, 1U << 14U) &&
        parseInteger(tag.substr(kStoreyCountPrefix.size()),
                     result.recipe.request.storeys.count)) {
      continue;
    }
    if (tag.starts_with(kConnectStoreysPrefix) && claim(seen, 1U << 15U) &&
        parseBool(tag.substr(kConnectStoreysPrefix.size()),
                  result.recipe.request.storeys.connectStoreys)) {
      continue;
    }
    if (tag.starts_with(kConnectorKindPrefix) && claim(seen, 1U << 16U)) {
      std::uint8_t value = 0U;
      if (parseInteger(tag.substr(kConnectorKindPrefix.size()), value)) {
        result.recipe.request.storeys.connectorKind =
            static_cast<CreativeWorldLayoutVerticalConnectorKind>(value);
        continue;
      }
    }
    if (tag.starts_with(kPreferredDirectionPrefix) && claim(seen, 1U << 17U)) {
      std::uint8_t value = 0U;
      if (parseInteger(tag.substr(kPreferredDirectionPrefix.size()), value)) {
        result.recipe.request.storeys.preferredDirection =
            static_cast<CreativeWorldLayoutVerticalDirection>(value);
        continue;
      }
    }
    if (tag.starts_with(kFloorTopPrefix) && claim(seen, 1U << 18U) &&
        parseDouble(tag.substr(kFloorTopPrefix.size()),
                    result.recipe.floorTopLayer)) {
      continue;
    }
    if (tag.starts_with(kFloorThicknessPrefix) && claim(seen, 1U << 19U) &&
        parseInteger(tag.substr(kFloorThicknessPrefix.size()),
                     result.recipe.floorThicknessLayers)) {
      continue;
    }
    if (tag.starts_with(kRoofThicknessPrefix) && claim(seen, 1U << 20U) &&
        parseInteger(tag.substr(kRoofThicknessPrefix.size()),
                     result.recipe.roofThicknessLayers)) {
      continue;
    }
    if (tag.starts_with(kRoofStylePrefix) && claim(seen, 1U << 21U)) {
      std::uint8_t value = 0U;
      if (parseInteger(tag.substr(kRoofStylePrefix.size()), value)) {
        result.recipe.roofStyle =
            static_cast<CreativeStructuralRoofStyle>(value);
        continue;
      }
    }
    if (tag.starts_with(kRoofRidgeAxisPrefix) && claim(seen, 1U << 22U)) {
      std::uint8_t value = 0U;
      if (parseInteger(tag.substr(kRoofRidgeAxisPrefix.size()), value)) {
        result.recipe.roofRidgeAxis =
            static_cast<CreativeStructuralRoofRidgeAxis>(value);
        continue;
      }
    }
    if (tag.starts_with(kRoofPitchPrefix) && claim(seen, 1U << 23U) &&
        parseDouble(tag.substr(kRoofPitchPrefix.size()),
                    result.recipe.roofPitchDegrees)) {
      continue;
    }
    if (tag.starts_with(kRoofOverhangPrefix) && claim(seen, 1U << 24U) &&
        parseDouble(tag.substr(kRoofOverhangPrefix.size()),
                    result.recipe.roofOverhangCells)) {
      continue;
    }
    return result;
  }

  result.valid = result.present && seen == kExpectedFields &&
                 validCreativeWorldLayoutBuildingBlockoutRecipe(result.recipe);
  return result;
}

bool setCreativeWorldLayoutBuildingBlockoutProvenance(
    CreativeWorldLayout& layout, std::size_t buildingIndex,
    const CreativeWorldLayoutBuildingBlockoutProvenance& provenance) {
  if (buildingIndex >= layout.buildings.size() || !provenance.valid ||
      !validCreativeWorldLayoutBuildingBlockoutRecipe(provenance.recipe)) {
    return false;
  }

  CreativeWorldLayoutBuilding& building = layout.buildings[buildingIndex];
  std::erase_if(building.tags, [](const std::string& tag) {
    return isCreativeWorldLayoutBuildingBlockoutProvenanceTag(tag);
  });
  const CreativeWorldLayoutBuildingBlockoutRecipe& recipe = provenance.recipe;
  const CreativeWorldLayoutBuildingBlockoutRequest& request = recipe.request;
  building.tags.push_back(std::string{kVersionPrefix} +
                          std::to_string(recipe.version));
  building.tags.push_back(std::string{kBaselinePrefix} +
                          hex64(provenance.instanceBaselineFingerprint));
  building.tags.push_back(std::string{kMinimumXPrefix} +
                          std::to_string(request.footprint.minimum.x));
  building.tags.push_back(std::string{kMinimumZPrefix} +
                          std::to_string(request.footprint.minimum.z));
  building.tags.push_back(std::string{kMaximumXPrefix} +
                          std::to_string(request.footprint.maximum.x));
  building.tags.push_back(std::string{kMaximumZPrefix} +
                          std::to_string(request.footprint.maximum.z));
  building.tags.push_back(std::string{kPatternPrefix} +
                          enumValue(request.pattern));
  building.tags.push_back(std::string{kWallThicknessPrefix} +
                          encodeDouble(request.wallThicknessCells));
  building.tags.push_back(std::string{kConnectRoomsPrefix} +
                          (request.connectRooms ? "1" : "0"));
  building.tags.push_back(std::string{kIncludeEntrancePrefix} +
                          (request.facade.includeEntrance ? "1" : "0"));
  building.tags.push_back(std::string{kEntranceEdgePrefix} +
                          enumValue(request.facade.entranceEdge));
  building.tags.push_back(std::string{kEntranceOffsetPrefix} +
                          encodeDouble(request.facade.entranceOffsetCells));
  building.tags.push_back(std::string{kIncludeWindowsPrefix} +
                          (request.facade.includeExteriorWindows ? "1" : "0"));
  building.tags.push_back(std::string{kWallHeightPrefix} +
                          std::to_string(request.wallHeightCells));
  building.tags.push_back(std::string{kStoreyCountPrefix} +
                          std::to_string(request.storeys.count));
  building.tags.push_back(std::string{kConnectStoreysPrefix} +
                          (request.storeys.connectStoreys ? "1" : "0"));
  building.tags.push_back(std::string{kConnectorKindPrefix} +
                          enumValue(request.storeys.connectorKind));
  building.tags.push_back(std::string{kPreferredDirectionPrefix} +
                          enumValue(request.storeys.preferredDirection));
  building.tags.push_back(std::string{kFloorTopPrefix} +
                          encodeDouble(recipe.floorTopLayer));
  building.tags.push_back(std::string{kFloorThicknessPrefix} +
                          std::to_string(recipe.floorThicknessLayers));
  building.tags.push_back(std::string{kRoofThicknessPrefix} +
                          std::to_string(recipe.roofThicknessLayers));
  building.tags.push_back(std::string{kRoofStylePrefix} +
                          enumValue(recipe.roofStyle));
  building.tags.push_back(std::string{kRoofRidgeAxisPrefix} +
                          enumValue(recipe.roofRidgeAxis));
  building.tags.push_back(std::string{kRoofPitchPrefix} +
                          encodeDouble(recipe.roofPitchDegrees));
  building.tags.push_back(std::string{kRoofOverhangPrefix} +
                          encodeDouble(recipe.roofOverhangCells));
  return true;
}

CreativeWorldLayoutBuildingBlockoutSyncReceipt
inspectCreativeWorldLayoutBuildingBlockoutSync(
    const CreativeWorldLayout& layout, std::size_t buildingIndex) {
  CreativeWorldLayoutBuildingBlockoutSyncReceipt result;
  result.requested = true;
  result.buildingIndex = buildingIndex;
  if (buildingIndex >= layout.buildings.size()) {
    result.state = CreativeWorldLayoutBuildingBlockoutSyncState::Invalid;
    result.reasonCode =
        "creative_world_layout_building_blockout_sync_building_invalid";
    return result;
  }
  result.provenance =
      creativeWorldLayoutBuildingBlockoutProvenance(layout, buildingIndex);
  if (!result.provenance.present) {
    result.accepted = true;
    result.state = CreativeWorldLayoutBuildingBlockoutSyncState::Unlinked;
    result.reasonCode = "creative_world_layout_building_blockout_sync_unlinked";
    return result;
  }
  result.instanceFingerprint =
      fingerprintCreativeWorldLayoutBuildingBlockout(layout, buildingIndex);
  if (!result.provenance.valid || !result.instanceFingerprint.valid) {
    result.accepted = true;
    result.state = CreativeWorldLayoutBuildingBlockoutSyncState::Invalid;
    result.reasonCode =
        "creative_world_layout_building_blockout_sync_provenance_invalid";
    return result;
  }
  result.accepted = true;
  if (result.instanceFingerprint.value ==
      result.provenance.instanceBaselineFingerprint) {
    result.state = CreativeWorldLayoutBuildingBlockoutSyncState::Current;
    result.reasonCode = "creative_world_layout_building_blockout_sync_current";
  } else {
    result.state =
        CreativeWorldLayoutBuildingBlockoutSyncState::LocallyModified;
    result.reasonCode =
        "creative_world_layout_building_blockout_sync_locally_modified";
  }
  return result;
}

}  // namespace iggy3d::creative
