#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/recipes/TerrainGrounding.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d::creative::world_layout_compile {

bool buildWorldLayoutObjectRecipes(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout,
    const CreativeGridSettings& grid,
    std::unordered_set<std::string>& stableKeys,
    std::vector<CreativeBuildingRecipeRequest>& buildings,
    std::vector<CreativeRecipePlan>& desiredObjectRecipes,
    std::vector<CreativeRecipePlan>& desiredLibraryRecipes,
    CreativeWorldLayoutCompileResult& result) {
  CreativeDocument terrainStaged = document;
  if (!stageWorldLayoutTerrain(document, layout, stableKeys, result,
                               terrainStaged)) {
    return false;
  }
  const CreativeTerrainSurfacePlan terrainSurface =
      buildCreativeComposedTerrainSurfacePlan(
          terrainStaged.terrainField(), document.terrainHeightField());
  if (!terrainSurface.accepted) {
    result.receipt.kernelReasonCode = terrainSurface.reasonCode;
    setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
              "creative_world_layout_terrain_surface_rejected");
    return false;
  }

  const double floorLayerThickness =
      defaultCreativeStructuralLayerThicknessMeters(CreativeObjectKind::Floor);
  for (std::size_t index = 0U; index < buildings.size(); ++index) {
    const CreativeWorldLayoutBuilding& symbol = layout.buildings[index];
    if (symbol.groundingMode ==
        CreativeWorldLayoutGroundingMode::Foundation) {
      double authoredGroundLayer =
          std::numeric_limits<double>::infinity();
      for (const CreativeWorldLayoutLevel& level : layout.levels) {
        if (level.buildingIndex == index) {
          const double floorThicknessLayers =
              static_cast<double>(level.floorThicknessLayers) *
              floorLayerThickness / grid.cellSizeMeters;
          authoredGroundLayer =
              std::min(authoredGroundLayer,
                       level.floorTopLayer - floorThicknessLayers);
        }
      }
      if (!std::isfinite(authoredGroundLayer)) {
        authoredGroundLayer = static_cast<double>(symbol.rootBaseLayer);
      }
      const CreativeTerrainGroundingPlan grounding =
          planCreativeTerrainGrounding(
              {&terrainSurface, symbol.rootFootprint.minimum,
               symbol.rootFootprint.maximum, authoredGroundLayer,
               symbol.maximumGroundReliefCells});
      if (!grounding.accepted) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Building;
        result.receipt.failedIndex = index;
        result.receipt.kernelReasonCode = grounding.reasonCode;
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_building_grounding_rejected");
        return false;
      }

      const double offsetMeters =
          grounding.verticalOffsetLayers * grid.cellSizeMeters;
      if (!shiftBuildingVertically(buildings[index], offsetMeters)) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Building;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_building_grounding_offset_invalid");
        return false;
      }
      ++result.receipt.groundedBuildingCount;

      if (grounding.reliefCells > 0U) {
        const std::string foundationKey =
            childKey(symbol.stableKey, "foundation");
        if (!registerKey(stableKeys, foundationKey,
                         CreativeWorldLayoutTable::Building, index,
                         result.receipt)) {
          return false;
        }
        CreativeBounds foundationBounds;
        if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                             symbol.rootFootprint.minimum.x,
                             foundationBounds.min.x) ||
            !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                             symbol.rootFootprint.maximum.x,
                             foundationBounds.max.x) ||
            !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                             symbol.rootFootprint.minimum.z,
                             foundationBounds.min.z) ||
            !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                             symbol.rootFootprint.maximum.z,
                             foundationBounds.max.z) ||
            !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                             grounding.minimumHeightCells,
                             foundationBounds.min.y) ||
            !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                             grounding.maximumHeightCells,
                             foundationBounds.max.y)) {
          result.receipt.failedTable = CreativeWorldLayoutTable::Building;
          result.receipt.failedIndex = index;
          setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                    "creative_world_layout_foundation_bounds_invalid");
          return false;
        }
        CreativeBuildingBoxSpec foundation{
            CreativeObjectKind::Floor, foundationKey,
            symbol.name + " Foundation", foundationBounds};
        appendTagOnce(foundation.tags,
                      creativeWorldLayoutProvenanceTag(
                          layout, CreativeWorldLayoutTable::Building, index));
        appendTagOnce(foundation.tags, "creative_world_layout:foundation");
        buildings[index].boxes.insert(buildings[index].boxes.begin(),
                                      std::move(foundation));
        ++result.receipt.foundationObjectCount;
      }
    }

    CreativeBuildingRecipeResult built =
        buildCreativeBuildingRecipe(buildings[index]);
    if (!built.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = built.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_building_rejected");
      return false;
    }
    built.plan.definitionFingerprint =
        fingerprintCreativeRecipePlan(built.plan);
    if (built.plan.definitionFingerprint == 0U) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode =
          "creative_recipe_definition_fingerprint_invalid";
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_building_fingerprint_rejected");
      return false;
    }
    desiredObjectRecipes.push_back(std::move(built.plan));
  }
  for (CreativeRecipePlan& recipe : desiredLibraryRecipes) {
    desiredObjectRecipes.push_back(std::move(recipe));
  }
  return true;
}

}  // namespace iggy3d::creative::world_layout_compile
