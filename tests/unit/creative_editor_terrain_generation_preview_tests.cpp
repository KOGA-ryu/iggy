#include "EditorPreviewFrame.hpp"

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/recipes/TerrainGeneration.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainGenerationResult generationFor(std::uint64_t seed) {
  cr::CreativeTerrainGeneratorRecipe recipe;
  recipe.seed = seed;
  recipe.bounds = {{-1, -1}, 3U, 3U};
  recipe.baseHeightCells = 10U;
  recipe.reliefCells = 6U;
  recipe.horizontalScaleCells = 3.0;
  recipe.octaveCount = 5U;
  recipe.persistence = 0.5;
  recipe.lacunarity = 2.0;
  recipe.slopeDamping = 0.8;
  return cr::buildCreativeTerrainGenerationPlan(recipe);
}

bool generatedPreviewReplacesTerrainAndCachesByHeightHash() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Generated Terrain Preview");
  static_cast<void>(document.assignId(901U));
  constexpr std::array controls{
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 1U}},
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{10, 0}, 4U, 1U}},
  };
  const cr::CreativeTerrainMutationReceipt terrainApplied =
      document.applyTerrainControlEdits(controls);
  const std::uint64_t documentRevision = document.revision();

  app::CreativeEditorSceneCache sourceCache;
  const bool sourceBuilt =
      app::refreshCreativeEditorSceneCache(sourceCache, document);
  const cr::CreativeTerrainGenerationResult firstGeneration =
      generationFor(1001U);
  app::CreativeEditorGeneratedTerrainPreviewCache previewCache;
  const bool firstRefresh =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          previewCache, sourceCache, document, firstGeneration);

  bool reused = true;
  for (std::uint32_t frame = 0U; frame < 300U; ++frame) {
    reused = !app::refreshCreativeEditorGeneratedTerrainPreview(
                 previewCache, sourceCache, document, firstGeneration) &&
             reused;
  }

  app::invalidateCreativeEditorSceneCache(sourceCache);
  const bool sourceRebuilt =
      app::refreshCreativeEditorSceneCache(sourceCache, document);
  const bool sourceRefresh =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          previewCache, sourceCache, document, firstGeneration);

  const cr::CreativeTerrainGenerationResult secondGeneration =
      generationFor(2002U);
  const bool secondRefresh =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          previewCache, sourceCache, document, secondGeneration);
  const bool containsOutsideSource = std::any_of(
      previewCache.composedSurface.columns.begin(),
      previewCache.composedSurface.columns.end(),
      [](const cr::CreativeTerrainColumn& column) {
        return column.coord == cr::CreativeTerrainCoord2{10, 0} &&
               column.heightCells == 4U;
      });

  bool generatedRegionMatches = true;
  const cr::CreativeTerrainHeightFieldBounds generatedBounds =
      secondGeneration.plan.heightField.bounds();
  std::size_t generatedIndex = 0U;
  for (std::uint16_t z = 0U; z < generatedBounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < generatedBounds.widthCells; ++x) {
      const cr::CreativeTerrainCoord2 coord{
          generatedBounds.minimum.x + static_cast<std::int32_t>(x),
          generatedBounds.minimum.z + static_cast<std::int32_t>(z)};
      const auto found = std::lower_bound(
          previewCache.composedSurface.columns.begin(),
          previewCache.composedSurface.columns.end(), coord,
          [](const cr::CreativeTerrainColumn& column,
             cr::CreativeTerrainCoord2 candidate) {
            return column.coord.z != candidate.z
                       ? column.coord.z < candidate.z
                       : column.coord.x < candidate.x;
          });
      generatedRegionMatches =
          generatedRegionMatches &&
          found != previewCache.composedSurface.columns.end() &&
          found->coord == coord &&
          found->heightCells ==
              secondGeneration.plan.heightField.heights()[generatedIndex];
      ++generatedIndex;
    }
  }

  cr::CreativeTerrainGenerationResult invalidGeneration;
  const bool invalidRefresh =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          previewCache, sourceCache, document, invalidGeneration);

  return expect(terrainApplied.accepted && sourceBuilt &&
                    firstGeneration.receipt.accepted && firstRefresh,
                "valid generation builds a transient preview scene") &&
         expect(previewCache.refreshCount == 3U && sourceRebuilt &&
                    sourceRefresh && secondRefresh && reused,
                "idle frames reuse output while source and height changes rebuild") &&
         expect(firstGeneration.receipt.heightHash !=
                    secondGeneration.receipt.heightHash,
                "test seeds produce distinct generated terrain") &&
         expect(containsOutsideSource && generatedRegionMatches,
                "generated region replaces source while outside terrain remains") &&
         expect(previewCache.composedSurface.columns.empty() &&
                    previewCache.terrainCollisionPatches.empty() &&
                    previewCache.terrainSurfacePatches.empty() &&
                    !previewCache.valid && !invalidRefresh,
                "invalid generation clears transient output atomically") &&
         expect(document.revision() == documentRevision &&
                    document.terrainField().controlCount() == 2U,
                "preview never mutates document or legacy terrain controls");
}

}  // namespace

int main() {
  return generatedPreviewReplacesTerrainAndCachesByHeightHash()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
