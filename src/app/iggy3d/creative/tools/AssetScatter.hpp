#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeAssetScatterCandidateCapacity = 128U;
inline constexpr std::size_t kCreativeAssetScatterEvaluationCapacity = 33U * 33U;

enum class CreativeAssetScatterStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  CapacityExceeded,
  Empty,
  Ready,
};

struct CreativeAssetScatterRequest {
  CreativeVec3 center{};
  double radiusMeters = 4.0;
  double spacingMeters = 2.0;
  double densityFraction = 0.65;
  CreativeAssetScatterYaw yaw = CreativeAssetScatterYaw::Full;
  double scaleVariation = 0.10;
  std::uint64_t seed = 0;
  std::size_t maxCandidateCount = kCreativeAssetScatterCandidateCapacity;
};

struct CreativeAssetScatterCandidate {
  CreativeVec3 position{};
  double yawOffsetRadians = 0.0;
  double uniformScale = 1.0;
};

enum class CreativeAssetScatterEvaluationStatus : std::uint8_t {
  Ready,
  DensityRejected,
  SpacingRejected,
  CapacityRejected,
  Count,
};

struct CreativeAssetScatterEvaluation {
  CreativeAssetScatterCandidate candidate{};
  CreativeAssetScatterEvaluationStatus status =
      CreativeAssetScatterEvaluationStatus::Count;
};

struct CreativeAssetScatterPlan {
  std::array<CreativeAssetScatterCandidate,
             kCreativeAssetScatterCandidateCapacity>
      candidates{};
  std::array<CreativeAssetScatterEvaluation,
             kCreativeAssetScatterEvaluationCapacity>
      evaluations{};
  std::size_t candidateCount = 0;
  std::size_t evaluationCount = 0;
  std::uint32_t examinedCellCount = 0;
  std::uint32_t densityRejectedCount = 0;
  std::uint32_t spacingRejectedCount = 0;
  CreativeAssetScatterStatus status =
      CreativeAssetScatterStatus::NotRequested;
  bool requested = false;
  bool accepted = false;
  bool truncated = false;

  [[nodiscard]] std::span<const CreativeAssetScatterCandidate> items()
      const noexcept {
    return {candidates.data(), candidateCount};
  }

  [[nodiscard]] std::span<const CreativeAssetScatterEvaluation>
  evaluatedItems() const noexcept {
    return {evaluations.data(), evaluationCount};
  }
};

enum class CreativeAssetScatterRecipeMutationStatus : std::uint8_t {
  NotRequested,
  Empty,
  InvalidRequest,
  CreateRejected,
  RecipeNotFound,
  RecipeKindMismatch,
  RecipeDependencyConflict,
  RecipeRejected,
  RemoveRejected,
  Applied,
};

struct CreativeAssetScatterRecipeMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool updatedExistingRecipe = false;
  CreativeAssetScatterRecipeMutationStatus status =
      CreativeAssetScatterRecipeMutationStatus::NotRequested;
  CreativePatternRecipeId patternRecipeId =
      kInvalidCreativePatternRecipeId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t requestedObjectCount = 0U;
  std::uint64_t generatedObjectCount = 0U;
  std::uint64_t replacedGeneratedObjectCount = 0U;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  CreativePatternRecipeMutationReceipt patternMutationReceipt;
  std::vector<CreativeObjectId> generatedObjectIds;
  std::vector<CreativeObjectId> replacedGeneratedObjectIds;
  std::string message = "creative_asset_scatter_recipe_not_requested";
};

static_assert(std::is_trivially_copyable_v<CreativeAssetScatterRequest>);
static_assert(std::is_standard_layout_v<CreativeAssetScatterRequest>);
static_assert(std::is_trivially_copyable_v<CreativeAssetScatterCandidate>);
static_assert(std::is_standard_layout_v<CreativeAssetScatterCandidate>);
static_assert(std::is_trivially_copyable_v<CreativeAssetScatterEvaluation>);
static_assert(std::is_standard_layout_v<CreativeAssetScatterEvaluation>);
static_assert(std::is_trivially_copyable_v<CreativeAssetScatterPlan>);
static_assert(std::is_standard_layout_v<CreativeAssetScatterPlan>);

// Bounded deterministic jittered-grid planner. Candidate spacing is enforced
// explicitly, so density/yaw/scale changes cannot create overlapping output.
[[nodiscard]] CreativeAssetScatterPlan planCreativeAssetScatter(
    const CreativeAssetScatterRequest& request) noexcept;

[[nodiscard]] std::uint64_t creativeAssetScatterSpatialKey(
    CreativeVec3 position,
    double quantumMeters) noexcept;

[[nodiscard]] std::string_view toString(
    CreativeAssetScatterRecipeMutationStatus status) noexcept;

// Creates geometry and its editable scatter relationship as one publication.
// The recipe owns canonical asset-local source bounds; requests carry each
// admitted output's placed authored bounds and must match the recipe identity.
[[nodiscard]] CreativeAssetScatterRecipeMutationReceipt
createCreativeAssetScatterRecipeAtomically(
    CreativeDocument& document,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    std::span<const CreativeObjectId> selectionFilterObjectIds,
    const CreativeAssetScatterRecipe& recipe);

// Rebuilds only recipeId's owned outputs. Its stable recipe id, selection
// filter, and all unrelated document geometry survive the replacement.
[[nodiscard]] CreativeAssetScatterRecipeMutationReceipt
updateCreativeAssetScatterRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    const CreativeAssetScatterRecipe& recipe);

// Appends one painted region and only its newly admitted outputs. Existing
// output ids and geometry remain untouched.
[[nodiscard]] CreativeAssetScatterRecipeMutationReceipt
extendCreativeAssetScatterRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    const CreativeAssetScatterRecipe& recipe);

// Adds one local exclusion and removes only the owned output under the eraser.
// Removing the final output removes the now-empty relationship as well.
[[nodiscard]] CreativeAssetScatterRecipeMutationReceipt
excludeCreativeAssetScatterOutputAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    CreativeObjectId outputObjectId,
    const CreativeAssetScatterRecipe& recipe);

// Removes one editable scatter relationship and every object it owns as one
// publication. Dependency preflight is identical to recipe replacement.
[[nodiscard]] CreativeAssetScatterRecipeMutationReceipt
removeCreativeAssetScatterRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId);

}  // namespace iggy3d::creative
