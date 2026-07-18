#pragma once

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t kInvalidCreativeWorldLayoutRecipeIndex =
    std::numeric_limits<std::size_t>::max();

enum class CreativeWorldLayoutRecipeChangeKind : std::uint8_t {
  Add,
  Keep,
  Refined,
  Replace,
  Remove,
  Conflict,
  DetachAndReplace,
  Detach,
};

enum class CreativeWorldLayoutConflictResolution : std::uint8_t {
  Block,
  Regenerate,
  Detach,
  Count,
};

struct CreativeWorldLayoutConflictDecision {
  std::string instanceKey;
  CreativeWorldLayoutConflictResolution resolution =
      CreativeWorldLayoutConflictResolution::Block;
};

struct CreativeWorldLayoutRecipeChange {
  CreativeWorldLayoutRecipeChangeKind kind =
      CreativeWorldLayoutRecipeChangeKind::Keep;
  CreativeRecipeKind recipeKind = CreativeRecipeKind::Unknown;
  std::string instanceKey;
  std::size_t desiredRecipeIndex =
      kInvalidCreativeWorldLayoutRecipeIndex;
  std::uint64_t existingObjectCount = 0U;
  std::uint64_t desiredObjectCount = 0U;
  std::uint64_t refinedObjectCount = 0U;
  std::uint64_t missingBaselineCount = 0U;
};

struct CreativeWorldLayoutReconciliationRequest {
  const CreativeDocument* document = nullptr;
  std::string_view layoutTag;
  std::span<const CreativeRecipePlan> desiredRecipes;
  std::span<const CreativeWorldLayoutConflictDecision> conflictDecisions;
};

struct CreativeWorldLayoutReconciliationResult {
  bool accepted = false;
  bool blocked = false;
  std::vector<CreativeWorldLayoutRecipeChange> changes;
  std::vector<CreativeObjectId> removeObjectIds;
  std::vector<CreativeObjectId> detachObjectIds;
  std::vector<std::size_t> applyRecipeIndices;
  std::uint64_t createRecipeCount = 0U;
  std::uint64_t keepRecipeCount = 0U;
  std::uint64_t refinedRecipeCount = 0U;
  std::uint64_t replaceRecipeCount = 0U;
  std::uint64_t removeRecipeCount = 0U;
  std::uint64_t conflictRecipeCount = 0U;
  std::uint64_t detachRecipeCount = 0U;
  std::string reasonCode =
      "creative_world_layout_reconciliation_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutRecipeChangeKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutConflictResolution resolution) noexcept;

// Computes a deterministic three-way decision from generated baseline tags,
// the live document, and the newly desired recipes. Decision keys must be
// unique and identify actual conflicts; missing decisions block and stale
// decisions reject the whole plan. Group lookup is ordered and deterministic;
// member matching is quadratic only within one recipe ownership group.
[[nodiscard]] CreativeWorldLayoutReconciliationResult
reconcileCreativeWorldLayoutRecipes(
    const CreativeWorldLayoutReconciliationRequest& request);

// Removes only generator ownership metadata. Authored tags, hierarchy, object
// identity, geometry, and transforms remain intact.
[[nodiscard]] bool detachCreativeWorldLayoutObject(
    CreativeObject& object,
    std::string_view layoutTag);

}  // namespace iggy3d::creative
