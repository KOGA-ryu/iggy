#pragma once

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeSemanticSelectionOwner : std::uint8_t {
  None,
  AuthoredObject,
  PatternRecipe,
  WorldLayoutSource,
};

enum class CreativeSemanticSelectionStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  MissingObject,
  Ready,
};

// One immutable interpretation of a raw document-object hit. Ownership is a
// chain, not an either/or choice: a PatternRecipe output may retain underlying
// World Layout provenance. primaryOwner names the nearest editable owner while
// the remaining fields preserve ancestry for Inspector and 2D synchronization.
struct CreativeSemanticSelectionResolution {
  bool requested = false;
  bool accepted = false;
  CreativeSemanticSelectionStatus status =
      CreativeSemanticSelectionStatus::NotRequested;
  CreativeSemanticSelectionOwner primaryOwner =
      CreativeSemanticSelectionOwner::None;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  // Effective state after resolving the complete parent chain. Local flags
  // remain available on the CreativeObject for Outliner/Inspector editing.
  bool objectVisible = false;
  bool objectLocked = false;
  bool hasParent = false;
  CreativeObjectId parentObjectId = kInvalidObjectId;
  CreativePatternRecipeId patternRecipeId = kInvalidCreativePatternRecipeId;
  CreativePatternRecipeKind patternRecipeKind =
      CreativePatternRecipeKind::Count;
  CreativeWorldLayoutObjectProvenance worldLayoutSource{};
  std::string_view reasonCode = "creative_selection_not_requested";
};

struct CreativeSemanticSelectionSetResolution {
  bool requested = false;
  bool accepted = false;
  CreativeSemanticSelectionStatus status =
      CreativeSemanticSelectionStatus::NotRequested;
  CreativeSemanticSelectionOwner primaryOwner =
      CreativeSemanticSelectionOwner::None;
  std::size_t selectedCount = 0U;
  std::size_t resolvedCount = 0U;
  std::size_t missingCount = 0U;
  CreativeObjectId primaryObjectId = kInvalidObjectId;
  CreativePatternRecipeId patternRecipeId = kInvalidCreativePatternRecipeId;
  CreativePatternRecipeKind patternRecipeKind =
      CreativePatternRecipeKind::Count;
  CreativeWorldLayoutSourceRef commonWorldLayoutSource{};
  std::string_view reasonCode = "creative_selection_set_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeSemanticSelectionOwner owner) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSemanticSelectionStatus status) noexcept;

// Hidden and locked objects still resolve: Outliner and Inspector must be able
// to inspect, reveal, or unlock them. Pointer hit admission owns whether such an
// object can be picked directly in a particular view.
[[nodiscard]] CreativeSemanticSelectionResolution
resolveCreativeSemanticSelection(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const CreativeWorldLayout* worldLayout = nullptr) noexcept;

// The point-aware overload disambiguates condensed generated walls. The point
// is expressed in World Layout grid cells, matching provenance resolution.
[[nodiscard]] CreativeSemanticSelectionResolution
resolveCreativeSemanticSelection(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const CreativeWorldLayout& worldLayout,
    CreativeVec3 sourcePointCells) noexcept;

// Resolves a normalized object selection to its deepest shared semantic owner.
// Pattern ownership wins when every selected output belongs to one recipe. A
// common World Layout source is still retained so 2D/elevation views can follow
// the same selection. Missing ids fail closed instead of silently shrinking the
// requested set.
[[nodiscard]] CreativeSemanticSelectionSetResolution
resolveCreativeSemanticSelectionSet(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeObjectId primaryObjectId = kInvalidObjectId,
    const CreativeWorldLayout* worldLayout = nullptr) noexcept;

}  // namespace iggy3d::creative
