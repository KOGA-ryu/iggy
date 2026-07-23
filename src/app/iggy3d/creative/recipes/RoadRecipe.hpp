#pragma once

#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainPathSource.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeRoadRecipeVersion = 1U;
inline constexpr std::size_t kCreativeRoadGeneratedEdgePieceCapacity = 2048U;

struct CreativeRoadRecipeRequest {
  std::uint32_t version = kCreativeRoadRecipeVersion;
  std::string instanceKey;
  std::string name = "Road";
  CreativeGridSettings grid{};
  CreativeTerrainPathSourceRecipe source{};
  std::vector<std::string> tags;
};

enum class CreativeRoadRecipeStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRequest,
  PathRejected,
  StructureCapacityExceeded,
  InvalidStructure,
  Ready,
};

struct CreativeRoadRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeRoadRecipeStatus status = CreativeRoadRecipeStatus::NotRequested;
  std::uint64_t centerlineSampleCount = 0U;
  std::uint64_t edgePieceCount = 0U;
  std::uint64_t suppressedJoinPieceCount = 0U;
  std::uint64_t terrainCellCount = 0U;
  std::uint64_t terrainHeightHash = 0U;
  std::uint64_t definitionFingerprint = 0U;
  std::string reasonCode = "creative_road_recipe_not_requested";
};

struct CreativeRoadStructureResult {
  CreativeRecipePlan plan;
  CreativeRoadRecipeReceipt receipt{};
};

struct CreativeRoadRecipeResult {
  CreativeTerrainPathSourceResult terrain;
  CreativeRecipePlan structure;
  CreativeRoadRecipeReceipt receipt{};
};

[[nodiscard]] std::string_view toString(
    CreativeRoadRecipeStatus status) noexcept;

// Builds only the generated structural members against an already composed
// road terrain field. World Layout uses this after deterministic operation
// replay, so multiple roads and earlier landforms share one final elevation.
[[nodiscard]] CreativeRoadStructureResult planCreativeRoadStructure(
    const CreativeTerrainHeightField& generatedTerrain,
    const CreativeRoadRecipeRequest& request);

// Canonical one-shot owner for direct 3D authoring and focused proofs. Terrain
// and generated edge members are both derived from request.source.
[[nodiscard]] CreativeRoadRecipeResult buildCreativeRoadRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainMaterialField& materialSource,
    const CreativeRoadRecipeRequest& request,
    CreativeTerrainPathSourceCache* cache = nullptr);

}  // namespace iggy3d::creative
