#pragma once

#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/recipes/RampRecipe.hpp"
#include "app/iggy3d/creative/recipes/StairRecipe.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeRetainingEdgeRecipeVersion = 1U;
inline constexpr std::size_t kCreativeRetainingEdgeTransitionCapacity = 16U;
inline constexpr std::size_t kCreativeRetainingEdgeGeneratedObjectCapacity =
    4096U;

enum class CreativeRetainingEdgeSelection : std::uint8_t {
  All,
  Internal,
  Perimeter,
  Count,
};

enum class CreativeRetainingEdgeKit : std::uint8_t {
  Procedural,
  InfrastructureStone,
  Count,
};

enum class CreativeRetainingEdgeTransitionKind : std::uint8_t {
  Stair,
  Ramp,
  Count,
};

struct CreativeRetainingEdgeTransition {
  CreativeTerrainHardEdge edge{};
  CreativeRetainingEdgeTransitionKind kind =
      CreativeRetainingEdgeTransitionKind::Stair;
  std::uint16_t runCells = 3U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeRetainingEdgeTransition,
      CreativeRetainingEdgeTransition) noexcept = default;
};

struct CreativeRetainingEdgeSettings {
  CreativeRetainingEdgeSelection selection =
      CreativeRetainingEdgeSelection::All;
  CreativeRetainingEdgeKit kit = CreativeRetainingEdgeKit::Procedural;
  double thicknessMeters = 0.35;
  double maximumHeightMeters = 16.0;
  bool closeCorners = true;
  bool capEnds = true;
  CreativeStructuralMaterial material = CreativeStructuralMaterial::Stone;
  std::array<CreativeRetainingEdgeTransition,
             kCreativeRetainingEdgeTransitionCapacity>
      transitions{};
  std::size_t transitionCount = 0U;

  [[nodiscard]] friend constexpr bool operator==(
      const CreativeRetainingEdgeSettings&,
      const CreativeRetainingEdgeSettings&) noexcept = default;
};

// Durable source attached to one exact World Layout landform profile. Terrain
// cut/fill remains owned by that profile; this recipe decorates its canonical
// hard seams and never invents a second terrain representation.
struct CreativeRetainingEdgeSourceRecipe {
  std::uint32_t version = kCreativeRetainingEdgeRecipeVersion;
  std::string terrainProfileKey;
  CreativeRetainingEdgeSettings settings{};

  [[nodiscard]] friend bool operator==(
      const CreativeRetainingEdgeSourceRecipe&,
      const CreativeRetainingEdgeSourceRecipe&) noexcept = default;
};

struct CreativeRetainingEdgeRecipeRequest {
  std::uint32_t version = kCreativeRetainingEdgeRecipeVersion;
  std::string instanceKey;
  std::string name = "Retaining Edge";
  CreativeGridSettings grid{};
  CreativeTerrainHeightFieldBounds profileBounds{};
  CreativeRetainingEdgeSourceRecipe source{};
  const CreativeTerrainHeightField* terrain = nullptr;
  std::span<const CreativeTerrainHardEdge> hardEdges;
  std::vector<std::string> tags;
};

enum class CreativeRetainingEdgeRecipeStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRequest,
  NoMatchingEdges,
  TransitionInvalid,
  HeightExceeded,
  StructureCapacityExceeded,
  InvalidStructure,
  Ready,
};

struct CreativeRetainingEdgeRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeRetainingEdgeRecipeStatus status =
      CreativeRetainingEdgeRecipeStatus::NotRequested;
  std::uint64_t selectedEdgeCount = 0U;
  std::uint64_t wallSegmentCount = 0U;
  std::uint64_t cornerCount = 0U;
  std::uint64_t capCount = 0U;
  std::uint64_t stairCount = 0U;
  std::uint64_t rampCount = 0U;
  std::uint64_t generatedObjectCount = 0U;
  std::uint64_t definitionFingerprint = 0U;
  double maximumWallHeightMeters = 0.0;
  std::string reasonCode = "creative_retaining_edge_recipe_not_requested";
};

struct CreativeRetainingEdgeRecipeResult {
  CreativeRecipePlan structure;
  CreativeRetainingEdgeRecipeReceipt receipt{};
};

[[nodiscard]] bool isValidCreativeRetainingEdgeSettings(
    const CreativeRetainingEdgeSettings& settings) noexcept;
[[nodiscard]] bool isValidCreativeRetainingEdgeSourceRecipe(
    const CreativeRetainingEdgeSourceRecipe& source) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeRetainingEdgeRecipeStatus status) noexcept;

// Pure semantic planner over already-composed terrain and its exact hard-edge
// topology. The request is borrowed only for this call; generated output owns
// no terrain state.
[[nodiscard]] CreativeRetainingEdgeRecipeResult planCreativeRetainingEdge(
    const CreativeRetainingEdgeRecipeRequest& request);

}  // namespace iggy3d::creative
