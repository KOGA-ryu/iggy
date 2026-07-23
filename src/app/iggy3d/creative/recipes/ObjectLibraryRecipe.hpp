#pragma once

#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeObjectLibraryPlacementMode : std::uint8_t {
  Bounds,
  Point,
  Count,
};

enum class CreativeObjectLibraryRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  Empty,
  InvalidPlacement,
  InvalidPlan,
  Ready,
};

// A library item is authored either as an exact world-space bounds or as a
// point anchor. This keeps map recipes semantic while still using the object
// descriptor as the authority for the rendered/runtime object kind.
struct CreativeObjectLibraryPlacementSpec {
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  CreativeObjectLibraryPlacementMode mode =
      CreativeObjectLibraryPlacementMode::Bounds;
  std::string stableKey;
  std::string name;
  std::string assetId;
  CreativeBounds bounds;
  CreativeVec3 point;
  CreativeBounds assetSourceBounds;
  bool hasAssetSourceBounds = false;
  double yawRadians = 0.0;
  CreativeVec3 scale{1.0, 1.0, 1.0};
  bool visible = true;
  CreativePlayerSpawnSettings playerSpawn{};
  CreativeNpcSpawnSettings npcSpawn{};
  CreativeLootPointSettings lootPoint{};
  CreativeExitPointSettings exitPoint{};
  std::vector<std::string> tags;
};

struct CreativeObjectLibraryRecipeRequest {
  std::string stableKey = "object_library";
  std::string name = "Object Library Placement";
  std::vector<CreativeObjectLibraryPlacementSpec> placements;
};

struct CreativeObjectLibraryRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeObjectLibraryRecipeStatus status =
      CreativeObjectLibraryRecipeStatus::NotRequested;
  std::uint64_t boundedPlacementCount = 0U;
  std::uint64_t pointPlacementCount = 0U;
  std::size_t failedPlacementIndex = 0U;
  std::string reasonCode = "creative_object_library_recipe_not_requested";
};

struct CreativeObjectLibraryRecipeResult {
  CreativeRecipePlan plan;
  CreativeObjectLibraryRecipeReceipt receipt;
};

[[nodiscard]] std::string_view toString(
    CreativeObjectLibraryPlacementMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeObjectLibraryRecipeStatus status) noexcept;

[[nodiscard]] CreativeObjectLibraryRecipeResult
buildCreativeObjectLibraryRecipe(
    const CreativeObjectLibraryRecipeRequest& request);

}  // namespace iggy3d::creative
