#pragma once

#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainPathSource.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeWatercourseRecipeVersion = 1U;

struct CreativeWatercourseRecipeRequest {
  std::uint32_t version = kCreativeWatercourseRecipeVersion;
  std::string instanceKey;
  std::string name = "Watercourse";
  CreativeGridSettings grid{};
  CreativeTerrainPathSourceRecipe source{};
};

enum class CreativeWatercourseRecipeStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRequest,
  PathRejected,
  TerrainRejected,
  CrossingRejected,
  Ready,
};

struct CreativeWatercourseSurfaceSample {
  CreativeTerrainCoord2 coord{};
  double flowProgress = 0.0;
  double bankHeightCells = 0.0;
  double bedHeightCells = 0.0;
  double reservedSurfaceHeightCells = 0.0;
  CreativeVec3 bedPositionMeters{};
  CreativeVec3 reservedSurfacePositionMeters{};

};

// One exact attachment frame across a watercourse. The later Bridge recipe can
// consume this without re-sampling the path or asking the creator to align a
// span by eye. This record does not create a bridge or water object itself.
struct CreativeWatercourseCrossingFrame {
  CreativeTerrainWatercourseCrossingId id =
      kInvalidCreativeTerrainWatercourseCrossingId;
  CreativeTerrainPathSourcePointId sourcePointId =
      kInvalidCreativeTerrainPathSourcePointId;
  CreativeVec3 centerMeters{};
  CreativeVec3 crossingAxis{};
  CreativeVec3 leftBankMeters{};
  CreativeVec3 rightBankMeters{};
  CreativeVec3 leftApproachMeters{};
  CreativeVec3 rightApproachMeters{};
  // Grid-space facts retain the exact terrain samples used to derive the
  // world-space frame. Bridge and approach-grade owners consume these rather
  // than re-sampling or reverse-engineering the watercourse path.
  CreativeVec3 centerGrid{};
  CreativeVec3 leftBankGrid{};
  CreativeVec3 rightBankGrid{};
  CreativeVec3 leftApproachGrid{};
  CreativeVec3 rightApproachGrid{};
  CreativeVec3 channelBedGrid{};
  CreativeVec3 clearanceReferenceGrid{};
  CreativeVec3 channelBedMeters{};
  CreativeVec3 clearanceReferenceMeters{};
  CreativeTransform bridgeTransform{};
  double spanMeters = 0.0;

};

struct CreativeWatercourseRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeWatercourseRecipeStatus status =
      CreativeWatercourseRecipeStatus::NotRequested;
  std::uint64_t centerlineSampleCount = 0U;
  std::uint64_t reservedSurfaceSampleCount = 0U;
  std::uint64_t crossingCount = 0U;
  std::uint64_t terrainCellCount = 0U;
  std::uint64_t terrainHeightHash = 0U;
  std::uint64_t definitionFingerprint = 0U;
  std::string reasonCode = "creative_watercourse_recipe_not_requested";
};

struct CreativeWatercoursePlan {
  CreativeRecipeKind kind = CreativeRecipeKind::Watercourse;
  CreativeTerrainPathKind pathKind = CreativeTerrainPathKind::River;
  CreativeTerrainWatercourseDrainageDirection drainageDirection =
      CreativeTerrainWatercourseDrainageDirection::Unspecified;
  CreativeTerrainWaterSurfacePolicy surfacePolicy =
      CreativeTerrainWaterSurfacePolicy::None;
  std::string instanceKey;
  std::uint64_t definitionFingerprint = 0U;
  std::vector<CreativeWatercourseSurfaceSample> surfaceSamples;
  std::vector<CreativeWatercourseCrossingFrame> crossings;
};

struct CreativeWatercoursePlanResult {
  CreativeWatercoursePlan plan;
  CreativeWatercourseRecipeReceipt receipt{};
};

struct CreativeWatercourseRecipeResult {
  CreativeTerrainPathSourceResult terrain;
  CreativeWatercoursePlan plan;
  CreativeWatercourseRecipeReceipt receipt{};
};

[[nodiscard]] std::string_view toString(
    CreativeWatercourseRecipeStatus status) noexcept;

// Plans reserved water elevation, flow order, and crossing attachment frames
// against already-composed terrain. It deliberately emits no water geometry,
// collision, simulation, or bridge object.
[[nodiscard]] CreativeWatercoursePlanResult planCreativeWatercourse(
    const CreativeTerrainHeightField& generatedTerrain,
    const CreativeWatercourseRecipeRequest& request);

[[nodiscard]] CreativeWatercourseRecipeResult buildCreativeWatercourseRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainMaterialField& materialSource,
    const CreativeWatercourseRecipeRequest& request,
    CreativeTerrainPathSourceCache* cache = nullptr);

}  // namespace iggy3d::creative
