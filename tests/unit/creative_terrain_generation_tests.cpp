#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/document/TerrainHeightField.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/recipes/TerrainGeneration.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <utility>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool exactPoint(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
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

bool authoredHeightFieldMutationIsUndoableAndClearable() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Authored Heightfield");
  static_cast<void>(document.assignId(712U));
  cr::CreativeAppState appState;
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      appState.facade.installDocument(std::move(document));
  const cr::CreativeTerrainGenerationResult generation =
      cr::buildCreativeTerrainGenerationPlan(testRecipe());
  const std::uint64_t revisionBefore = appState.facade.document().revision();

  cr::CreativeDocumentHistoryTransaction transaction =
      cr::beginCreativeHistoryTransaction(appState.facade,
                                          "terrain_generation_apply");
  const cr::CreativeTerrainHeightFieldReplaceReceipt applied =
      appState.facade.replaceTerrainHeightField(
          generation.plan.heightField.bounds(),
          generation.plan.heightField.heights());
  const cr::CreativeHistoryRecordReceipt recorded =
      cr::commitCreativeHistoryTransaction(
          appState.history, std::move(transaction), appState.facade);
  const std::uint64_t revisionAfterApply =
      appState.facade.document().revision();
  const cr::CreativeTerrainHeightFieldReplaceReceipt repeated =
      appState.facade.replaceTerrainHeightField(
          generation.plan.heightField.bounds(),
          generation.plan.heightField.heights());
  const bool repeatedPreserved =
      appState.facade.document().revision() == revisionAfterApply &&
      cr::creativeUndoDepth(appState.history) == 1U;

  constexpr std::array<std::uint16_t, 1U> invalidHeight{65U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt rejected =
      appState.facade.replaceTerrainHeightField({{0, 0}, 1U, 1U},
                                                invalidHeight);
  const bool rejectionPreserved =
      appState.facade.document().revision() == revisionAfterApply &&
      appState.facade.document().terrainHeightField().bounds() ==
          generation.plan.heightField.bounds();
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const bool undoCleared =
      appState.facade.document().terrainHeightField().cellCount() == 0U;
  const cr::CreativeHistoryApplyReceipt redone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Redo);
  const cr::CreativeTerrainHeightField& restored =
      appState.facade.document().terrainHeightField();
  const bool redoExact =
      restored.bounds() == generation.plan.heightField.bounds() &&
      std::equal(restored.heights().begin(), restored.heights().end(),
                 generation.plan.heightField.heights().begin(),
                 generation.plan.heightField.heights().end());
  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &appState.facade.document();
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const bool directBakeIncludesTerrain =
      baked.receipt.accepted && baked.receipt.usedSmoothTerrainCollision &&
      baked.receipt.bakedTerrainSurfacePatchCount ==
          generation.plan.heightField.presentCellCount();
  const std::uint64_t revisionBeforeClear =
      appState.facade.document().revision();
  const cr::CreativeTerrainHeightFieldReplaceReceipt cleared =
      appState.facade.replaceTerrainHeightField({}, {});
  const cr::CreativeTerrainHeightFieldReplaceReceipt clearRepeated =
      appState.facade.replaceTerrainHeightField({}, {});

  return expect(installed.accepted && generation.receipt.accepted,
                "heightfield history fixture is valid") &&
         expect(applied.accepted && applied.changed && recorded.recorded &&
                    revisionAfterApply == revisionBefore + 1U,
                "heightfield apply is one document mutation and undo record") &&
         expect(repeated.accepted && !repeated.changed && repeatedPreserved,
                "identical heightfield replacement is a document no-op") &&
         expect(!rejected.accepted && !rejected.changed && rejectionPreserved,
                "invalid replacement leaves document state unchanged") &&
         expect(undone.accepted && undoCleared && redone.accepted && redoExact,
                "document history restores authored terrain exactly") &&
         expect(directBakeIncludesTerrain,
                "direct room bake consumes canonical authored terrain") &&
         expect(cleared.accepted && cleared.changed &&
                    appState.facade.document().terrainHeightField().cellCount() ==
                        0U &&
                    appState.facade.document().revision() ==
                        revisionBeforeClear + 1U &&
                    clearRepeated.accepted && !clearRepeated.changed,
                "empty replacement clears authored terrain monotonically");
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

const cr::CreativeTerrainSurfacePatch* findPatch(
    const cr::CreativeTerrainRenderPlan& plan,
    cr::CreativeTerrainCoord2 coord) {
  const auto found = std::find_if(
      plan.patches.begin(), plan.patches.end(),
      [coord](const cr::CreativeTerrainSurfacePatch& patch) {
        return patch.coord == coord;
      });
  return found == plan.patches.end() ? nullptr : &*found;
}

bool heightSurfaceCompositionReplacesRegionAndSharesCorners() {
  cr::CreativeTerrainHeightField baseField;
  constexpr cr::CreativeTerrainHeightFieldBounds baseBounds{
      {-2, 0}, 6U, 2U};
  constexpr std::array<std::uint16_t, 12U> baseHeights{
      3U, 3U, 3U, 3U, 3U, 3U,
      3U, 3U, 3U, 3U, 3U, 3U,
  };
  static_cast<void>(baseField.replace(baseBounds, baseHeights));
  const cr::CreativeTerrainSurfacePlan base =
      cr::buildCreativeTerrainHeightSurfacePlan(baseField);

  cr::CreativeTerrainHeightField replacement;
  constexpr cr::CreativeTerrainHeightFieldBounds replacementBounds{
      {0, 0}, 3U, 2U};
  constexpr std::array<std::uint16_t, 6U> replacementHeights{
      7U, 0U, 7U,
      7U, 7U, 7U,
  };
  static_cast<void>(replacement.replace(replacementBounds,
                                        replacementHeights));
  const cr::CreativeTerrainSurfacePlan composed =
      cr::replaceCreativeTerrainSurfaceRegion(base, replacement);
  const cr::CreativeTerrainRenderPlan render =
      cr::buildCreativeTerrainRenderPlan(composed, {}, 1.0);
  const cr::CreativeTerrainSurfacePatch* left = findPatch(render, {-1, 1});
  const cr::CreativeTerrainSurfacePatch* right = findPatch(render, {0, 1});
  const bool holeAbsent =
      std::none_of(composed.columns.begin(), composed.columns.end(),
                   [](const cr::CreativeTerrainColumn& column) {
                     return column.coord == cr::CreativeTerrainCoord2{1, 0};
                   });

  return expect(base.accepted && base.columns.size() == 12U &&
                    base.cuboids.size() == 2U,
                "dense heightfield converts to canonical row cuboids") &&
         expect(composed.accepted && composed.columns.size() == 11U &&
                    composed.cuboids.size() == 7U && holeAbsent,
                "replacement region preserves outside and removes zero cells") &&
         expect(composed.columns.front().coord ==
                        cr::CreativeTerrainCoord2{-2, 0} &&
                    composed.columns.back().coord ==
                        cr::CreativeTerrainCoord2{3, 1},
                "composed columns remain ordered by row then coordinate") &&
         expect(render.accepted && render.patches.size() == 11U &&
                    left != nullptr && right != nullptr &&
                    exactPoint(left->corners[1], right->corners[0]) &&
                    exactPoint(left->corners[2], right->corners[3]),
                "base and replacement share exact boundary vertices") &&
         expect(baseField.revision() == 1U && replacement.revision() == 1U,
                "surface composition mutates neither source field");
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

bool biomeIntentProducesExplicitDeterministicMaterialsAndBoundedWork() {
  cr::CreativeTerrainGeneratorRecipe recipe = testRecipe();
  recipe.bounds = {{0, 0}, 128U, 64U};
  recipe.baseHeightCells = 12U;
  recipe.reliefCells = 0U;
  recipe.octaveCount = cr::kCreativeTerrainGeneratorMaximumOctaves;
  cr::applyCreativeTerrainBiomeIntent(
      recipe, cr::CreativeTerrainBiomeIntent::Arid);
  recipe.materialTransitionHeightCells = 13U;
  const cr::CreativeTerrainGenerationResult first =
      cr::buildCreativeTerrainGenerationPlan(recipe);
  const cr::CreativeTerrainGenerationResult second =
      cr::buildCreativeTerrainGenerationPlan(recipe);
  cr::CreativeTerrainGeneratorRecipe heightOnlyRecipe = recipe;
  heightOnlyRecipe.paintMaterials = false;
  const cr::CreativeTerrainGenerationResult heightOnly =
      cr::buildCreativeTerrainGenerationPlan(heightOnlyRecipe);

  cr::applyCreativeTerrainBiomeIntent(
      recipe, cr::CreativeTerrainBiomeIntent::Alpine);
  const bool alpineExplicit =
      recipe.biomeIntent == cr::CreativeTerrainBiomeIntent::Alpine &&
      recipe.lowlandMaterial == cr::CreativeTerrainMaterial::Dirt &&
      recipe.highlandMaterial == cr::CreativeTerrainMaterial::Stone;
  recipe.lowlandMaterial = cr::CreativeTerrainMaterial::Sand;
  cr::applyCreativeTerrainBiomeIntent(
      recipe, cr::CreativeTerrainBiomeIntent::Custom);

  return expect(first.receipt.accepted && second.receipt.accepted,
                "maximum bounded biome generation is accepted") &&
         expect(first.receipt.generatedCellCount == 8192U &&
                    first.receipt.evaluatedOctaveCount == 8192U * 8U,
                "generation receipt exposes the exact bounded octave work") &&
         expect(first.receipt.generatedMaterialOverrideCount == 8192U &&
                    first.plan.materialField.overrideCount() == 8192U &&
                    first.plan.materialField.materialAt({0, 0}) ==
                        cr::CreativeTerrainMaterial::Sand,
                "arid lowland intent materializes explicit sand output") &&
         expect(first.receipt.heightHash == second.receipt.heightHash &&
                    first.receipt.materialHash == second.receipt.materialHash &&
                    cr::creativeTerrainMaterialFieldsEqual(
                        first.plan.materialField,
                        second.plan.materialField),
                "biome height and material outputs are deterministic") &&
         expect(heightOnly.receipt.accepted &&
                    heightOnly.receipt.heightHash == first.receipt.heightHash &&
                    heightOnly.receipt.generatedMaterialOverrideCount == 0U &&
                    heightOnly.plan.materialField.overrideCount() == 0U,
                "height-only generation preserves terrain shape without "
                "emitting material edits") &&
         expect(alpineExplicit &&
                    recipe.biomeIntent ==
                        cr::CreativeTerrainBiomeIntent::Custom &&
                    recipe.lowlandMaterial ==
                        cr::CreativeTerrainMaterial::Sand,
                "named presets write explicit fields and Custom preserves them") &&
         expect(cr::toString(cr::CreativeTerrainBiomeIntent::Wetland) ==
                        "Wetland" &&
                    [] {
                      cr::CreativeTerrainBiomeIntent parsed =
                          cr::CreativeTerrainBiomeIntent::Count;
                      return cr::parseCreativeTerrainBiomeIntent(
                                 "Temperate", parsed) &&
                             parsed ==
                                 cr::CreativeTerrainBiomeIntent::Temperate;
                    }(),
                "biome persistence text is total and stable");
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
  cr::CreativeTerrainGeneratorRecipe biome = testRecipe();
  biome.biomeIntent = cr::CreativeTerrainBiomeIntent::Count;
  cr::CreativeTerrainGeneratorRecipe material = testRecipe();
  material.highlandMaterial = cr::CreativeTerrainMaterial::Count;

  const cr::CreativeTerrainGenerationResult badVersion =
      cr::buildCreativeTerrainGenerationPlan(version);
  const cr::CreativeTerrainGenerationResult badKind =
      cr::buildCreativeTerrainGenerationPlan(kind);
  const cr::CreativeTerrainGenerationResult badBounds =
      cr::buildCreativeTerrainGenerationPlan(bounds);
  const cr::CreativeTerrainGenerationResult badParameters =
      cr::buildCreativeTerrainGenerationPlan(parameters);
  const cr::CreativeTerrainGenerationResult badBiome =
      cr::buildCreativeTerrainGenerationPlan(biome);
  const cr::CreativeTerrainGenerationResult badMaterial =
      cr::buildCreativeTerrainGenerationPlan(material);

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
         expect(!badBiome.receipt.accepted &&
                    !badMaterial.receipt.accepted &&
                    badBiome.receipt.status ==
                        cr::CreativeTerrainGenerationStatus::InvalidParameters &&
                    badMaterial.receipt.status ==
                        cr::CreativeTerrainGenerationStatus::InvalidParameters,
                "invalid biome and material values fail closed") &&
         expect(badVersion.plan.heightField.cellCount() == 0U &&
                    badKind.plan.heightField.cellCount() == 0U &&
                    badBounds.plan.heightField.cellCount() == 0U &&
                    badParameters.plan.heightField.cellCount() == 0U,
                "rejected recipes publish no partial terrain");
}

}  // namespace

int main() {
  const bool ok = heightFieldReplacementIsAtomicBoundedAndCanonical() &&
                  authoredHeightFieldMutationIsUndoableAndClearable() &&
                  flatRecipeProducesExactQuantizedBase() &&
                  heightSurfaceCompositionReplacesRegionAndSharesCorners() &&
                  generationIsDeterministicAndSeedSensitive() &&
                  slopeDampingSuppressesFineTerrainVariation() &&
                  biomeIntentProducesExplicitDeterministicMaterialsAndBoundedWork() &&
                  invalidRecipesFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
