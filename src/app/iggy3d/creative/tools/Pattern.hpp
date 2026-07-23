#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"

namespace iggy3d::creative {

enum class CreativeLinearArrayStatus : std::uint8_t {
  NotRequested,
  EmptySelection,
  InvalidRequest,
  OperationLimitExceeded,
  MissingObject,
  ObjectIdExhausted,
  PasteRejected,
  RecipeNotFound,
  RecipeKindMismatch,
  RecipeDependencyConflict,
  RecipeRejected,
  RemoveRejected,
  Planned,
  Applied,
};

enum class CreativeRadialArrayStatus : std::uint8_t {
  NotRequested,
  EmptySelection,
  InvalidRequest,
  DegenerateRadius,
  OperationLimitExceeded,
  MissingObject,
  ObjectIdExhausted,
  PasteRejected,
  RecipeNotFound,
  RecipeKindMismatch,
  RecipeDependencyConflict,
  RecipeRejected,
  RemoveRejected,
  Planned,
  Applied,
};

struct CreativeLinearArrayInstance {
  std::uint32_t ordinal = 0;
  CreativeVec3 offset{};
};

struct CreativeLinearArrayPlanRequest {
  std::uint64_t sourceObjectCount = 0;
  CreativeLinearArrayDirection direction =
      CreativeLinearArrayDirection::PositiveX;
  CreativeLinearArrayCopyCount copyCount =
      CreativeLinearArrayCopyCount::Four;
  CreativeLinearArraySpacing spacing =
      CreativeLinearArraySpacing::OneCell;
  double cellSize = 1.0;
  std::uint64_t maxGeneratedObjects =
      kCreativeLinearArrayGeneratedObjectCapacity;
};

struct CreativeLinearArrayPlanReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeLinearArrayStatus status =
      CreativeLinearArrayStatus::NotRequested;
  std::uint64_t sourceObjectCount = 0;
  std::uint64_t copyCount = 0;
  std::uint64_t generatedObjectCount = 0;
  std::array<CreativeLinearArrayInstance,
             kCreativeLinearArrayInstanceCapacity>
      instances{};
  std::size_t instanceCount = 0;
  std::string_view reasonCode = "creative_linear_array_not_requested";

  [[nodiscard]] std::span<const CreativeLinearArrayInstance>
  plannedInstances() const noexcept {
    return {instances.data(), instanceCount};
  }
};

struct CreativeLinearArrayReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeLinearArrayStatus status =
      CreativeLinearArrayStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t sourceObjectCount = 0;
  std::uint64_t generatedObjectCount = 0;
  std::uint64_t replacedGeneratedObjectCount = 0;
  bool updatedExistingRecipe = false;
  CreativePatternRecipeId patternRecipeId =
      kInvalidCreativePatternRecipeId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::size_t finalCopyFirstObjectIndex = 0;
  std::size_t finalCopyObjectCount = 0;
  CreativeLinearArrayPlanReceipt plan;
  CreativeClipboardCopyReceipt copyReceipt;
  CreativeClipboardBatchPasteReceipt pasteReceipt;
  CreativePatternRecipeMutationReceipt patternMutationReceipt;
  std::vector<CreativeObjectId> sourceObjectIds;
  std::string message = "creative_linear_array_not_requested";

  [[nodiscard]] std::span<const CreativeObjectId>
  generatedObjectIds() const noexcept {
    return pasteReceipt.pastedObjectIds;
  }

  [[nodiscard]] std::span<const CreativeObjectId>
  finalCopyObjectIds() const noexcept {
    if (finalCopyFirstObjectIndex > pasteReceipt.pastedObjectIds.size() ||
        finalCopyObjectCount >
            pasteReceipt.pastedObjectIds.size() - finalCopyFirstObjectIndex) {
      return {};
    }
    return {pasteReceipt.pastedObjectIds.data() + finalCopyFirstObjectIndex,
            finalCopyObjectCount};
  }
};

struct CreativeRadialArrayInstance {
  std::uint32_t ordinal = 0;
  double angleRadians = 0.0;
};

struct CreativeRadialArrayPlanRequest {
  std::uint64_t sourceObjectCount = 0;
  CreativeVec3 pivot{};
  CreativeAxis3 axis = CreativeAxis3::Y;
  CreativeRadialArrayInstanceCount instanceCount =
      CreativeRadialArrayInstanceCount::Eight;
  CreativeRadialArraySweep sweep = CreativeRadialArraySweep::Degrees360;
  std::uint64_t maxGeneratedObjects =
      kCreativeRadialArrayGeneratedObjectCapacity;
};

struct CreativeRadialArrayPlanReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeRadialArrayStatus status =
      CreativeRadialArrayStatus::NotRequested;
  CreativeVec3 pivot{};
  CreativeAxis3 axis = CreativeAxis3::Y;
  std::uint64_t sourceObjectCount = 0;
  std::uint64_t totalInstanceCount = 0;
  std::uint64_t generatedCopyCount = 0;
  std::uint64_t generatedObjectCount = 0;
  std::array<CreativeRadialArrayInstance,
             kCreativeRadialArrayInstanceCapacity>
      instances{};
  std::size_t instanceCount = 0;
  std::string_view reasonCode = "creative_radial_array_not_requested";

  [[nodiscard]] std::span<const CreativeRadialArrayInstance>
  plannedInstances() const noexcept {
    return {instances.data(), instanceCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeRadialArrayInstance>);
static_assert(std::is_trivially_copyable_v<CreativeRadialArrayPlanRequest>);
static_assert(std::is_trivially_copyable_v<CreativeRadialArrayPlanReceipt>);
static_assert(std::is_standard_layout_v<CreativeRadialArrayPlanReceipt>);

struct CreativeRadialArrayReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRadialArrayStatus status =
      CreativeRadialArrayStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t sourceObjectCount = 0;
  std::uint64_t generatedObjectCount = 0;
  std::uint64_t replacedGeneratedObjectCount = 0;
  bool updatedExistingRecipe = false;
  CreativePatternRecipeId patternRecipeId =
      kInvalidCreativePatternRecipeId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::size_t finalCopyFirstObjectIndex = 0;
  std::size_t finalCopyObjectCount = 0;
  CreativeRadialArrayPlanReceipt plan;
  CreativeClipboardCopyReceipt copyReceipt;
  CreativeClipboardBatchPasteReceipt pasteReceipt;
  CreativePatternRecipeMutationReceipt patternMutationReceipt;
  std::vector<CreativeObjectId> sourceObjectIds;
  std::string message = "creative_radial_array_not_requested";

  [[nodiscard]] std::span<const CreativeObjectId>
  generatedObjectIds() const noexcept {
    return pasteReceipt.pastedObjectIds;
  }

  [[nodiscard]] std::span<const CreativeObjectId>
  finalCopyObjectIds() const noexcept {
    if (finalCopyFirstObjectIndex > pasteReceipt.pastedObjectIds.size() ||
        finalCopyObjectCount >
            pasteReceipt.pastedObjectIds.size() - finalCopyFirstObjectIndex) {
      return {};
    }
    return {pasteReceipt.pastedObjectIds.data() + finalCopyFirstObjectIndex,
            finalCopyObjectCount};
  }
};

struct CreativePatternReplacementPreflight {
  bool accepted = false;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::string_view reasonCode = "creative_pattern_replacement_not_requested";
  std::vector<CreativeObjectId> rootObjectIds;
};

[[nodiscard]] std::string_view toString(
    CreativeLinearArrayStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeRadialArrayStatus status) noexcept;

// O(k), allocation-free, where k is the requested copy count (maximum 32).
// Every offset is derived from its ordinal, so floating-point error does not
// accumulate from one instance to the next.
[[nodiscard]] CreativeLinearArrayPlanReceipt planCreativeLinearArray(
    const CreativeLinearArrayPlanRequest& request) noexcept;

// O(k), allocation-free, where k is total instance count minus the original.
// Closed rings omit the duplicate 360-degree endpoint. Partial sweeps include
// both the original at zero degrees and the requested endpoint.
[[nodiscard]] CreativeRadialArrayPlanReceipt planCreativeRadialArray(
    const CreativeRadialArrayPlanRequest& request) noexcept;

// Shared replacement law for every editable pattern recipe. Generated roots
// may be rebuilt only when no outside child or downstream recipe depends on
// the geometry that would be removed.
[[nodiscard]] CreativePatternReplacementPreflight
preflightCreativePatternReplacement(
    const CreativeDocument& document,
    const CreativePatternRecipe& recipe);

// O(n log n + n * k), plus one target-document staging copy, where n is the
// unique selected-object count and k is copy count. The document is published
// only after every generated copy succeeds.
[[nodiscard]] CreativeLinearArrayReceipt createCreativeLinearArrayAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeLinearArrayRequest& request);

// Rebuilds only the generated members owned by recipeId, keeps the recipe ID
// stable, and publishes the replacement only after the new copies, recipe
// record, and old-output removal all succeed.
[[nodiscard]] CreativeLinearArrayReceipt
updateCreativeLinearArrayRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    const CreativeLinearArrayRequest& request);

// O(n log n + n * k), plus one target-document staging copy. Every copy uses
// the same pivot and source group, so parent remapping and rigid rotation remain
// atomic across the complete radial pattern.
[[nodiscard]] CreativeRadialArrayReceipt createCreativeRadialArrayAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeRadialArrayRequest& request);

[[nodiscard]] CreativeRadialArrayReceipt
updateCreativeRadialArrayRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    const CreativeRadialArrayRequest& request);

// Removes only the editable relationship. Source and generated geometry remain
// untouched as independent document objects.
[[nodiscard]] CreativePatternRecipeMutationReceipt
detachCreativePatternRecipe(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId);

}  // namespace iggy3d::creative
