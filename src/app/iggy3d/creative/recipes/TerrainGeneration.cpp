#include "app/iggy3d/creative/recipes/TerrainGeneration.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace iggy3d::creative {
namespace {

struct NoiseSample {
  double value = 0.0;
  double derivativeX = 0.0;
  double derivativeZ = 0.0;
};

[[nodiscard]] std::uint64_t mixBits(std::uint64_t value) noexcept {
  value ^= value >> 30U;
  value *= 0xbf58476d1ce4e5b9ULL;
  value ^= value >> 27U;
  value *= 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] double latticeValue(std::int64_t x,
                                  std::int64_t z,
                                  std::uint64_t seed) noexcept {
  std::uint64_t hash = mixBits(seed ^ 0x9e3779b97f4a7c15ULL);
  hash = mixBits(hash ^ mixBits(static_cast<std::uint64_t>(x)));
  hash = mixBits(hash ^ mixBits(static_cast<std::uint64_t>(z) +
                                0x632be59bd9b4e019ULL));
  constexpr double inverseMantissaRange =
      1.0 / static_cast<double>(1ULL << 53U);
  const double unit =
      static_cast<double>(hash >> 11U) * inverseMantissaRange;
  return unit * 2.0 - 1.0;
}

[[nodiscard]] double quintic(double value) noexcept {
  return value * value * value *
         (value * (value * 6.0 - 15.0) + 10.0);
}

[[nodiscard]] double quinticDerivative(double value) noexcept {
  const double oneMinus = 1.0 - value;
  return 30.0 * value * value * oneMinus * oneMinus;
}

[[nodiscard]] bool sampleValueNoise(double x,
                                    double z,
                                    std::uint64_t seed,
                                    NoiseSample& output) noexcept {
  if (!std::isfinite(x) || !std::isfinite(z)) {
    return false;
  }
  const double floorX = std::floor(x);
  const double floorZ = std::floor(z);
  // Keep a representable margin for the +1 lattice samples below. Valid
  // recipes stay many orders of magnitude inside this guard.
  constexpr double safeLatticeCoordinateLimit = 9.0e18;
  if (floorX < -safeLatticeCoordinateLimit ||
      floorX > safeLatticeCoordinateLimit ||
      floorZ < -safeLatticeCoordinateLimit ||
      floorZ > safeLatticeCoordinateLimit) {
    return false;
  }

  const std::int64_t cellX = static_cast<std::int64_t>(floorX);
  const std::int64_t cellZ = static_cast<std::int64_t>(floorZ);
  const double fractionX = x - floorX;
  const double fractionZ = z - floorZ;
  const double blendX = quintic(fractionX);
  const double blendZ = quintic(fractionZ);
  const double derivativeBlendX = quinticDerivative(fractionX);
  const double derivativeBlendZ = quinticDerivative(fractionZ);

  const double a = latticeValue(cellX, cellZ, seed);
  const double b = latticeValue(cellX + 1, cellZ, seed);
  const double c = latticeValue(cellX, cellZ + 1, seed);
  const double d = latticeValue(cellX + 1, cellZ + 1, seed);
  const double lower = a + (b - a) * blendX;
  const double upper = c + (d - c) * blendX;
  output.value = lower + (upper - lower) * blendZ;
  output.derivativeX =
      derivativeBlendX * ((b - a) + ((d - c) - (b - a)) * blendZ);
  output.derivativeZ = derivativeBlendZ * (upper - lower);
  return std::isfinite(output.value) && std::isfinite(output.derivativeX) &&
         std::isfinite(output.derivativeZ);
}

[[nodiscard]] bool evaluateSlopeDampedFbm(
    const CreativeTerrainGeneratorRecipe& recipe,
    double worldX,
    double worldZ,
    double& output,
    std::uint64_t& dampedContributionCount) noexcept {
  double sampleX = worldX / recipe.horizontalScaleCells;
  double sampleZ = worldZ / recipe.horizontalScaleCells;
  double amplitude = 1.0;
  double amplitudeSum = 0.0;
  double accumulatedDerivativeX = 0.0;
  double accumulatedDerivativeZ = 0.0;
  double value = 0.0;
  for (std::uint8_t octave = 0U; octave < recipe.octaveCount; ++octave) {
    NoiseSample sample;
    if (!sampleValueNoise(sampleX, sampleZ, recipe.seed, sample)) {
      return false;
    }
    accumulatedDerivativeX += sample.derivativeX;
    accumulatedDerivativeZ += sample.derivativeZ;
    const double gradientSquared =
        accumulatedDerivativeX * accumulatedDerivativeX +
        accumulatedDerivativeZ * accumulatedDerivativeZ;
    const double divisor = 1.0 + recipe.slopeDamping * gradientSquared;
    if (divisor > 1.0) {
      ++dampedContributionCount;
    }
    value += amplitude * sample.value / divisor;
    amplitudeSum += amplitude;
    amplitude *= recipe.persistence;

    const double rotatedX = 0.8 * sampleX - 0.6 * sampleZ;
    const double rotatedZ = 0.6 * sampleX + 0.8 * sampleZ;
    sampleX = rotatedX * recipe.lacunarity;
    sampleZ = rotatedZ * recipe.lacunarity;
  }
  if (!std::isfinite(value) || !std::isfinite(amplitudeSum) ||
      amplitudeSum <= 0.0) {
    return false;
  }
  output = value / amplitudeSum;
  return std::isfinite(output);
}

[[nodiscard]] std::uint64_t hashHeightField(
    const CreativeTerrainHeightField& field) noexcept {
  StableHasher hasher;
  const CreativeTerrainHeightFieldBounds bounds = field.bounds();
  hasher.addI64(bounds.minimum.x);
  hasher.addI64(bounds.minimum.z);
  hasher.addU64(bounds.widthCells);
  hasher.addU64(bounds.depthCells);
  for (const std::uint16_t height : field.heights()) {
    hasher.addU64(height);
  }
  return hasher.value();
}

[[nodiscard]] std::uint64_t hashMaterialField(
    const CreativeTerrainMaterialField& field) noexcept {
  StableHasher hasher;
  for (const CreativeTerrainMaterialOverride& value : field.overrides()) {
    hasher.addI64(value.coord.x);
    hasher.addI64(value.coord.z);
    for (const std::uint8_t weight : value.weights) {
      hasher.addU64(weight);
    }
  }
  return hasher.value();
}

[[nodiscard]] std::uint16_t defaultMaterialTransition(
    const CreativeTerrainGeneratorRecipe& recipe) noexcept {
  const std::uint32_t offset =
      std::max<std::uint32_t>(1U, recipe.reliefCells / 2U);
  return static_cast<std::uint16_t>(std::min<std::uint32_t>(
      kCreativeTerrainMaximumHeightCells,
      static_cast<std::uint32_t>(recipe.baseHeightCells) + offset));
}

}  // namespace

bool isValidCreativeTerrainGeneratorRecipe(
    const CreativeTerrainGeneratorRecipe& recipe) noexcept {
  return recipe.version == kCreativeTerrainGeneratorRecipeVersion &&
         recipe.kind == CreativeTerrainGeneratorKind::SlopeDampedFbm &&
         isValidCreativeTerrainHeightFieldBounds(recipe.bounds) &&
         recipe.baseHeightCells >= kCreativeTerrainMinimumHeightCells &&
         recipe.baseHeightCells <= kCreativeTerrainMaximumHeightCells &&
         recipe.reliefCells <= kCreativeTerrainMaximumHeightCells &&
         std::isfinite(recipe.horizontalScaleCells) &&
         recipe.horizontalScaleCells >=
             kCreativeTerrainGeneratorMinimumHorizontalScaleCells &&
         recipe.horizontalScaleCells <=
             kCreativeTerrainGeneratorMaximumHorizontalScaleCells &&
         recipe.octaveCount >= 1U &&
         recipe.octaveCount <= kCreativeTerrainGeneratorMaximumOctaves &&
         std::isfinite(recipe.persistence) &&
         recipe.persistence >= kCreativeTerrainGeneratorMinimumPersistence &&
         recipe.persistence <= kCreativeTerrainGeneratorMaximumPersistence &&
         std::isfinite(recipe.lacunarity) &&
         recipe.lacunarity >= kCreativeTerrainGeneratorMinimumLacunarity &&
         recipe.lacunarity <= kCreativeTerrainGeneratorMaximumLacunarity &&
         std::isfinite(recipe.slopeDamping) && recipe.slopeDamping >= 0.0 &&
         recipe.slopeDamping <=
             kCreativeTerrainGeneratorMaximumSlopeDamping &&
         recipe.biomeIntent < CreativeTerrainBiomeIntent::Count &&
         isValidCreativeTerrainMaterial(recipe.lowlandMaterial) &&
         isValidCreativeTerrainMaterial(recipe.highlandMaterial) &&
         recipe.materialTransitionHeightCells >=
             kCreativeTerrainMinimumHeightCells &&
         recipe.materialTransitionHeightCells <=
             kCreativeTerrainMaximumHeightCells;
}

std::string_view toString(CreativeTerrainGeneratorKind kind) noexcept {
  switch (kind) {
    case CreativeTerrainGeneratorKind::SlopeDampedFbm:
      return "SlopeDampedFbm";
    case CreativeTerrainGeneratorKind::Count:
      break;
  }
  return "Invalid";
}

bool parseCreativeTerrainGeneratorKind(
    std::string_view value,
    CreativeTerrainGeneratorKind& output) noexcept {
  if (value == "SlopeDampedFbm") {
    output = CreativeTerrainGeneratorKind::SlopeDampedFbm;
    return true;
  }
  return false;
}

std::string_view toString(CreativeTerrainBiomeIntent intent) noexcept {
  switch (intent) {
    case CreativeTerrainBiomeIntent::Temperate: return "Temperate";
    case CreativeTerrainBiomeIntent::Alpine: return "Alpine";
    case CreativeTerrainBiomeIntent::Arid: return "Arid";
    case CreativeTerrainBiomeIntent::Wetland: return "Wetland";
    case CreativeTerrainBiomeIntent::Custom: return "Custom";
    case CreativeTerrainBiomeIntent::Count: break;
  }
  return "Invalid";
}

bool parseCreativeTerrainBiomeIntent(
    std::string_view value,
    CreativeTerrainBiomeIntent& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeTerrainBiomeIntent::Count);
       ++index) {
    const auto candidate = static_cast<CreativeTerrainBiomeIntent>(index);
    if (value == toString(candidate)) {
      output = candidate;
      return true;
    }
  }
  return false;
}

void applyCreativeTerrainBiomeIntent(
    CreativeTerrainGeneratorRecipe& recipe,
    CreativeTerrainBiomeIntent intent) noexcept {
  recipe.biomeIntent = intent;
  switch (intent) {
    case CreativeTerrainBiomeIntent::Temperate:
      recipe.lowlandMaterial = CreativeTerrainMaterial::Grass;
      recipe.highlandMaterial = CreativeTerrainMaterial::Stone;
      break;
    case CreativeTerrainBiomeIntent::Alpine:
      recipe.lowlandMaterial = CreativeTerrainMaterial::Dirt;
      recipe.highlandMaterial = CreativeTerrainMaterial::Stone;
      break;
    case CreativeTerrainBiomeIntent::Arid:
      recipe.lowlandMaterial = CreativeTerrainMaterial::Sand;
      recipe.highlandMaterial = CreativeTerrainMaterial::Stone;
      break;
    case CreativeTerrainBiomeIntent::Wetland:
      recipe.lowlandMaterial = CreativeTerrainMaterial::Dirt;
      recipe.highlandMaterial = CreativeTerrainMaterial::Grass;
      break;
    case CreativeTerrainBiomeIntent::Custom:
    case CreativeTerrainBiomeIntent::Count:
      return;
  }
  recipe.materialTransitionHeightCells = defaultMaterialTransition(recipe);
}

std::string_view toString(CreativeTerrainGenerationStatus status) noexcept {
  switch (status) {
    case CreativeTerrainGenerationStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainGenerationStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeTerrainGenerationStatus::InvalidKind:
      return "InvalidKind";
    case CreativeTerrainGenerationStatus::InvalidBounds:
      return "InvalidBounds";
    case CreativeTerrainGenerationStatus::InvalidParameters:
      return "InvalidParameters";
    case CreativeTerrainGenerationStatus::EvaluationFailed:
      return "EvaluationFailed";
    case CreativeTerrainGenerationStatus::HeightFieldRejected:
      return "HeightFieldRejected";
    case CreativeTerrainGenerationStatus::MaterialFieldRejected:
      return "MaterialFieldRejected";
    case CreativeTerrainGenerationStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainGenerationResult buildCreativeTerrainGenerationPlan(
    const CreativeTerrainGeneratorRecipe& recipe) {
  CreativeTerrainGenerationResult result;
  result.receipt.requested = true;
  result.plan.recipe = recipe;
  if (recipe.version != kCreativeTerrainGeneratorRecipeVersion) {
    result.receipt.status =
        CreativeTerrainGenerationStatus::UnsupportedVersion;
    result.receipt.reasonCode =
        "creative_terrain_generation_version_unsupported";
    return result;
  }
  if (recipe.kind != CreativeTerrainGeneratorKind::SlopeDampedFbm) {
    result.receipt.status = CreativeTerrainGenerationStatus::InvalidKind;
    result.receipt.reasonCode = "creative_terrain_generation_kind_invalid";
    return result;
  }
  if (!isValidCreativeTerrainHeightFieldBounds(recipe.bounds)) {
    result.receipt.status = CreativeTerrainGenerationStatus::InvalidBounds;
    result.receipt.reasonCode = "creative_terrain_generation_bounds_invalid";
    return result;
  }
  if (!isValidCreativeTerrainGeneratorRecipe(recipe)) {
    result.receipt.status =
        CreativeTerrainGenerationStatus::InvalidParameters;
    result.receipt.reasonCode =
        "creative_terrain_generation_parameters_invalid";
    return result;
  }

  const std::size_t cellCount =
      static_cast<std::size_t>(recipe.bounds.widthCells) *
      recipe.bounds.depthCells;
  std::vector<std::uint16_t> heights;
  heights.reserve(cellCount);
  std::vector<CreativeTerrainMaterialEdit> materialEdits;
  materialEdits.reserve(cellCount);
  std::uint16_t minimumHeight = kCreativeTerrainMaximumHeightCells;
  std::uint16_t maximumHeight = kCreativeTerrainMinimumHeightCells;
  for (std::uint16_t z = 0U; z < recipe.bounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < recipe.bounds.widthCells; ++x) {
      const double worldX =
          static_cast<double>(recipe.bounds.minimum.x) +
          static_cast<double>(x) + 0.5;
      const double worldZ =
          static_cast<double>(recipe.bounds.minimum.z) +
          static_cast<double>(z) + 0.5;
      double noise = 0.0;
      if (!evaluateSlopeDampedFbm(
              recipe, worldX, worldZ, noise,
              result.receipt.dampedContributionCount)) {
        result.receipt.status =
            CreativeTerrainGenerationStatus::EvaluationFailed;
        result.receipt.reasonCode =
            "creative_terrain_generation_evaluation_failed";
        return result;
      }
      const double requestedHeight =
          static_cast<double>(recipe.baseHeightCells) +
          noise * static_cast<double>(recipe.reliefCells);
      const auto roundedHeight = static_cast<std::int64_t>(
          std::llround(requestedHeight));
      const std::uint16_t height = static_cast<std::uint16_t>(
          std::clamp<std::int64_t>(
              roundedHeight, kCreativeTerrainMinimumHeightCells,
              kCreativeTerrainMaximumHeightCells));
      heights.push_back(height);
      const CreativeTerrainMaterial material =
          height >= recipe.materialTransitionHeightCells
              ? recipe.highlandMaterial
              : recipe.lowlandMaterial;
      if (recipe.paintMaterials &&
          material != CreativeTerrainMaterial::Grass) {
        materialEdits.push_back(
            {CreativeTerrainMaterialEditKind::Set,
             {recipe.bounds.minimum.x + static_cast<std::int32_t>(x),
              recipe.bounds.minimum.z + static_cast<std::int32_t>(z)},
             material, {}});
      }
      minimumHeight = std::min(minimumHeight, height);
      maximumHeight = std::max(maximumHeight, height);
    }
  }

  const CreativeTerrainHeightFieldReplaceReceipt installed =
      result.plan.heightField.replace(recipe.bounds, heights);
  if (!installed.accepted || !result.plan.heightField.validateInvariants()) {
    result.plan.heightField.clear();
    result.receipt.status =
        CreativeTerrainGenerationStatus::HeightFieldRejected;
    result.receipt.reasonCode =
        "creative_terrain_generation_height_field_rejected";
    return result;
  }
  if (!materialEdits.empty()) {
    const CreativeTerrainMaterialMutationReceipt materialInstalled =
        result.plan.materialField.apply(materialEdits);
    if (!materialInstalled.accepted ||
        !result.plan.materialField.validateInvariants()) {
      result.plan.heightField.clear();
      result.plan.materialField.clear();
      result.receipt.status =
          CreativeTerrainGenerationStatus::MaterialFieldRejected;
      result.receipt.reasonCode =
          "creative_terrain_generation_material_field_rejected";
      return result;
    }
  }

  result.receipt.accepted = true;
  result.receipt.status = CreativeTerrainGenerationStatus::Ready;
  result.receipt.generatedCellCount = heights.size();
  result.receipt.evaluatedOctaveCount =
      result.receipt.generatedCellCount * recipe.octaveCount;
  result.receipt.generatedMaterialOverrideCount =
      result.plan.materialField.overrideCount();
  result.receipt.minimumHeightCells = minimumHeight;
  result.receipt.maximumHeightCells = maximumHeight;
  result.receipt.heightHash = hashHeightField(result.plan.heightField);
  result.receipt.materialHash = hashMaterialField(result.plan.materialField);
  result.receipt.reasonCode = "creative_terrain_generation_ready";
  return result;
}

}  // namespace iggy3d::creative
