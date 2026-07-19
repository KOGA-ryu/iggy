#include "app/iggy3d/creative/document/TerrainHeightField.hpp"
#include "app/iggy3d/creative/recipes/TerrainGeneration.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainGeneratorRecipe testRecipe() {
  cr::CreativeTerrainGeneratorRecipe recipe;
  recipe.seed = 0x123456789abcdef0ULL;
  recipe.bounds = {{-16, 7}, 32U, 24U};
  recipe.baseHeightCells = 20U;
  recipe.reliefCells = 16U;
  recipe.horizontalScaleCells = 18.0;
  recipe.octaveCount = 6U;
  recipe.persistence = 0.52;
  recipe.lacunarity = 2.03;
  recipe.slopeDamping = 0.7;
  return recipe;
}

bool heightFieldReplacementIsAtomicBoundedAndCanonical() {
  cr::CreativeTerrainHeightField field;
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{-2, 3}, 3U, 2U};
  constexpr std::array<std::uint16_t, 6U> heights{1U, 2U, 0U, 4U, 5U, 6U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt applied =
      field.replace(bounds, heights);
  const cr::CreativeTerrainHeightFieldReplaceReceipt repeated =
      field.replace(bounds, heights);

  constexpr std::array<std::uint16_t, 2U> invalidHeights{1U, 65U};
  constexpr cr::CreativeTerrainHeightFieldBounds invalidHeightBounds{
      {0, 0}, 2U, 1U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt invalid =
      field.replace(invalidHeightBounds, invalidHeights);
  const cr::CreativeTerrainHeightFieldReplaceReceipt mismatched =
      field.replace(bounds, std::span{invalidHeights}.first(1U));
  constexpr cr::CreativeTerrainHeightFieldBounds capacityBounds{
      {0, 0}, 8193U, 1U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt capacity =
      field.replace(capacityBounds, {});
  constexpr cr::CreativeTerrainHeightFieldBounds overflowBounds{
      {std::numeric_limits<std::int32_t>::max(), 0}, 2U, 1U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt overflow =
      field.replace(overflowBounds, {});

  return expect(applied.accepted && applied.changed &&
                    applied.status ==
                        cr::CreativeTerrainHeightFieldReplaceStatus::Applied &&
                    field.revision() == 1U,
                "valid heightfield applies once") &&
         expect(field.validateInvariants() && field.cellCount() == 6U &&
                    field.presentCellCount() == 5U,
                "heightfield invariants and presence count hold") &&
         expect(field.heightAt({-2, 3}) == 1U &&
                    field.heightAt({0, 3}) == 0U &&
                    field.heightAt({-2, 4}) == 4U &&
                    !field.heightAt({1, 4}).has_value(),
                "row-major coordinate lookup preserves absent tiles") &&
         expect(repeated.accepted && !repeated.changed &&
                    repeated.status ==
                        cr::CreativeTerrainHeightFieldReplaceStatus::NoChange &&
                    field.revision() == 1U,
                "identical replacement is a semantic no-op") &&
         expect(!invalid.accepted && !mismatched.accepted &&
                    invalid.status ==
                        cr::CreativeTerrainHeightFieldReplaceStatus::
                            InvalidHeights &&
                    mismatched.status ==
                        cr::CreativeTerrainHeightFieldReplaceStatus::
                            InvalidHeights,
                "invalid heights and cell counts reject atomically") &&
         expect(!capacity.accepted && !overflow.accepted &&
                    capacity.status ==
                        cr::CreativeTerrainHeightFieldReplaceStatus::
                            InvalidBounds &&
                    overflow.status ==
                        cr::CreativeTerrainHeightFieldReplaceStatus::
                            InvalidBounds &&
                    field.bounds() == bounds && field.heights().size() == 6U,
                "capacity and coordinate overflow preserve the live field");
}

bool flatRecipeProducesExactQuantizedBase() {
  cr::CreativeTerrainGeneratorRecipe recipe = testRecipe();
  recipe.bounds = {{0, 0}, 8U, 6U};
  recipe.baseHeightCells = 7U;
  recipe.reliefCells = 0U;
  const cr::CreativeTerrainGenerationResult result =
      cr::buildCreativeTerrainGenerationPlan(recipe);

  return expect(result.receipt.accepted &&
                    result.receipt.status ==
                        cr::CreativeTerrainGenerationStatus::Ready,
                "flat generator plan is ready") &&
         expect(result.receipt.generatedCellCount == 48U &&
                    result.plan.heightField.cellCount() == 48U &&
                    result.plan.heightField.presentCellCount() == 48U,
                "generator emits every requested terrain tile") &&
         expect(result.receipt.minimumHeightCells == 7U &&
                    result.receipt.maximumHeightCells == 7U &&
                    std::all_of(result.plan.heightField.heights().begin(),
                                result.plan.heightField.heights().end(),
                                [](std::uint16_t height) {
                                  return height == 7U;
                                }),
                "zero relief quantizes to the exact base height") &&
         expect(result.receipt.heightHash != 0U &&
                    result.plan.recipe.seed == recipe.seed,
                "plan retains source recipe and stable output hash");
}

bool generationIsDeterministicAndSeedSensitive() {
  constexpr std::uint64_t expectedVersionOneHeightHash =
      6238936245176684470ULL;
  const cr::CreativeTerrainGeneratorRecipe recipe = testRecipe();
  const cr::CreativeTerrainGenerationResult first =
      cr::buildCreativeTerrainGenerationPlan(recipe);
  const cr::CreativeTerrainGenerationResult second =
      cr::buildCreativeTerrainGenerationPlan(recipe);
  cr::CreativeTerrainGeneratorRecipe differentRecipe = recipe;
  ++differentRecipe.seed;
  const cr::CreativeTerrainGenerationResult different =
      cr::buildCreativeTerrainGenerationPlan(differentRecipe);

  const bool exact =
      first.plan.heightField.heights().size() ==
          second.plan.heightField.heights().size() &&
      std::equal(first.plan.heightField.heights().begin(),
                 first.plan.heightField.heights().end(),
                 second.plan.heightField.heights().begin());
  const bool seedChanged =
      first.plan.heightField.heights().size() ==
          different.plan.heightField.heights().size() &&
      !std::equal(first.plan.heightField.heights().begin(),
                  first.plan.heightField.heights().end(),
                  different.plan.heightField.heights().begin());

  return expect(first.receipt.accepted && second.receipt.accepted && exact,
                "same recipe produces exact row-major heights") &&
         expect(first.receipt.heightHash == second.receipt.heightHash,
                "same recipe produces exact height hash") &&
         expect(first.receipt.heightHash == expectedVersionOneHeightHash,
                "version one recipe output remains pinned") &&
         expect(different.receipt.accepted && seedChanged &&
                    first.receipt.heightHash != different.receipt.heightHash,
                "seed changes the generated terrain") &&
         expect(first.receipt.minimumHeightCells >=
                        cr::kCreativeTerrainMinimumHeightCells &&
                    first.receipt.maximumHeightCells <=
                        cr::kCreativeTerrainMaximumHeightCells,
                "generated terrain remains inside quantized height bounds");
}

std::uint64_t totalNeighborVariation(
    const cr::CreativeTerrainHeightField& field) {
  const cr::CreativeTerrainHeightFieldBounds bounds = field.bounds();
  std::uint64_t variation = 0U;
  for (std::uint16_t z = 0U; z < bounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < bounds.widthCells; ++x) {
      const cr::CreativeTerrainCoord2 coord{
          bounds.minimum.x + static_cast<std::int32_t>(x),
          bounds.minimum.z + static_cast<std::int32_t>(z)};
      const std::uint16_t height = field.heightAt(coord).value();
      if (x + 1U < bounds.widthCells) {
        const std::uint16_t neighbor =
            field.heightAt({coord.x + 1, coord.z}).value();
        variation += height > neighbor ? height - neighbor : neighbor - height;
      }
      if (z + 1U < bounds.depthCells) {
        const std::uint16_t neighbor =
            field.heightAt({coord.x, coord.z + 1}).value();
        variation += height > neighbor ? height - neighbor : neighbor - height;
      }
    }
  }
  return variation;
}

bool slopeDampingSuppressesFineTerrainVariation() {
  cr::CreativeTerrainGeneratorRecipe undampedRecipe = testRecipe();
  undampedRecipe.bounds = {{0, 0}, 48U, 48U};
  undampedRecipe.slopeDamping = 0.0;
  cr::CreativeTerrainGeneratorRecipe dampedRecipe = undampedRecipe;
  dampedRecipe.slopeDamping = 1.5;
  const cr::CreativeTerrainGenerationResult undamped =
      cr::buildCreativeTerrainGenerationPlan(undampedRecipe);
  const cr::CreativeTerrainGenerationResult damped =
      cr::buildCreativeTerrainGenerationPlan(dampedRecipe);
  const std::uint64_t undampedVariation =
      totalNeighborVariation(undamped.plan.heightField);
  const std::uint64_t dampedVariation =
      totalNeighborVariation(damped.plan.heightField);

  return expect(undamped.receipt.accepted && damped.receipt.accepted,
                "damping comparison plans are ready") &&
         expect(undamped.receipt.dampedContributionCount == 0U &&
                    damped.receipt.dampedContributionCount > 0U,
                "receipt exposes active slope damping") &&
         expect(dampedVariation < undampedVariation,
                "slope damping reduces quantized neighbor variation") &&
         expect(damped.receipt.heightHash != undamped.receipt.heightHash,
                "slope damping changes generated terrain output");
}

bool invalidRecipesFailClosed() {
  cr::CreativeTerrainGeneratorRecipe version = testRecipe();
  ++version.version;
  cr::CreativeTerrainGeneratorRecipe kind = testRecipe();
  kind.kind = cr::CreativeTerrainGeneratorKind::Count;
  cr::CreativeTerrainGeneratorRecipe bounds = testRecipe();
  bounds.bounds = {{0, 0}, 8193U, 1U};
  cr::CreativeTerrainGeneratorRecipe parameters = testRecipe();
  parameters.horizontalScaleCells =
      std::numeric_limits<double>::quiet_NaN();

  const cr::CreativeTerrainGenerationResult badVersion =
      cr::buildCreativeTerrainGenerationPlan(version);
  const cr::CreativeTerrainGenerationResult badKind =
      cr::buildCreativeTerrainGenerationPlan(kind);
  const cr::CreativeTerrainGenerationResult badBounds =
      cr::buildCreativeTerrainGenerationPlan(bounds);
  const cr::CreativeTerrainGenerationResult badParameters =
      cr::buildCreativeTerrainGenerationPlan(parameters);

  return expect(!badVersion.receipt.accepted &&
                    badVersion.receipt.status ==
                        cr::CreativeTerrainGenerationStatus::
                            UnsupportedVersion,
                "unknown recipe version fails closed") &&
         expect(!badKind.receipt.accepted &&
                    badKind.receipt.status ==
                        cr::CreativeTerrainGenerationStatus::InvalidKind,
                "unknown generator kind fails closed") &&
         expect(!badBounds.receipt.accepted &&
                    badBounds.receipt.status ==
                        cr::CreativeTerrainGenerationStatus::InvalidBounds,
                "over-capacity generation bounds fail closed") &&
         expect(!badParameters.receipt.accepted &&
                    badParameters.receipt.status ==
                        cr::CreativeTerrainGenerationStatus::InvalidParameters,
                "non-finite parameters fail closed") &&
         expect(badVersion.plan.heightField.cellCount() == 0U &&
                    badKind.plan.heightField.cellCount() == 0U &&
                    badBounds.plan.heightField.cellCount() == 0U &&
                    badParameters.plan.heightField.cellCount() == 0U,
                "rejected recipes publish no partial terrain");
}

}  // namespace

int main() {
  const bool ok = heightFieldReplacementIsAtomicBoundedAndCanonical() &&
                  flatRecipeProducesExactQuantizedBase() &&
                  generationIsDeterministicAndSeedSensitive() &&
                  slopeDampingSuppressesFineTerrainVariation() &&
                  invalidRecipesFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
