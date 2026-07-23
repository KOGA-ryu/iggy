#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingTemplatePlacement.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/recipes/RetainingEdgeRecipe.hpp"
#include "app/iggy3d/creative/recipes/RoadRecipe.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::creative::world_layout_compile {
namespace {

using HardEdgesByTerrainSource =
    std::map<std::string, std::vector<CreativeTerrainHardEdge>, std::less<>>;

[[nodiscard]] bool coordInside(
    CreativeTerrainCoord2 coord,
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  const std::int64_t x =
      static_cast<std::int64_t>(coord.x) - bounds.minimum.x;
  const std::int64_t z =
      static_cast<std::int64_t>(coord.z) - bounds.minimum.z;
  return x >= 0 && z >= 0 && x < bounds.widthCells &&
         z < bounds.depthCells;
}

[[nodiscard]] HardEdgesByTerrainSource finalLandformHardEdgeOwners(
    const CreativeDocument& document) {
  HardEdgesByTerrainSource output;
  const auto& operations = document.terrainOperationStack().operations;
  // Landforms are the only operations that add hard edges, and each one first
  // erases every seam touching its bounds. The last touching landform therefore
  // owns a surviving seam. This reverse scan is bounded by E * 64 operations.
  for (const CreativeTerrainHardEdge edge : document.terrainHardEdges()) {
    for (auto operation = operations.rbegin(); operation != operations.rend();
         ++operation) {
      if (!operation->enabled ||
          operation->kind != CreativeTerrainOperationKind::Landform ||
          (!coordInside(edge.first, operation->landform.bounds) &&
           !coordInside(edge.second, operation->landform.bounds))) {
        continue;
      }
      output[operation->sourceKey].push_back(edge);
      break;
    }
  }
  return output;
}

}  // namespace

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
          terrainStaged.terrainField(), terrainStaged.terrainHeightField(),
          terrainStaged.terrainHardEdges());
  if (!terrainSurface.accepted) {
    result.receipt.kernelReasonCode = terrainSurface.reasonCode;
    setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
              "creative_world_layout_terrain_surface_rejected");
    return false;
  }

  result.plan.retainingEdgePlans.clear();
  result.receipt.retainingEdgeRecipeCount = 0U;
  result.receipt.retainingEdgeGeneratedObjectCount = 0U;
  const HardEdgesByTerrainSource ownedHardEdges =
      finalLandformHardEdgeOwners(terrainStaged);
  for (std::size_t index = 0U; index < layout.terrainProfiles.size(); ++index) {
    const CreativeWorldLayoutTerrainProfile& profile =
        layout.terrainProfiles[index];
    if (!profile.usesRetainingEdgeRecipe) {
      continue;
    }
    CreativeRetainingEdgeRecipeRequest request;
    request.instanceKey = profile.stableKey;
    request.name = profile.stableKey + " Retaining Edge";
    request.grid = grid;
    request.profileBounds = profile.landform.bounds;
    request.source = profile.retainingEdge;
    request.terrain = &terrainStaged.terrainHeightField();
    const std::string terrainSourceKey =
        creativeWorldLayoutTerrainLandformSourceKey(layout.stableKey,
                                                    profile.stableKey);
    const auto owned = ownedHardEdges.find(terrainSourceKey);
    request.hardEdges =
        owned == ownedHardEdges.end()
            ? std::span<const CreativeTerrainHardEdge>{}
            : std::span<const CreativeTerrainHardEdge>{owned->second};
    request.tags.push_back(creativeWorldLayoutTag(layout.stableKey));
    request.tags.push_back(creativeWorldLayoutProvenanceTag(
        layout, CreativeWorldLayoutTable::TerrainProfile, index));
    CreativeRetainingEdgeRecipeResult planned =
        planCreativeRetainingEdge(request);
    if (!planned.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = planned.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_retaining_edge_recipe_rejected");
      return false;
    }
    ++result.receipt.retainingEdgeRecipeCount;
    result.receipt.retainingEdgeGeneratedObjectCount +=
        planned.receipt.generatedObjectCount;
    desiredObjectRecipes.push_back(planned.structure);
    result.plan.retainingEdgePlans.push_back(std::move(planned));
  }

  for (std::size_t index = 0U; index < buildings.size(); ++index) {
    const CreativeWorldLayoutBuilding& symbol = layout.buildings[index];
    if (symbol.groundingMode ==
        CreativeWorldLayoutGroundingMode::Foundation) {
      const CreativeTerrainGroundingPlan grounding =
          planCreativeWorldLayoutBuildingGrounding(layout, index, grid,
                                                   terrainSurface);
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
  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& symbol = layout.terrainPaths[index];
    if (symbol.recipe.kind != CreativeTerrainPathKind::Road ||
        symbol.recipe.road.edgeTreatment ==
            CreativeTerrainRoadEdgeTreatment::None) {
      continue;
    }
    CreativeRoadRecipeRequest road;
    // Recipe instance keys share the same 128-character identifier grammar as
    // building recipes. The terrain operation source key intentionally uses a
    // slash-delimited namespace and therefore cannot double as provenance.
    road.instanceKey = symbol.stableKey;
    road.name = symbol.stableKey;
    road.grid = grid;
    road.source = symbol.recipe;
    road.tags.push_back(creativeWorldLayoutTag(layout.stableKey));
    road.tags.push_back(creativeWorldLayoutProvenanceTag(
        layout, CreativeWorldLayoutTable::TerrainPath, index));
    CreativeRoadStructureResult structure = planCreativeRoadStructure(
        terrainStaged.terrainHeightField(), road);
    if (!structure.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPath;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode =
          std::string(structure.receipt.reasonCode);
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_road_structure_rejected");
      return false;
    }
    if (!structure.plan.objects.empty()) {
      desiredObjectRecipes.push_back(std::move(structure.plan));
    }
  }
  for (const CreativeBridgeRecipeResult& bridge : result.plan.bridgePlans) {
    desiredObjectRecipes.push_back(bridge.structure);
  }
  return true;
}

}  // namespace iggy3d::creative::world_layout_compile
