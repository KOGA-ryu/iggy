#include "app/iggy3d/creative/tools/TerrainStamp.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr bool coordLess(CreativeTerrainCoord2 lhs,
                                       CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool validStampMode(CreativeTerrainStampMode mode) noexcept {
  return mode < CreativeTerrainStampMode::Count;
}

[[nodiscard]] bool validElevationMode(
    CreativeTerrainStampElevationMode mode) noexcept {
  return mode < CreativeTerrainStampElevationMode::Count;
}

[[nodiscard]] bool checkedCoord(std::int64_t x,
                                std::int64_t z,
                                CreativeTerrainCoord2& output) noexcept {
  constexpr std::int64_t minimum = std::numeric_limits<std::int32_t>::min();
  constexpr std::int64_t maximum = std::numeric_limits<std::int32_t>::max();
  if (x < minimum || x > maximum || z < minimum || z > maximum) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] bool checkedCellCount(std::uint64_t width,
                                    std::uint64_t depth,
                                    std::size_t& output) noexcept {
  if (width == 0U || depth == 0U ||
      width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max() ||
      width > kCreativeTerrainStampCellCapacity / depth) {
    return false;
  }
  output = static_cast<std::size_t>(width * depth);
  return true;
}

[[nodiscard]] std::optional<std::uint16_t> surfaceHeightAt(
    const CreativeTerrainSurfacePlan& surface,
    CreativeTerrainCoord2 coord) noexcept {
  const auto found = std::lower_bound(
      surface.columns.begin(), surface.columns.end(), coord,
      [](const CreativeTerrainColumn& column,
         CreativeTerrainCoord2 candidate) {
        return coordLess(column.coord, candidate);
      });
  if (found == surface.columns.end() || found->coord != coord) {
    return std::nullopt;
  }
  return found->heightCells;
}

struct TransformedLocalCoord {
  std::uint16_t x = 0U;
  std::uint16_t z = 0U;
};

[[nodiscard]] TransformedLocalCoord transformLocal(
    std::uint16_t x,
    std::uint16_t z,
    std::uint16_t widthCells,
    std::uint16_t depthCells,
    std::uint8_t quarterTurns,
    bool mirrorX,
    bool mirrorZ) noexcept {
  std::uint32_t localX = x;
  std::uint32_t localZ = z;
  if (mirrorX) {
    localX = static_cast<std::uint32_t>(widthCells - 1U) - localX;
  }
  if (mirrorZ) {
    localZ = static_cast<std::uint32_t>(depthCells - 1U) - localZ;
  }
  switch (quarterTurns) {
    case 0U:
      return {static_cast<std::uint16_t>(localX),
              static_cast<std::uint16_t>(localZ)};
    case 1U:
      return {static_cast<std::uint16_t>(depthCells - 1U - localZ),
              static_cast<std::uint16_t>(localX)};
    case 2U:
      return {static_cast<std::uint16_t>(widthCells - 1U - localX),
              static_cast<std::uint16_t>(depthCells - 1U - localZ)};
    case 3U:
      return {static_cast<std::uint16_t>(localZ),
              static_cast<std::uint16_t>(widthCells - 1U - localX)};
    default:
      return {};
  }
}

[[nodiscard]] bool sameHeightField(const CreativeTerrainHeightField& lhs,
                                   const CreativeTerrainHeightField& rhs) {
  return lhs.validateInvariants() && rhs.validateInvariants() &&
         lhs.bounds() == rhs.bounds() &&
         std::equal(lhs.heights().begin(), lhs.heights().end(),
                    rhs.heights().begin(), rhs.heights().end());
}

[[nodiscard]] bool sameMaterialField(
    const CreativeTerrainMaterialField& lhs,
    const CreativeTerrainMaterialField& rhs) {
  return lhs.validateInvariants() && rhs.validateInvariants() &&
         lhs.overrides().size() == rhs.overrides().size() &&
         std::equal(lhs.overrides().begin(), lhs.overrides().end(),
                    rhs.overrides().begin());
}

[[nodiscard]] bool validIdentity(std::string_view assetId,
                                 std::string_view label,
                                 std::uint64_t assetVersion) noexcept {
  return !assetId.empty() && assetId.size() <= kCreativeTerrainStampAssetIdCapacity &&
         !label.empty() && label.size() <= kCreativeTerrainStampLabelCapacity &&
         assetVersion != 0U;
}

}  // namespace

std::string_view toString(CreativeTerrainStampMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainStampMode::Merge: return "MERGE";
    case CreativeTerrainStampMode::Replace: return "REPLACE";
    case CreativeTerrainStampMode::Count: break;
  }
  return "INVALID";
}

bool parseCreativeTerrainStampMode(
    std::string_view value,
    CreativeTerrainStampMode& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeTerrainStampMode::Count);
       ++index) {
    const CreativeTerrainStampMode candidate =
        static_cast<CreativeTerrainStampMode>(index);
    if (toString(candidate) == value) {
      output = candidate;
      return true;
    }
  }
  return false;
}

std::string_view toString(CreativeTerrainStampElevationMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainStampElevationMode::Absolute: return "ABSOLUTE";
    case CreativeTerrainStampElevationMode::Surface: return "SURFACE";
    case CreativeTerrainStampElevationMode::Count: break;
  }
  return "INVALID";
}

bool parseCreativeTerrainStampElevationMode(
    std::string_view value,
    CreativeTerrainStampElevationMode& output) noexcept {
  for (std::uint8_t index = 0U;
       index <
       static_cast<std::uint8_t>(CreativeTerrainStampElevationMode::Count);
       ++index) {
    const CreativeTerrainStampElevationMode candidate =
        static_cast<CreativeTerrainStampElevationMode>(index);
    if (toString(candidate) == value) {
      output = candidate;
      return true;
    }
  }
  return false;
}

std::string_view toString(CreativeTerrainStampCopyStatus status) noexcept {
  switch (status) {
    case CreativeTerrainStampCopyStatus::NotRequested: return "NotRequested";
    case CreativeTerrainStampCopyStatus::InvalidSource: return "InvalidSource";
    case CreativeTerrainStampCopyStatus::InvalidBounds: return "InvalidBounds";
    case CreativeTerrainStampCopyStatus::InvalidIdentity:
      return "InvalidIdentity";
    case CreativeTerrainStampCopyStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainStampCopyStatus::EmptyRegion: return "EmptyRegion";
    case CreativeTerrainStampCopyStatus::Copied: return "Copied";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainStampPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainStampPlanStatus::NotRequested: return "NotRequested";
    case CreativeTerrainStampPlanStatus::InvalidStamp: return "InvalidStamp";
    case CreativeTerrainStampPlanStatus::InvalidDestination:
      return "InvalidDestination";
    case CreativeTerrainStampPlanStatus::InvalidRequest: return "InvalidRequest";
    case CreativeTerrainStampPlanStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeTerrainStampPlanStatus::HeightOutOfRange:
      return "HeightOutOfRange";
    case CreativeTerrainStampPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainStampPlanStatus::OutputRejected:
      return "OutputRejected";
    case CreativeTerrainStampPlanStatus::NoChange: return "NoChange";
    case CreativeTerrainStampPlanStatus::Ready: return "Ready";
  }
  return "Unknown";
}

std::uint64_t creativeTerrainStampContentSignature(
    const CreativeTerrainStamp& stamp) noexcept {
  StableHasher hasher;
  hasher.addU64(stamp.version);
  hasher.addU64(stamp.widthCells);
  hasher.addU64(stamp.depthCells);
  hasher.addU64(stamp.minimumHeightCells);
  for (std::size_t index = 0U; index < stamp.heights.size(); ++index) {
    hasher.addU64(stamp.heights[index]);
    if (index < stamp.materials.size()) {
      for (const std::uint8_t weight : stamp.materials[index]) {
        hasher.addU64(weight);
      }
    }
  }
  return hasher.value();
}

bool isValidCreativeTerrainStamp(const CreativeTerrainStamp& stamp) noexcept {
  std::size_t expectedCellCount = 0U;
  if (stamp.version != kCreativeTerrainStampVersion ||
      !validIdentity(stamp.assetId, stamp.label, stamp.assetVersion) ||
      !checkedCellCount(stamp.widthCells, stamp.depthCells, expectedCellCount) ||
      stamp.heights.size() != expectedCellCount ||
      stamp.materials.size() != expectedCellCount) {
    return false;
  }
  std::uint16_t minimumHeight = kCreativeTerrainMaximumHeightCells;
  bool present = false;
  for (std::size_t index = 0U; index < stamp.heights.size(); ++index) {
    const std::uint16_t height = stamp.heights[index];
    if (height > kCreativeTerrainMaximumHeightCells ||
        !isValidCreativeTerrainMaterialWeights(stamp.materials[index])) {
      return false;
    }
    if (height == kCreativeTerrainEmptyHeightCells) {
      if (stamp.materials[index] != creativeTerrainMaterialSolidWeights(
                                        CreativeTerrainMaterial::Grass)) {
        return false;
      }
      continue;
    }
    present = true;
    minimumHeight = std::min(minimumHeight, height);
  }
  return present && stamp.minimumHeightCells == minimumHeight &&
         stamp.contentSignature == creativeTerrainStampContentSignature(stamp);
}

bool isValidCreativeTerrainStampRecipe(
    const CreativeTerrainStampRecipe& recipe) noexcept {
  return recipe.version == kCreativeTerrainStampRecipeVersion &&
         isValidCreativeTerrainStamp(recipe.stamp) &&
         recipe.quarterTurns < 4U && validStampMode(recipe.mode) &&
         validElevationMode(recipe.elevationMode) &&
         recipe.manualHeightOffsetCells >=
             kCreativeTerrainStampMinimumHeightOffsetCells &&
         recipe.manualHeightOffsetCells <=
             kCreativeTerrainStampMaximumHeightOffsetCells;
}

bool creativeTerrainStampEmpty(const CreativeTerrainStamp& stamp) noexcept {
  return stamp.heights.empty();
}

void clearCreativeTerrainStamp(CreativeTerrainStamp& stamp) noexcept {
  stamp = {};
}

CreativeTerrainStampCopyReceipt copyCreativeTerrainRegionToStamp(
    std::uint64_t sourceDocumentId,
    std::uint64_t sourceRevision,
    const CreativeTerrainSurfacePlan& sourceSurface,
    const CreativeTerrainMaterialField& sourceMaterials,
    CreativeTerrainCoord2 minimumCoord,
    CreativeTerrainCoord2 maximumCoord,
    std::string_view assetId,
    std::string_view label,
    std::uint64_t assetVersion,
    CreativeTerrainStamp& outStamp) {
  CreativeTerrainStampCopyReceipt receipt;
  receipt.requested = true;
  receipt.minimumCoord = minimumCoord;
  receipt.maximumCoord = maximumCoord;
  if (!sourceSurface.accepted || !sourceMaterials.validateInvariants()) {
    receipt.status = CreativeTerrainStampCopyStatus::InvalidSource;
    receipt.reasonCode = "creative_terrain_stamp_source_invalid";
    return receipt;
  }
  if (!validIdentity(assetId, label, assetVersion)) {
    receipt.status = CreativeTerrainStampCopyStatus::InvalidIdentity;
    receipt.reasonCode = "creative_terrain_stamp_identity_invalid";
    return receipt;
  }
  const std::int64_t width = static_cast<std::int64_t>(maximumCoord.x) -
                                 minimumCoord.x +
                             1;
  const std::int64_t depth = static_cast<std::int64_t>(maximumCoord.z) -
                                 minimumCoord.z +
                             1;
  if (width <= 0 || depth <= 0) {
    receipt.status = CreativeTerrainStampCopyStatus::InvalidBounds;
    receipt.reasonCode = "creative_terrain_stamp_bounds_invalid";
    return receipt;
  }
  std::size_t cellCount = 0U;
  if (!checkedCellCount(static_cast<std::uint64_t>(width),
                        static_cast<std::uint64_t>(depth), cellCount)) {
    receipt.status = CreativeTerrainStampCopyStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_stamp_copy_capacity_exceeded";
    return receipt;
  }

  CreativeTerrainStamp staged;
  staged.assetId.assign(assetId);
  staged.label.assign(label);
  staged.assetVersion = assetVersion;
  staged.sourceDocumentId = sourceDocumentId;
  staged.sourceRevision = sourceRevision;
  staged.sourceMinimum = minimumCoord;
  staged.widthCells = static_cast<std::uint16_t>(width);
  staged.depthCells = static_cast<std::uint16_t>(depth);
  staged.heights.assign(cellCount, kCreativeTerrainEmptyHeightCells);
  staged.materials.assign(
      cellCount,
      creativeTerrainMaterialSolidWeights(CreativeTerrainMaterial::Grass));
  staged.minimumHeightCells = kCreativeTerrainMaximumHeightCells;
  for (std::uint16_t localZ = 0U; localZ < staged.depthCells; ++localZ) {
    for (std::uint16_t localX = 0U; localX < staged.widthCells; ++localX) {
      CreativeTerrainCoord2 coord{};
      if (!checkedCoord(static_cast<std::int64_t>(minimumCoord.x) + localX,
                        static_cast<std::int64_t>(minimumCoord.z) + localZ,
                        coord)) {
        receipt.status = CreativeTerrainStampCopyStatus::InvalidBounds;
        receipt.reasonCode = "creative_terrain_stamp_bounds_overflow";
        return receipt;
      }
      const std::optional<std::uint16_t> height =
          surfaceHeightAt(sourceSurface, coord);
      if (!height.has_value()) {
        continue;
      }
      const std::size_t index =
          static_cast<std::size_t>(localZ) * staged.widthCells + localX;
      staged.heights[index] = *height;
      staged.materials[index] = sourceMaterials.weightsAt(coord);
      staged.minimumHeightCells =
          std::min(staged.minimumHeightCells, *height);
      ++receipt.copiedPresentCellCount;
      if (staged.materials[index] != creativeTerrainMaterialSolidWeights(
                                        CreativeTerrainMaterial::Grass)) {
        ++receipt.copiedMaterialCellCount;
      }
    }
  }
  if (receipt.copiedPresentCellCount == 0U) {
    receipt.status = CreativeTerrainStampCopyStatus::EmptyRegion;
    receipt.reasonCode = "creative_terrain_stamp_region_empty";
    return receipt;
  }
  staged.contentSignature = creativeTerrainStampContentSignature(staged);
  receipt.accepted = true;
  receipt.status = CreativeTerrainStampCopyStatus::Copied;
  receipt.copiedCellCount = cellCount;
  receipt.minimumHeightCells = staged.minimumHeightCells;
  receipt.contentSignature = staged.contentSignature;
  receipt.reasonCode = "creative_terrain_stamp_copied";
  outStamp = std::move(staged);
  return receipt;
}

CreativeTerrainStampPlan buildCreativeTerrainStampPlan(
    const CreativeTerrainHeightField& destinationHeight,
    const CreativeTerrainMaterialField& destinationMaterial,
    const CreativeTerrainSurfacePlan& destinationSurface,
    const CreativeTerrainStampRequest& request) {
  CreativeTerrainStampPlan plan;
  plan.requested = true;
  plan.recipe = request;
  plan.targetMinimum = request.targetMinimum;
  if (!isValidCreativeTerrainStamp(request.stamp)) {
    plan.status = CreativeTerrainStampPlanStatus::InvalidStamp;
    plan.reasonCode = "creative_terrain_stamp_invalid";
    return plan;
  }
  if (!destinationHeight.validateInvariants() ||
      !destinationMaterial.validateInvariants() ||
      !destinationSurface.accepted) {
    plan.status = CreativeTerrainStampPlanStatus::InvalidDestination;
    plan.reasonCode = "creative_terrain_stamp_destination_invalid";
    return plan;
  }
  if (!isValidCreativeTerrainStampRecipe(request)) {
    plan.status = CreativeTerrainStampPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_stamp_request_invalid";
    return plan;
  }

  const bool swapsAxes = (request.quarterTurns % 2U) != 0U;
  plan.transformedWidthCells =
      swapsAxes ? request.stamp.depthCells : request.stamp.widthCells;
  plan.transformedDepthCells =
      swapsAxes ? request.stamp.widthCells : request.stamp.depthCells;
  CreativeTerrainCoord2 exclusiveMaximum{};
  if (!checkedCoord(static_cast<std::int64_t>(request.targetMinimum.x) +
                        plan.transformedWidthCells,
                    static_cast<std::int64_t>(request.targetMinimum.z) +
                        plan.transformedDepthCells,
                    exclusiveMaximum)) {
    plan.status = CreativeTerrainStampPlanStatus::CoordinateOverflow;
    plan.reasonCode = "creative_terrain_stamp_bounds_overflow";
    return plan;
  }
  plan.targetMaximum = {exclusiveMaximum.x - 1, exclusiveMaximum.z - 1};
  const std::optional<std::uint16_t> targetHeight =
      surfaceHeightAt(destinationSurface, request.targetMinimum);
  plan.targetSurfacePresent = targetHeight.has_value();
  plan.targetSurfaceHeightCells = targetHeight.value_or(0U);
  const std::int32_t surfaceOffset =
      request.elevationMode == CreativeTerrainStampElevationMode::Surface &&
              targetHeight.has_value()
          ? static_cast<std::int32_t>(*targetHeight) -
                request.stamp.minimumHeightCells
          : 0;
  plan.appliedHeightOffsetCells =
      surfaceOffset + request.manualHeightOffsetCells;

  std::int64_t minimumX = plan.targetMinimum.x;
  std::int64_t minimumZ = plan.targetMinimum.z;
  std::int64_t maximumX = plan.targetMaximum.x;
  std::int64_t maximumZ = plan.targetMaximum.z;
  const CreativeTerrainHeightFieldBounds existingBounds =
      destinationHeight.bounds();
  if (destinationHeight.cellCount() > 0U) {
    minimumX = std::min(minimumX,
                        static_cast<std::int64_t>(existingBounds.minimum.x));
    minimumZ = std::min(minimumZ,
                        static_cast<std::int64_t>(existingBounds.minimum.z));
    maximumX = std::max(
        maximumX,
        static_cast<std::int64_t>(existingBounds.minimum.x) +
            existingBounds.widthCells - 1);
    maximumZ = std::max(
        maximumZ,
        static_cast<std::int64_t>(existingBounds.minimum.z) +
            existingBounds.depthCells - 1);
  }
  for (const CreativeTerrainColumn& column : destinationSurface.columns) {
    minimumX = std::min(minimumX, static_cast<std::int64_t>(column.coord.x));
    minimumZ = std::min(minimumZ, static_cast<std::int64_t>(column.coord.z));
    maximumX = std::max(maximumX, static_cast<std::int64_t>(column.coord.x));
    maximumZ = std::max(maximumZ, static_cast<std::int64_t>(column.coord.z));
  }
  const std::uint64_t outputWidth =
      static_cast<std::uint64_t>(maximumX - minimumX + 1);
  const std::uint64_t outputDepth =
      static_cast<std::uint64_t>(maximumZ - minimumZ + 1);
  std::size_t outputCellCount = 0U;
  CreativeTerrainCoord2 outputMinimum{};
  if (!checkedCoord(minimumX, minimumZ, outputMinimum)) {
    plan.status = CreativeTerrainStampPlanStatus::CoordinateOverflow;
    plan.reasonCode = "creative_terrain_stamp_output_bounds_overflow";
    return plan;
  }
  if (!checkedCellCount(outputWidth, outputDepth, outputCellCount)) {
    plan.status = CreativeTerrainStampPlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_stamp_output_capacity_exceeded";
    return plan;
  }

  std::vector<std::uint16_t> heights(outputCellCount,
                                     kCreativeTerrainEmptyHeightCells);
  for (std::uint16_t localZ = 0U; localZ < outputDepth; ++localZ) {
    for (std::uint16_t localX = 0U; localX < outputWidth; ++localX) {
      const CreativeTerrainCoord2 coord{
          static_cast<std::int32_t>(minimumX + localX),
          static_cast<std::int32_t>(minimumZ + localZ)};
      const std::optional<std::uint16_t> authored =
          destinationHeight.heightAt(coord);
      const std::optional<std::uint16_t> canonical =
          surfaceHeightAt(destinationSurface, coord);
      heights[static_cast<std::size_t>(localZ) * outputWidth + localX] =
          authored.has_value() ? *authored : canonical.value_or(0U);
    }
  }

  std::vector<CreativeTerrainMaterialEdit> materialEdits;
  materialEdits.reserve(request.stamp.cellCount());
  for (std::uint16_t sourceZ = 0U; sourceZ < request.stamp.depthCells;
       ++sourceZ) {
    for (std::uint16_t sourceX = 0U; sourceX < request.stamp.widthCells;
         ++sourceX) {
      const std::size_t sourceIndex =
          static_cast<std::size_t>(sourceZ) * request.stamp.widthCells +
          sourceX;
      const std::uint16_t sourceHeight = request.stamp.heights[sourceIndex];
      if (sourceHeight == 0U && request.mode == CreativeTerrainStampMode::Merge) {
        continue;
      }
      const TransformedLocalCoord transformed = transformLocal(
          sourceX, sourceZ, request.stamp.widthCells,
          request.stamp.depthCells, request.quarterTurns, request.mirrorX,
          request.mirrorZ);
      CreativeTerrainCoord2 target{};
      if (!checkedCoord(static_cast<std::int64_t>(request.targetMinimum.x) +
                            transformed.x,
                        static_cast<std::int64_t>(request.targetMinimum.z) +
                            transformed.z,
                        target)) {
        plan.status = CreativeTerrainStampPlanStatus::CoordinateOverflow;
        plan.reasonCode = "creative_terrain_stamp_coordinate_overflow";
        return plan;
      }
      const std::size_t outputIndex =
          static_cast<std::size_t>(target.z - outputMinimum.z) * outputWidth +
          static_cast<std::size_t>(target.x - outputMinimum.x);
      std::uint16_t finalHeight = 0U;
      if (sourceHeight != 0U) {
        const std::int32_t shifted =
            static_cast<std::int32_t>(sourceHeight) +
            plan.appliedHeightOffsetCells;
        if (shifted < kCreativeTerrainMinimumHeightCells ||
            shifted > kCreativeTerrainMaximumHeightCells) {
          plan.status = CreativeTerrainStampPlanStatus::HeightOutOfRange;
          plan.reasonCode = "creative_terrain_stamp_height_out_of_range";
          return plan;
        }
        finalHeight = static_cast<std::uint16_t>(shifted);
        ++plan.presentStampCellCount;
      }
      if (heights[outputIndex] != finalHeight) {
        heights[outputIndex] = finalHeight;
        ++plan.changedHeightCellCount;
      }
      const CreativeTerrainMaterialWeights finalWeights =
          sourceHeight == 0U
              ? creativeTerrainMaterialSolidWeights(
                    CreativeTerrainMaterial::Grass)
              : request.stamp.materials[sourceIndex];
      if (destinationMaterial.weightsAt(target) != finalWeights) {
        materialEdits.push_back(
            makeCreativeTerrainMaterialWeightEdit(target, finalWeights));
        ++plan.changedMaterialCellCount;
      }
      ++plan.affectedCellCount;
    }
  }

  CreativeTerrainHeightField outputHeight;
  const CreativeTerrainHeightFieldBounds outputBounds{
      outputMinimum, static_cast<std::uint16_t>(outputWidth),
      static_cast<std::uint16_t>(outputDepth)};
  if (!outputHeight.replace(outputBounds, heights).accepted) {
    plan.status = CreativeTerrainStampPlanStatus::OutputRejected;
    plan.reasonCode = "creative_terrain_stamp_height_output_rejected";
    return plan;
  }
  CreativeTerrainMaterialField outputMaterial = destinationMaterial;
  if (!materialEdits.empty() && !outputMaterial.apply(materialEdits).accepted) {
    plan.status = CreativeTerrainStampPlanStatus::OutputRejected;
    plan.reasonCode = "creative_terrain_stamp_material_output_rejected";
    return plan;
  }
  plan.heightField = std::move(outputHeight);
  plan.materialField = std::move(outputMaterial);
  plan.accepted = true;
  if (sameHeightField(plan.heightField, destinationHeight) &&
      sameMaterialField(plan.materialField, destinationMaterial)) {
    plan.status = CreativeTerrainStampPlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_stamp_no_change";
    return plan;
  }
  plan.status = CreativeTerrainStampPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_stamp_ready";
  return plan;
}

}  // namespace iggy3d::creative
