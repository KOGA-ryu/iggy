#include "app/iggy3d/creative/recipes/TerrainLandform.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <span>
#include <vector>

namespace iggy3d::creative {
namespace {

template <typename Enum>
[[nodiscard]] constexpr bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) < static_cast<std::size_t>(count);
}

template <typename Enum, std::size_t Extent>
[[nodiscard]] bool parseNamed(
    std::string_view text,
    std::span<const Enum, Extent> values,
    Enum& output) noexcept {
  for (const Enum value : values) {
    if (text == toString(value)) {
      output = value;
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool hardEdgeLess(CreativeTerrainHardEdge lhs,
                                CreativeTerrainHardEdge rhs) noexcept {
  return lhs.first != rhs.first ? coordLess(lhs.first, rhs.first)
                                : coordLess(lhs.second, rhs.second);
}

[[nodiscard]] bool validSource(
    const CreativeTerrainSurfacePlan& source) noexcept {
  if (!source.accepted ||
      (source.status != CreativeTerrainSurfacePlanStatus::Empty &&
       source.status != CreativeTerrainSurfacePlanStatus::Ready)) {
    return false;
  }
  for (std::size_t index = 0U; index < source.columns.size(); ++index) {
    const CreativeTerrainColumn& column = source.columns[index];
    if (column.heightCells < kCreativeTerrainMinimumHeightCells ||
        column.heightCells > kCreativeTerrainMaximumHeightCells ||
        (index > 0U &&
         !coordLess(source.columns[index - 1U].coord, column.coord))) {
      return false;
    }
  }
  return source.status == CreativeTerrainSurfacePlanStatus::Empty
             ? source.columns.empty()
             : !source.columns.empty();
}

[[nodiscard]] std::uint16_t sourceHeightAt(
    const CreativeTerrainHeightField& existing,
    const CreativeTerrainSurfacePlan& canonical,
    CreativeTerrainCoord2 coord) noexcept {
  if (existing.contains(coord)) {
    return existing.heightAt(coord).value_or(kCreativeTerrainEmptyHeightCells);
  }
  const auto found = std::lower_bound(
      canonical.columns.begin(), canonical.columns.end(), coord,
      [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 candidate) {
        return coordLess(column.coord, candidate);
      });
  return found != canonical.columns.end() && found->coord == coord
             ? found->heightCells
             : kCreativeTerrainEmptyHeightCells;
}

[[nodiscard]] bool unionBounds(
    const CreativeTerrainHeightField& existing,
    CreativeTerrainHeightFieldBounds authored,
    CreativeTerrainHeightFieldBounds& output) noexcept {
  if (existing.cellCount() == 0U) {
    output = authored;
    return isValidCreativeTerrainHeightFieldBounds(output);
  }
  const CreativeTerrainHeightFieldBounds current = existing.bounds();
  const std::int64_t minimumX =
      std::min<std::int64_t>(current.minimum.x, authored.minimum.x);
  const std::int64_t minimumZ =
      std::min<std::int64_t>(current.minimum.z, authored.minimum.z);
  const std::int64_t maximumX = std::max(
      static_cast<std::int64_t>(current.minimum.x) + current.widthCells,
      static_cast<std::int64_t>(authored.minimum.x) + authored.widthCells);
  const std::int64_t maximumZ = std::max(
      static_cast<std::int64_t>(current.minimum.z) + current.depthCells,
      static_cast<std::int64_t>(authored.minimum.z) + authored.depthCells);
  const std::int64_t width = maximumX - minimumX;
  const std::int64_t depth = maximumZ - minimumZ;
  if (minimumX < std::numeric_limits<std::int32_t>::min() ||
      minimumX > std::numeric_limits<std::int32_t>::max() ||
      minimumZ < std::numeric_limits<std::int32_t>::min() ||
      minimumZ > std::numeric_limits<std::int32_t>::max() || width <= 0 ||
      depth <= 0 || width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max() ||
      static_cast<std::uint64_t>(width) *
              static_cast<std::uint64_t>(depth) >
          kCreativeTerrainHeightFieldCellCapacity) {
    return false;
  }
  output = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(depth)};
  return isValidCreativeTerrainHeightFieldBounds(output);
}

[[nodiscard]] bool localCoordinate(
    CreativeTerrainCoord2 coord,
    CreativeTerrainHeightFieldBounds bounds,
    std::uint16_t& x,
    std::uint16_t& z) noexcept {
  const std::int64_t localX =
      static_cast<std::int64_t>(coord.x) - bounds.minimum.x;
  const std::int64_t localZ =
      static_cast<std::int64_t>(coord.z) - bounds.minimum.z;
  if (localX < 0 || localZ < 0 || localX >= bounds.widthCells ||
      localZ >= bounds.depthCells) {
    return false;
  }
  x = static_cast<std::uint16_t>(localX);
  z = static_cast<std::uint16_t>(localZ);
  return true;
}

[[nodiscard]] std::uint16_t boundaryDistance(
    std::uint16_t x,
    std::uint16_t z,
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  return std::min({x, z,
                   static_cast<std::uint16_t>(bounds.widthCells - 1U - x),
                   static_cast<std::uint16_t>(bounds.depthCells - 1U - z)});
}

[[nodiscard]] std::uint32_t edgeWeight(
    const CreativeTerrainLandformRecipe& recipe,
    std::uint16_t distance) noexcept {
  constexpr std::uint32_t kFull = 65'535U;
  std::uint32_t weight = kFull;
  if (recipe.edge == CreativeTerrainLandformEdge::Slope) {
    weight = recipe.edgeWidthCells == 0U
                 ? kFull
                 : std::min<std::uint32_t>(
                       kFull,
                       static_cast<std::uint32_t>(distance) * kFull /
                           recipe.edgeWidthCells);
  }
  if (recipe.featherCells != 0U) {
    weight = std::min<std::uint32_t>(
        weight, std::min<std::uint32_t>(
                    kFull, static_cast<std::uint32_t>(distance) * kFull /
                               recipe.featherCells));
  }
  return weight;
}

[[nodiscard]] std::uint16_t directedIndex(
    const CreativeTerrainLandformRecipe& recipe,
    std::uint16_t x,
    std::uint16_t z,
    std::uint16_t& extent) noexcept {
  switch (recipe.direction) {
    case CreativeTerrainLandformDirection::PositiveX:
      extent = recipe.bounds.widthCells;
      return x;
    case CreativeTerrainLandformDirection::PositiveZ:
      extent = recipe.bounds.depthCells;
      return z;
    case CreativeTerrainLandformDirection::NegativeX:
      extent = recipe.bounds.widthCells;
      return static_cast<std::uint16_t>(extent - 1U - x);
    case CreativeTerrainLandformDirection::NegativeZ:
      extent = recipe.bounds.depthCells;
      return static_cast<std::uint16_t>(extent - 1U - z);
    case CreativeTerrainLandformDirection::Count:
      break;
  }
  extent = 0U;
  return 0U;
}

[[nodiscard]] std::uint16_t lerpHeight(std::uint16_t from,
                                       std::uint16_t to,
                                       std::uint32_t numerator,
                                       std::uint32_t denominator) noexcept {
  if (denominator == 0U || from == to) {
    return to;
  }
  const std::int64_t delta = static_cast<std::int64_t>(to) - from;
  const std::int64_t scaled =
      delta * numerator + (delta >= 0 ? denominator / 2U
                                      : -static_cast<std::int64_t>(denominator / 2U));
  return static_cast<std::uint16_t>(std::clamp<std::int64_t>(
      static_cast<std::int64_t>(from) + scaled / denominator,
      kCreativeTerrainMinimumHeightCells, kCreativeTerrainMaximumHeightCells));
}

[[nodiscard]] std::uint16_t authoredHeight(
    const CreativeTerrainLandformRecipe& recipe,
    std::uint16_t x,
    std::uint16_t z,
    bool& transition) noexcept {
  if (recipe.kind == CreativeTerrainLandformKind::Plateau) {
    transition = false;
    return recipe.targetHeightCells;
  }
  std::uint16_t extent = 0U;
  const std::uint16_t along = directedIndex(recipe, x, z, extent);
  if (recipe.kind == CreativeTerrainLandformKind::Terrace) {
    const std::uint32_t band = std::min<std::uint32_t>(
        recipe.terraceCount - 1U,
        static_cast<std::uint32_t>(along) * recipe.terraceCount /
            std::max<std::uint16_t>(1U, extent));
    transition = band > 0U &&
                 static_cast<std::uint32_t>(along) * recipe.terraceCount %
                         std::max<std::uint16_t>(1U, extent) <
                     recipe.terraceCount;
    return lerpHeight(recipe.baseHeightCells, recipe.targetHeightCells, band,
                      recipe.terraceCount - 1U);
  }

  const std::uint32_t doubled = static_cast<std::uint32_t>(along) * 2U + 1U;
  const std::uint32_t center = extent;
  if (recipe.edge == CreativeTerrainLandformEdge::Retaining ||
      recipe.edgeWidthCells == 0U) {
    transition =
        std::abs(static_cast<std::int32_t>(doubled) -
                 static_cast<std::int32_t>(center)) <= 1;
    return doubled < center ? recipe.baseHeightCells
                            : recipe.targetHeightCells;
  }
  const std::int32_t signedDistance = static_cast<std::int32_t>(doubled) -
                                      static_cast<std::int32_t>(center);
  const std::int32_t halfSpan =
      static_cast<std::int32_t>(recipe.edgeWidthCells) * 2;
  if (signedDistance <= -halfSpan) {
    transition = false;
    return recipe.baseHeightCells;
  }
  if (signedDistance >= halfSpan) {
    transition = false;
    return recipe.targetHeightCells;
  }
  transition = true;
  return lerpHeight(recipe.baseHeightCells, recipe.targetHeightCells,
                    static_cast<std::uint32_t>(signedDistance + halfSpan),
                    static_cast<std::uint32_t>(halfSpan * 2));
}

[[nodiscard]] std::int32_t weatheringOffset(
    const CreativeTerrainLandformRecipe& recipe,
    CreativeTerrainCoord2 coord) noexcept {
  if (recipe.erosion != CreativeTerrainLandformErosion::Weathered ||
      recipe.erosionReliefCells == 0U) {
    return 0;
  }
  std::uint64_t value = recipe.seed;
  value ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.x)) *
           0x9e3779b185ebca87ULL;
  value ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.z)) *
           0xc2b2ae3d27d4eb4fULL;
  value ^= value >> 30U;
  value *= 0xbf58476d1ce4e5b9ULL;
  value ^= value >> 27U;
  const std::uint32_t span = recipe.erosionReliefCells * 2U + 1U;
  return static_cast<std::int32_t>(value % span) -
         recipe.erosionReliefCells;
}

[[nodiscard]] std::uint16_t applyOffset(std::uint16_t value,
                                        std::int32_t offset) noexcept {
  return static_cast<std::uint16_t>(std::clamp<std::int32_t>(
      static_cast<std::int32_t>(value) + offset,
      kCreativeTerrainMinimumHeightCells, kCreativeTerrainMaximumHeightCells));
}

[[nodiscard]] std::uint16_t blendHeight(std::uint16_t source,
                                        std::uint16_t target,
                                        std::uint32_t weight) noexcept {
  constexpr std::uint64_t kFull = 65'535U;
  const std::uint64_t value =
      static_cast<std::uint64_t>(source) * (kFull - weight) +
      static_cast<std::uint64_t>(target) * weight + kFull / 2U;
  return static_cast<std::uint16_t>(value / kFull);
}

[[nodiscard]] bool internalEdgeIsHard(
    const CreativeTerrainLandformRecipe& recipe,
    std::uint16_t firstX,
    std::uint16_t firstZ,
    std::uint16_t secondX,
    std::uint16_t secondZ) noexcept {
  if (recipe.kind != CreativeTerrainLandformKind::Terrace &&
      (recipe.kind != CreativeTerrainLandformKind::Cliff ||
       recipe.edge != CreativeTerrainLandformEdge::Retaining)) {
    return false;
  }
  bool firstTransition = false;
  bool secondTransition = false;
  return authoredHeight(recipe, firstX, firstZ, firstTransition) !=
         authoredHeight(recipe, secondX, secondZ, secondTransition);
}

void appendHardEdgeIfDiscontinuous(
    CreativeTerrainLandformResult& result,
    const CreativeTerrainSurfacePlan& priorCanonical,
    CreativeTerrainCoord2 first,
    CreativeTerrainCoord2 second) {
  const std::uint16_t firstHeight =
      sourceHeightAt(result.heightField, priorCanonical, first);
  const std::uint16_t secondHeight =
      sourceHeightAt(result.heightField, priorCanonical, second);
  if (firstHeight == secondHeight ||
      (firstHeight == kCreativeTerrainEmptyHeightCells &&
       secondHeight == kCreativeTerrainEmptyHeightCells)) {
    return;
  }
  result.hardEdges.push_back(
      canonicalCreativeTerrainHardEdge(first, second));
}

[[nodiscard]] bool appendLandformHardEdges(
    CreativeTerrainLandformResult& result,
    const CreativeTerrainSurfacePlan& priorCanonical,
    const CreativeTerrainLandformRecipe& recipe) {
  const CreativeTerrainHeightFieldBounds bounds = recipe.bounds;
  for (std::uint16_t z = 0U; z < bounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < bounds.widthCells; ++x) {
      const CreativeTerrainCoord2 coord{
          bounds.minimum.x + static_cast<std::int32_t>(x),
          bounds.minimum.z + static_cast<std::int32_t>(z)};
      if (x + 1U < bounds.widthCells &&
          internalEdgeIsHard(recipe, x, z,
                             static_cast<std::uint16_t>(x + 1U), z)) {
        appendHardEdgeIfDiscontinuous(
            result, priorCanonical, coord,
            {static_cast<std::int32_t>(coord.x + 1), coord.z});
      }
      if (z + 1U < bounds.depthCells &&
          internalEdgeIsHard(recipe, x, z, x,
                             static_cast<std::uint16_t>(z + 1U))) {
        appendHardEdgeIfDiscontinuous(
            result, priorCanonical, coord,
            {coord.x, static_cast<std::int32_t>(coord.z + 1)});
      }
    }
  }

  if (recipe.edge == CreativeTerrainLandformEdge::Retaining) {
    const std::int64_t minimumX = bounds.minimum.x;
    const std::int64_t minimumZ = bounds.minimum.z;
    const std::int64_t maximumX = minimumX + bounds.widthCells - 1;
    const std::int64_t maximumZ = minimumZ + bounds.depthCells - 1;
    for (std::int64_t z = minimumZ; z <= maximumZ; ++z) {
      if (minimumX > std::numeric_limits<std::int32_t>::min()) {
        appendHardEdgeIfDiscontinuous(
            result, priorCanonical,
            {static_cast<std::int32_t>(minimumX), static_cast<std::int32_t>(z)},
            {static_cast<std::int32_t>(minimumX - 1),
             static_cast<std::int32_t>(z)});
      }
      if (maximumX < std::numeric_limits<std::int32_t>::max()) {
        appendHardEdgeIfDiscontinuous(
            result, priorCanonical,
            {static_cast<std::int32_t>(maximumX), static_cast<std::int32_t>(z)},
            {static_cast<std::int32_t>(maximumX + 1),
             static_cast<std::int32_t>(z)});
      }
    }
    for (std::int64_t x = minimumX; x <= maximumX; ++x) {
      if (minimumZ > std::numeric_limits<std::int32_t>::min()) {
        appendHardEdgeIfDiscontinuous(
            result, priorCanonical,
            {static_cast<std::int32_t>(x), static_cast<std::int32_t>(minimumZ)},
            {static_cast<std::int32_t>(x),
             static_cast<std::int32_t>(minimumZ - 1)});
      }
      if (maximumZ < std::numeric_limits<std::int32_t>::max()) {
        appendHardEdgeIfDiscontinuous(
            result, priorCanonical,
            {static_cast<std::int32_t>(x), static_cast<std::int32_t>(maximumZ)},
            {static_cast<std::int32_t>(x),
             static_cast<std::int32_t>(maximumZ + 1)});
      }
    }
  }

  std::sort(result.hardEdges.begin(), result.hardEdges.end(), hardEdgeLess);
  result.hardEdges.erase(
      std::unique(result.hardEdges.begin(), result.hardEdges.end()),
      result.hardEdges.end());
  return validateCreativeTerrainHardEdges(result.hardEdges);
}

[[nodiscard]] std::uint64_t hashHeights(
    CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) noexcept {
  StableHasher hasher;
  hasher.addI64(bounds.minimum.x);
  hasher.addI64(bounds.minimum.z);
  hasher.addU64(bounds.widthCells);
  hasher.addU64(bounds.depthCells);
  for (const std::uint16_t height : heights) {
    hasher.addU64(height);
  }
  return hasher.value();
}

}  // namespace

bool isValidCreativeTerrainLandformRecipe(
    const CreativeTerrainLandformRecipe& recipe) noexcept {
  const std::uint64_t cellCount =
      static_cast<std::uint64_t>(recipe.bounds.widthCells) *
      recipe.bounds.depthCells;
  const bool shaped = recipe.kind == CreativeTerrainLandformKind::Terrace ||
                      recipe.kind == CreativeTerrainLandformKind::Cliff;
  return recipe.version == kCreativeTerrainLandformRecipeVersion &&
         validEnum(recipe.kind, CreativeTerrainLandformKind::Count) &&
         isValidCreativeTerrainHeightFieldBounds(recipe.bounds) &&
         cellCount <= kCreativeTerrainHeightFieldCellCapacity &&
         recipe.baseHeightCells >= kCreativeTerrainMinimumHeightCells &&
         recipe.baseHeightCells <= kCreativeTerrainMaximumHeightCells &&
         recipe.targetHeightCells >= kCreativeTerrainMinimumHeightCells &&
         recipe.targetHeightCells <= kCreativeTerrainMaximumHeightCells &&
         (!shaped || recipe.baseHeightCells != recipe.targetHeightCells) &&
         recipe.terraceCount >= 2U &&
         recipe.terraceCount <= kCreativeTerrainLandformMaximumTerraceCount &&
         validEnum(recipe.direction, CreativeTerrainLandformDirection::Count) &&
         validEnum(recipe.edge, CreativeTerrainLandformEdge::Count) &&
         recipe.edgeWidthCells <= kCreativeTerrainLandformMaximumEdgeCells &&
         (recipe.edge == CreativeTerrainLandformEdge::Retaining ||
          recipe.edgeWidthCells > 0U) &&
         recipe.featherCells <= kCreativeTerrainLandformMaximumEdgeCells &&
         isValidCreativeTerrainMaterial(recipe.material) &&
         validEnum(recipe.erosion, CreativeTerrainLandformErosion::Count) &&
         recipe.erosionReliefCells <=
             kCreativeTerrainLandformMaximumErosionReliefCells &&
         (recipe.erosion == CreativeTerrainLandformErosion::Weathered ||
          recipe.erosionReliefCells == 0U);
}

std::string_view toString(CreativeTerrainLandformKind value) noexcept {
  constexpr std::array names{"PLATEAU", "TERRACE", "CLIFF"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

bool parseCreativeTerrainLandformKind(
    std::string_view text,
    CreativeTerrainLandformKind& output) noexcept {
  constexpr std::array values{CreativeTerrainLandformKind::Plateau,
                              CreativeTerrainLandformKind::Terrace,
                              CreativeTerrainLandformKind::Cliff};
  return parseNamed(text, std::span{values}, output);
}

std::string_view toString(CreativeTerrainLandformDirection value) noexcept {
  constexpr std::array names{"POSITIVE_X", "POSITIVE_Z", "NEGATIVE_X",
                             "NEGATIVE_Z"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

bool parseCreativeTerrainLandformDirection(
    std::string_view text,
    CreativeTerrainLandformDirection& output) noexcept {
  constexpr std::array values{
      CreativeTerrainLandformDirection::PositiveX,
      CreativeTerrainLandformDirection::PositiveZ,
      CreativeTerrainLandformDirection::NegativeX,
      CreativeTerrainLandformDirection::NegativeZ};
  return parseNamed(text, std::span{values}, output);
}

std::string_view toString(CreativeTerrainLandformEdge value) noexcept {
  constexpr std::array names{"SLOPE", "RETAINING"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

bool parseCreativeTerrainLandformEdge(
    std::string_view text,
    CreativeTerrainLandformEdge& output) noexcept {
  constexpr std::array values{CreativeTerrainLandformEdge::Slope,
                              CreativeTerrainLandformEdge::Retaining};
  return parseNamed(text, std::span{values}, output);
}

std::string_view toString(CreativeTerrainLandformErosion value) noexcept {
  constexpr std::array names{"CLEAN", "WEATHERED"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

bool parseCreativeTerrainLandformErosion(
    std::string_view text,
    CreativeTerrainLandformErosion& output) noexcept {
  constexpr std::array values{CreativeTerrainLandformErosion::Clean,
                              CreativeTerrainLandformErosion::Weathered};
  return parseNamed(text, std::span{values}, output);
}

std::string_view toString(CreativeTerrainLandformStatus value) noexcept {
  constexpr std::array names{"NOT_REQUESTED",      "UNSUPPORTED_VERSION",
                             "INVALID_RECIPE",     "INVALID_SOURCE",
                             "CAPACITY_EXCEEDED",  "OUTPUT_REJECTED",
                             "MATERIAL_REJECTED",  "READY"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

CreativeTerrainLandformResult buildCreativeTerrainLandform(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainMaterialField& existingMaterial,
    const CreativeTerrainLandformRecipe& recipe) {
  CreativeTerrainLandformResult result;
  result.recipe = recipe;
  CreativeTerrainLandformReceipt& receipt = result.receipt;
  receipt.requested = true;
  if (recipe.version != kCreativeTerrainLandformRecipeVersion) {
    receipt.status = CreativeTerrainLandformStatus::UnsupportedVersion;
    receipt.reasonCode = "creative_terrain_landform_version_unsupported";
    return result;
  }
  if (!isValidCreativeTerrainLandformRecipe(recipe)) {
    receipt.status = CreativeTerrainLandformStatus::InvalidRecipe;
    receipt.reasonCode = "creative_terrain_landform_recipe_invalid";
    return result;
  }
  if (!existingAuthored.validateInvariants() ||
      !existingMaterial.validateInvariants() || !validSource(canonicalSource)) {
    receipt.status = CreativeTerrainLandformStatus::InvalidSource;
    receipt.reasonCode = "creative_terrain_landform_source_invalid";
    return result;
  }

  CreativeTerrainHeightFieldBounds outputBounds;
  if (!unionBounds(existingAuthored, recipe.bounds, outputBounds)) {
    receipt.status = CreativeTerrainLandformStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_landform_capacity_exceeded";
    return result;
  }

  const std::uint64_t outputCount =
      static_cast<std::uint64_t>(outputBounds.widthCells) *
      outputBounds.depthCells;
  std::vector<std::uint16_t> heights;
  heights.reserve(static_cast<std::size_t>(outputCount));
  result.materialEdits.reserve(static_cast<std::size_t>(
      static_cast<std::uint64_t>(recipe.bounds.widthCells) *
      recipe.bounds.depthCells));
  for (std::uint16_t z = 0U; z < outputBounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < outputBounds.widthCells; ++x) {
      const CreativeTerrainCoord2 coord{
          outputBounds.minimum.x + static_cast<std::int32_t>(x),
          outputBounds.minimum.z + static_cast<std::int32_t>(z)};
      const std::uint16_t source =
          sourceHeightAt(existingAuthored, canonicalSource, coord);
      std::uint16_t output = source;
      std::uint16_t localX = 0U;
      std::uint16_t localZ = 0U;
      if (localCoordinate(coord, recipe.bounds, localX, localZ)) {
        ++receipt.evaluatedCellCount;
        bool shapeTransition = false;
        std::uint16_t target =
            authoredHeight(recipe, localX, localZ, shapeTransition);
        const std::uint16_t distance =
            boundaryDistance(localX, localZ, recipe.bounds);
        const std::uint32_t weight = edgeWeight(recipe, distance);
        const bool edgeTransition = weight < 65'535U;
        if (shapeTransition || edgeTransition) {
          ++receipt.transitionCellCount;
          if (recipe.erosion == CreativeTerrainLandformErosion::Weathered) {
            target = applyOffset(target, weatheringOffset(recipe, coord));
            ++receipt.weatheredCellCount;
          }
        }
        const std::uint16_t foundation =
            source == kCreativeTerrainEmptyHeightCells
                ? recipe.baseHeightCells
                : source;
        output = blendHeight(foundation, target, weight);
        if (recipe.paintSurface && output != kCreativeTerrainEmptyHeightCells &&
            existingMaterial.materialAt(coord) != recipe.material) {
          result.materialEdits.push_back(
              recipe.material == CreativeTerrainMaterial::Grass
                  ? CreativeTerrainMaterialEdit{
                        CreativeTerrainMaterialEditKind::Clear, coord,
                        CreativeTerrainMaterial::Grass, {}}
                  : CreativeTerrainMaterialEdit{
                        CreativeTerrainMaterialEditKind::Set, coord,
                        recipe.material, {}});
        }
      }
      receipt.modifiedCellCount += output != source ? 1U : 0U;
      heights.push_back(output);
    }
  }

  const CreativeTerrainHeightFieldReplaceReceipt replaced =
      result.heightField.replace(outputBounds, heights);
  if (!replaced.accepted) {
    result.materialEdits.clear();
    receipt.status = CreativeTerrainLandformStatus::OutputRejected;
    receipt.reasonCode = "creative_terrain_landform_output_rejected";
    return result;
  }
  if (!appendLandformHardEdges(result, canonicalSource, recipe)) {
    result.heightField.clear();
    result.materialEdits.clear();
    result.hardEdges.clear();
    receipt.status = CreativeTerrainLandformStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_landform_hard_edge_capacity_exceeded";
    return result;
  }
  if (result.materialEdits.size() >
      kCreativeTerrainMaterialOverrideCapacity) {
    result.heightField.clear();
    result.materialEdits.clear();
    result.hardEdges.clear();
    receipt.status = CreativeTerrainLandformStatus::MaterialRejected;
    receipt.reasonCode = "creative_terrain_landform_material_capacity_exceeded";
    return result;
  }
  receipt.accepted = true;
  receipt.status = CreativeTerrainLandformStatus::Ready;
  receipt.materialEditCount = result.materialEdits.size();
  receipt.hardEdgeCount = result.hardEdges.size();
  receipt.outputCellCount = outputCount;
  receipt.heightHash = hashHeights(outputBounds, heights);
  receipt.reasonCode = "creative_terrain_landform_ready";
  return result;
}

}  // namespace iggy3d::creative
