#include "app/iggy3d/creative/recipes/TerrainComposition.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainHeightField makeField(
    cr::CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  cr::CreativeTerrainHeightField field;
  static_cast<void>(field.replace(bounds, heights));
  return field;
}

cr::CreativeTerrainGenerationResult makeGeneration(
    cr::CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  cr::CreativeTerrainGenerationResult result;
  result.plan.recipe.bounds = bounds;
  result.plan.heightField = makeField(bounds, heights);
  result.receipt.requested = true;
  result.receipt.accepted = true;
  result.receipt.status = cr::CreativeTerrainGenerationStatus::Ready;
  result.receipt.generatedCellCount = heights.size();
  result.receipt.reasonCode = "test_generation_ready";
  return result;
}

bool disjointRegionPreservesAuthoredAndMaterializesSourceGap() {
  constexpr cr::CreativeTerrainHeightFieldBounds existingBounds{
      {0, 0}, 2U, 2U};
  constexpr std::array<std::uint16_t, 4U> existingHeights{4U, 4U, 4U, 4U};
  const cr::CreativeTerrainHeightField existing =
      makeField(existingBounds, existingHeights);

  constexpr cr::CreativeTerrainHeightFieldBounds canonicalBounds{
      {0, 0}, 5U, 2U};
  constexpr std::array<std::uint16_t, 10U> canonicalHeights{
      4U, 4U, 6U, 0U, 0U,
      4U, 4U, 6U, 0U, 0U,
  };
  const cr::CreativeTerrainHeightField canonicalField =
      makeField(canonicalBounds, canonicalHeights);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(canonicalField);

  constexpr cr::CreativeTerrainHeightFieldBounds generatedBounds{
      {3, 0}, 2U, 2U};
  constexpr std::array<std::uint16_t, 4U> generatedHeights{10U, 10U, 10U,
                                                           10U};
  const cr::CreativeTerrainGenerationResult generation =
      makeGeneration(generatedBounds, generatedHeights);
  cr::CreativeTerrainCompositionRecipe recipe;
  recipe.featherCells = 0U;
  const cr::CreativeTerrainCompositionResult composed =
      cr::composeCreativeTerrainGeneration(existing, canonical, generation,
                                            recipe);
  constexpr std::array<std::uint16_t, 10U> expected{
      4U, 4U, 6U, 10U, 10U,
      4U, 4U, 6U, 10U, 10U,
  };

  return expect(composed.receipt.accepted &&
                    composed.heightField.bounds() == canonicalBounds,
                "disjoint composition expands to the authored/generated union") &&
         expect(std::equal(expected.begin(), expected.end(),
                           composed.heightField.heights().begin(),
                           composed.heightField.heights().end()),
                "existing authored cells and canonical gap terrain survive") &&
         expect(composed.receipt.boundsExpanded &&
                    composed.receipt.modifiedCellCount == 4U &&
                    composed.receipt.materializedSourceCellCount == 2U,
                "receipt exposes expansion, edits, and materialized fallback");
}

bool rectangleFeatherBlendsDeterministically() {
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{-2, -2}, 5U, 5U};
  std::array<std::uint16_t, 25U> sourceHeights{};
  sourceHeights.fill(4U);
  std::array<std::uint16_t, 25U> generatedHeights{};
  generatedHeights.fill(12U);
  const cr::CreativeTerrainHeightField existing =
      makeField(bounds, sourceHeights);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(existing);
  const cr::CreativeTerrainGenerationResult generation =
      makeGeneration(bounds, generatedHeights);
  cr::CreativeTerrainCompositionRecipe recipe;
  recipe.featherCells = 1U;
  const cr::CreativeTerrainCompositionResult first =
      cr::composeCreativeTerrainGeneration(existing, canonical, generation,
                                            recipe);
  const cr::CreativeTerrainCompositionResult second =
      cr::composeCreativeTerrainGeneration(existing, canonical, generation,
                                            recipe);
  const std::span<const std::uint16_t> heights = first.heightField.heights();

  return expect(first.receipt.accepted && heights.size() == 25U,
                "rectangle feather composes a complete field") &&
         expect(heights[0U] == 8U && heights[4U] == 8U &&
                    heights[20U] == 8U && heights[24U] == 8U &&
                    heights[6U] == 12U && heights[12U] == 12U,
                "outer ring is blended while the interior reaches target") &&
         expect(first.receipt.featheredCellCount == 16U &&
                    first.receipt.modifiedCellCount == 25U &&
                    first.receipt.heightHash == second.receipt.heightHash &&
                    std::equal(heights.begin(), heights.end(),
                               second.heightField.heights().begin(),
                               second.heightField.heights().end()),
                "feather output and hash are deterministic");
}

bool ellipseMaskLeavesCornersUntouched() {
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 5U, 5U};
  std::array<std::uint16_t, 25U> sourceHeights{};
  sourceHeights.fill(4U);
  std::array<std::uint16_t, 25U> generatedHeights{};
  generatedHeights.fill(12U);
  const cr::CreativeTerrainHeightField existing =
      makeField(bounds, sourceHeights);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(existing);
  const cr::CreativeTerrainGenerationResult generation =
      makeGeneration(bounds, generatedHeights);
  cr::CreativeTerrainCompositionRecipe recipe;
  recipe.mask = cr::CreativeTerrainCompositionMask::Ellipse;
  recipe.featherCells = 0U;
  const cr::CreativeTerrainCompositionResult composed =
      cr::composeCreativeTerrainGeneration(existing, canonical, generation,
                                            recipe);
  const std::span<const std::uint16_t> heights =
      composed.heightField.heights();

  return expect(composed.receipt.accepted && heights[0U] == 4U &&
                    heights[4U] == 4U && heights[20U] == 4U &&
                    heights[24U] == 4U,
                "ellipse mask preserves cells outside its footprint") &&
         expect(heights[12U] == 12U &&
                    composed.receipt.maskedCellCount < heights.size() &&
                    composed.receipt.preservedCellCount >= 4U,
                "ellipse center applies while corners remain source terrain");
}

bool raiseAndLowerAreMonotonic() {
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{-1, 3}, 3U, 1U};
  constexpr std::array<std::uint16_t, 3U> sourceHeights{8U, 8U, 8U};
  constexpr std::array<std::uint16_t, 3U> generatedHeights{4U, 8U, 12U};
  const cr::CreativeTerrainHeightField existing =
      makeField(bounds, sourceHeights);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(existing);
  const cr::CreativeTerrainGenerationResult generation =
      makeGeneration(bounds, generatedHeights);
  cr::CreativeTerrainCompositionRecipe recipe;
  recipe.featherCells = 0U;
  recipe.mode = cr::CreativeTerrainCompositionMode::Raise;
  const cr::CreativeTerrainCompositionResult raised =
      cr::composeCreativeTerrainGeneration(existing, canonical, generation,
                                            recipe);
  recipe.mode = cr::CreativeTerrainCompositionMode::Lower;
  const cr::CreativeTerrainCompositionResult lowered =
      cr::composeCreativeTerrainGeneration(existing, canonical, generation,
                                            recipe);
  constexpr std::array<std::uint16_t, 3U> expectedRaised{8U, 8U, 12U};
  constexpr std::array<std::uint16_t, 3U> expectedLowered{4U, 8U, 8U};

  return expect(raised.receipt.accepted && lowered.receipt.accepted,
                "raise and lower recipes compose") &&
         expect(std::equal(expectedRaised.begin(), expectedRaised.end(),
                           raised.heightField.heights().begin(),
                           raised.heightField.heights().end()),
                "raise never lowers source terrain") &&
         expect(std::equal(expectedLowered.begin(), expectedLowered.end(),
                           lowered.heightField.heights().begin(),
                           lowered.heightField.heights().end()),
                "lower never raises source terrain");
}

bool invalidInputsAndCapacityRejectAtomically() {
  constexpr cr::CreativeTerrainHeightFieldBounds existingBounds{
      {0, 0}, 1U, 1U};
  constexpr std::array<std::uint16_t, 1U> height{4U};
  const cr::CreativeTerrainHeightField existing =
      makeField(existingBounds, height);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(existing);
  constexpr cr::CreativeTerrainHeightFieldBounds farBounds{
      {8192, 0}, 1U, 1U};
  const cr::CreativeTerrainGenerationResult farGeneration =
      makeGeneration(farBounds, height);
  cr::CreativeTerrainCompositionRecipe recipe;
  const cr::CreativeTerrainCompositionResult capacity =
      cr::composeCreativeTerrainGeneration(existing, canonical, farGeneration,
                                            recipe);
  const cr::CreativeTerrainGenerationResult invalidGeneration;
  const cr::CreativeTerrainCompositionResult rejectedGeneration =
      cr::composeCreativeTerrainGeneration(existing, canonical,
                                            invalidGeneration, recipe);
  const cr::CreativeTerrainSurfacePlan invalidSource;
  const cr::CreativeTerrainCompositionResult rejectedSource =
      cr::composeCreativeTerrainGeneration(existing, invalidSource,
                                            farGeneration, recipe);
  recipe.version = cr::kCreativeTerrainCompositionRecipeVersion + 1U;
  const cr::CreativeTerrainCompositionResult invalidVersion =
      cr::composeCreativeTerrainGeneration(existing, canonical, farGeneration,
                                            recipe);
  recipe.version = cr::kCreativeTerrainCompositionRecipeVersion;
  recipe.featherCells =
      cr::kCreativeTerrainCompositionMaximumFeatherCells + 1U;
  const cr::CreativeTerrainCompositionResult invalidFeather =
      cr::composeCreativeTerrainGeneration(existing, canonical, farGeneration,
                                            recipe);
  recipe.featherCells = 0U;
  recipe.mask = cr::CreativeTerrainCompositionMask::Count;
  const cr::CreativeTerrainCompositionResult invalidMask =
      cr::composeCreativeTerrainGeneration(existing, canonical, farGeneration,
                                            recipe);
  recipe.mask = cr::CreativeTerrainCompositionMask::Rectangle;
  recipe.mode = cr::CreativeTerrainCompositionMode::Count;
  const cr::CreativeTerrainCompositionResult invalidMode =
      cr::composeCreativeTerrainGeneration(existing, canonical, farGeneration,
                                            recipe);

  return expect(!capacity.receipt.accepted &&
                    capacity.receipt.status ==
                        cr::CreativeTerrainCompositionStatus::CapacityExceeded &&
                    capacity.heightField.cellCount() == 0U,
                "oversized union rejects without partial output") &&
         expect(!invalidMask.receipt.accepted &&
                    invalidMask.receipt.status ==
                        cr::CreativeTerrainCompositionStatus::InvalidMask &&
                    !invalidMode.receipt.accepted &&
                    invalidMode.receipt.status ==
                        cr::CreativeTerrainCompositionStatus::InvalidMode,
                "invalid closed-enum values fail closed") &&
         expect(!rejectedGeneration.receipt.accepted &&
                    rejectedGeneration.receipt.status ==
                        cr::CreativeTerrainCompositionStatus::InvalidGeneration &&
                    !rejectedSource.receipt.accepted &&
                    rejectedSource.receipt.status ==
                        cr::CreativeTerrainCompositionStatus::InvalidSource,
                "invalid generation and source truth fail closed") &&
         expect(!invalidVersion.receipt.accepted &&
                    invalidVersion.receipt.status ==
                        cr::CreativeTerrainCompositionStatus::UnsupportedVersion &&
                    !invalidFeather.receipt.accepted &&
                    invalidFeather.receipt.status ==
                        cr::CreativeTerrainCompositionStatus::InvalidFeather,
                "unsupported versions and feather values fail closed");
}

}  // namespace

int main() {
  return disjointRegionPreservesAuthoredAndMaterializesSourceGap() &&
                 rectangleFeatherBlendsDeterministically() &&
                 ellipseMaskLeavesCornersUntouched() &&
                 raiseAndLowerAreMonotonic() &&
                 invalidInputsAndCapacityRejectAtomically()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
