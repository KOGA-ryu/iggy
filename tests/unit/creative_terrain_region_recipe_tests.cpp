#include "app/iggy3d/creative/recipes/TerrainRegionRecipe.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
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

cr::CreativeTerrainHeightField field(
    cr::CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  cr::CreativeTerrainHeightField result;
  (void)result.replace(bounds, heights);
  return result;
}

bool fieldsEqual(const cr::CreativeTerrainHeightField& lhs,
                 const cr::CreativeTerrainHeightField& rhs) {
  return lhs.bounds() == rhs.bounds() &&
         lhs.heights().size() == rhs.heights().size() &&
         std::equal(lhs.heights().begin(), lhs.heights().end(),
                    rhs.heights().begin());
}

bool closedContractAndSharedMaskArePinned() {
  cr::CreativeTerrainRegionRecipe recipe;
  recipe.bounds = {{-2, 3}, 5U, 5U};
  cr::CreativeTerrainRegionMode parsed = cr::CreativeTerrainRegionMode::Count;
  const std::uint32_t corner = cr::creativeTerrainCompositionMaskWeight(
      cr::CreativeTerrainCompositionMask::Ellipse, 0U, 0U, recipe.bounds,
      0U);
  const std::uint32_t center = cr::creativeTerrainCompositionMaskWeight(
      cr::CreativeTerrainCompositionMask::Ellipse, 2U, 2U, recipe.bounds,
      0U);
  return expect(cr::isValidCreativeTerrainRegionRecipe(recipe),
                "default region recipe is durable and valid") &&
         expect(cr::creativeTerrainRegionModeUsesAmount(
                    cr::CreativeTerrainRegionMode::Raise) &&
                    cr::creativeTerrainRegionModeUsesAmount(
                        cr::CreativeTerrainRegionMode::Smooth) &&
                    !cr::creativeTerrainRegionModeUsesAmount(
                        cr::CreativeTerrainRegionMode::Flatten),
                "amount ownership is explicit") &&
         expect(cr::creativeTerrainRegionModeUsesTargetHeight(
                    cr::CreativeTerrainRegionMode::Flatten) &&
                    cr::creativeTerrainRegionModeUsesTargetHeight(
                        cr::CreativeTerrainRegionMode::Noise) &&
                    cr::creativeTerrainRegionModeUsesNoise(
                        cr::CreativeTerrainRegionMode::Noise),
                "target and noise controls are explicit") &&
         expect(cr::parseCreativeTerrainRegionMode("Erase", parsed) &&
                    parsed == cr::CreativeTerrainRegionMode::Erase &&
                    cr::toString(cr::CreativeTerrainRegionMode::Noise) ==
                        "Noise" &&
                    !cr::parseCreativeTerrainRegionMode("noise", parsed),
                "region mode persistence labels are closed") &&
         expect(corner == 0U &&
                    center ==
                        cr::kCreativeTerrainCompositionMaximumMaskWeight,
                "ellipse membership uses the shared composition kernel");
}

bool additiveModesPreserveHolesAndClamp() {
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 4U, 1U};
  constexpr std::array<std::uint16_t, 4U> heights{{0U, 4U, 63U, 1U}};
  const cr::CreativeTerrainHeightField existing = field(bounds, heights);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(existing);
  cr::CreativeTerrainRegionRecipe recipe;
  recipe.bounds = bounds;
  recipe.amountCells = 4U;
  recipe.mode = cr::CreativeTerrainRegionMode::Raise;
  const cr::CreativeTerrainRegionRecipeResult raised =
      cr::buildCreativeTerrainRegionRecipe(existing, canonical, recipe);
  recipe.mode = cr::CreativeTerrainRegionMode::Lower;
  const cr::CreativeTerrainRegionRecipeResult lowered =
      cr::buildCreativeTerrainRegionRecipe(existing, canonical, recipe);

  constexpr std::array<std::uint16_t, 4U> expectedRaised{{0U, 8U, 64U, 5U}};
  constexpr std::array<std::uint16_t, 4U> expectedLowered{{0U, 1U, 59U, 1U}};
  return expect(raised.receipt.accepted &&
                    std::equal(raised.heightField.heights().begin(),
                               raised.heightField.heights().end(),
                               expectedRaised.begin()),
                "raise is additive, bounded, and does not fill holes") &&
         expect(lowered.receipt.accepted &&
                    std::equal(lowered.heightField.heights().begin(),
                               lowered.heightField.heights().end(),
                               expectedLowered.begin()),
                "lower is additive, bounded, and does not erase tiles");
}

bool flattenMasksAndFeatherMaterializeExactCandidate() {
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 5U, 5U};
  constexpr std::array<std::uint16_t, 25U> low{
      2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U,
      2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U,
  };
  const cr::CreativeTerrainHeightField existing = field(bounds, low);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(existing);
  cr::CreativeTerrainRegionRecipe recipe;
  recipe.bounds = bounds;
  recipe.mode = cr::CreativeTerrainRegionMode::Flatten;
  recipe.targetHeightCells = 10U;
  recipe.featherCells = 1U;
  const cr::CreativeTerrainRegionRecipeResult rectangle =
      cr::buildCreativeTerrainRegionRecipe(existing, canonical, recipe);
  recipe.mask = cr::CreativeTerrainCompositionMask::Ellipse;
  recipe.featherCells = 0U;
  const cr::CreativeTerrainRegionRecipeResult ellipse =
      cr::buildCreativeTerrainRegionRecipe(existing, canonical, recipe);

  return expect(rectangle.receipt.accepted &&
                    rectangle.heightField.heights()[0U] == 6U &&
                    rectangle.heightField.heights()[12U] == 10U &&
                    rectangle.receipt.featheredCellCount == 16U,
                "rectangle feather blends edges and preserves a full center") &&
         expect(ellipse.receipt.accepted &&
                    ellipse.heightField.heights()[0U] == 2U &&
                    ellipse.heightField.heights()[12U] == 10U &&
                    ellipse.receipt.maskedCellCount < 25U,
                "ellipse excludes corners without changing the output bounds");
}

bool smoothReadsOneImmutableSnapshot() {
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 3U, 1U};
  constexpr std::array<std::uint16_t, 3U> heights{{2U, 10U, 20U}};
  const cr::CreativeTerrainHeightField existing = field(bounds, heights);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(existing);
  cr::CreativeTerrainRegionRecipe recipe;
  recipe.bounds = bounds;
  recipe.mode = cr::CreativeTerrainRegionMode::Smooth;
  recipe.amountCells = 8U;
  const cr::CreativeTerrainRegionRecipeResult result =
      cr::buildCreativeTerrainRegionRecipe(existing, canonical, recipe);
  constexpr std::array<std::uint16_t, 3U> expected{{6U, 11U, 15U}};
  return expect(result.receipt.accepted &&
                    std::equal(result.heightField.heights().begin(),
                               result.heightField.heights().end(),
                               expected.begin()),
                "smooth samples every output from the same 3x3 source snapshot");
}

bool noiseAndEraseAreDeterministic() {
  cr::CreativeTerrainHeightField empty;
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(empty);
  cr::CreativeTerrainRegionRecipe recipe;
  recipe.bounds = {{-3, 5}, 8U, 8U};
  recipe.mode = cr::CreativeTerrainRegionMode::Noise;
  recipe.targetHeightCells = 12U;
  recipe.noiseReliefCells = 5U;
  recipe.noiseScaleCells = 6.0;
  recipe.seed = 77U;
  const cr::CreativeTerrainRegionRecipeResult first =
      cr::buildCreativeTerrainRegionRecipe(empty, canonical, recipe);
  const cr::CreativeTerrainRegionRecipeResult repeated =
      cr::buildCreativeTerrainRegionRecipe(empty, canonical, recipe);
  recipe.seed = 78U;
  const cr::CreativeTerrainRegionRecipeResult changedSeed =
      cr::buildCreativeTerrainRegionRecipe(empty, canonical, recipe);

  recipe.mode = cr::CreativeTerrainRegionMode::Erase;
  const cr::CreativeTerrainSurfacePlan noisyCanonical =
      cr::buildCreativeTerrainHeightSurfacePlan(first.heightField);
  const cr::CreativeTerrainRegionRecipeResult erased =
      cr::buildCreativeTerrainRegionRecipe(first.heightField, noisyCanonical,
                                           recipe);
  return expect(first.receipt.accepted && repeated.receipt.accepted &&
                    fieldsEqual(first.heightField, repeated.heightField) &&
                    first.receipt.heightHash == repeated.receipt.heightHash,
                "noise is exactly deterministic for one recipe") &&
         expect(changedSeed.receipt.accepted &&
                    changedSeed.receipt.heightHash != first.receipt.heightHash,
                "noise seed changes the authored candidate") &&
         expect(erased.receipt.accepted &&
                    std::all_of(erased.heightField.heights().begin(),
                                erased.heightField.heights().end(),
                                [](std::uint16_t height) {
                                  return height ==
                                         cr::kCreativeTerrainEmptyHeightCells;
                                }),
                "erase removes the entire full-weight region");
}

bool invalidAndOversizedRequestsFailAtomically() {
  constexpr cr::CreativeTerrainHeightFieldBounds oneCell{{0, 0}, 1U, 1U};
  constexpr std::array<std::uint16_t, 1U> height{{4U}};
  const cr::CreativeTerrainHeightField existing = field(oneCell, height);
  const cr::CreativeTerrainSurfacePlan canonical =
      cr::buildCreativeTerrainHeightSurfacePlan(existing);
  cr::CreativeTerrainRegionRecipe invalid;
  invalid.bounds = oneCell;
  invalid.amountCells = 0U;
  const cr::CreativeTerrainRegionRecipeResult invalidResult =
      cr::buildCreativeTerrainRegionRecipe(existing, canonical, invalid);

  cr::CreativeTerrainRegionRecipe oversized;
  oversized.bounds = {{10000, 10000}, 1U, 1U};
  const cr::CreativeTerrainRegionRecipeResult oversizedResult =
      cr::buildCreativeTerrainRegionRecipe(existing, canonical, oversized);
  return expect(!invalidResult.receipt.accepted &&
                    invalidResult.heightField.cellCount() == 0U &&
                    invalidResult.receipt.status ==
                        cr::CreativeTerrainRegionRecipeStatus::InvalidRecipe,
                "invalid recipe emits no partial candidate") &&
         expect(!oversizedResult.receipt.accepted &&
                    oversizedResult.heightField.cellCount() == 0U &&
                    oversizedResult.receipt.status ==
                        cr::CreativeTerrainRegionRecipeStatus::CapacityExceeded,
                "union capacity failure is atomic");
}

}  // namespace

int main() {
  bool ok = true;
  ok = closedContractAndSharedMaskArePinned() && ok;
  ok = additiveModesPreserveHolesAndClamp() && ok;
  ok = flattenMasksAndFeatherMaterializeExactCandidate() && ok;
  ok = smoothReadsOneImmutableSnapshot() && ok;
  ok = noiseAndEraseAreDeterministic() && ok;
  ok = invalidAndOversizedRequestsFailAtomically() && ok;
  return ok ? 0 : 1;
}
