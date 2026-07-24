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

enum class CreativeSemanticObjectAction : std::uint8_t {
  Inspect,
  Copy,
  Duplicate,
  Delete,
  Cut,
  Rename,
  SetVisible,
  SetLocked,
  TransformSelection,
  SetTransform,
  StructuralMutation,
  Count,
};

enum class CreativeSemanticObjectActionRoute : std::uint8_t {
  Reject,
  ReadOnly,
  Document,
  SemanticDocument,
  PatternRecipe,
  WorldLayoutSource,
  RefineThenAdopt,
};

struct CreativeSemanticObjectActionPolicy {
  bool allowed = false;
  CreativeSemanticObjectActionRoute route =
      CreativeSemanticObjectActionRoute::Reject;
  std::string_view reasonCode = "creative_semantic_action_not_requested";
};

enum class CreativeStructuralMutationAdmissionStatus : std::uint8_t {
  NotRequested,
  InvalidSelection,
  Ready,
  OwnershipRejected,
};

// Owner-facing receipt for raw structural document edits. Invalid or missing
// selections remain distinguishable from resolved semantic ownership
// conflicts, allowing domain owners to preserve their established validation
// receipts while still rejecting generated output before history begins.
struct CreativeStructuralMutationAdmission {
  bool requested = false;
  bool selectionResolved = false;
  bool allowed = false;
  CreativeStructuralMutationAdmissionStatus status =
      CreativeStructuralMutationAdmissionStatus::NotRequested;
  CreativeSemanticSelectionStatus selectionStatus =
      CreativeSemanticSelectionStatus::NotRequested;
  CreativeSemanticObjectActionRoute route =
      CreativeSemanticObjectActionRoute::Reject;
  std::size_t requestedObjectCount = 0U;
  std::size_t resolvedObjectCount = 0U;
  CreativeObjectId primaryObjectId = kInvalidObjectId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::string_view reasonCode =
      "creative_structural_mutation_not_requested";
};

[[nodiscard]] constexpr bool
creativeStructuralMutationOwnershipRejected(
    const CreativeStructuralMutationAdmission& admission) noexcept {
  return admission.status ==
         CreativeStructuralMutationAdmissionStatus::OwnershipRejected;
}

[[nodiscard]] constexpr bool creativeSemanticActionUsesDocumentMutation(
    const CreativeSemanticObjectActionPolicy& policy) noexcept {
  return policy.allowed &&
         (policy.route == CreativeSemanticObjectActionRoute::Document ||
          policy.route ==
              CreativeSemanticObjectActionRoute::SemanticDocument);
}

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
  std::size_t authoredOwnerCount = 0U;
  std::size_t patternOwnerCount = 0U;
  std::size_t worldLayoutOwnerCount = 0U;
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

// Provenance ancestry is distinct from the nearest editable owner. Pattern
// output can retain this tag while PatternRecipe remains its primary owner.
[[nodiscard]] bool creativeObjectHasWorldLayoutProvenanceTag(
    const CreativeObject& object) noexcept;

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

// A one-room building's deepest shared source is its room. Transform admission
// can promote that source to Building only when the requested selection contains
// every generated member of the same building.
[[nodiscard]] CreativeWorldLayoutSourceRef
resolveCompleteCreativeWorldLayoutBuildingSelectionSource(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeWorldLayout& worldLayout);

// One closed policy for actions that begin from generated document output.
// Validation remains with the selected owner; this function only decides which
// owner is allowed to interpret the command.
[[nodiscard]] CreativeSemanticObjectActionPolicy
resolveCreativeSemanticObjectAction(
    CreativeSemanticSelectionOwner owner,
    CreativeSemanticObjectAction action,
    CreativeWorldLayoutTable worldLayoutTable =
        CreativeWorldLayoutTable::None,
    std::size_t worldLayoutContributorCount = 0U) noexcept;
[[nodiscard]] CreativeSemanticObjectActionPolicy
resolveCreativeSemanticObjectAction(
    const CreativeSemanticSelectionResolution& selection,
    CreativeSemanticObjectAction action) noexcept;
[[nodiscard]] CreativeSemanticObjectActionPolicy
resolveCreativeSemanticObjectAction(
    const CreativeSemanticSelectionSetResolution& selection,
    CreativeSemanticObjectAction action) noexcept;

// Structural mutation owners admit only the exact raw Document route.
// SemanticDocument is intentionally excluded because it requires a
// recipe-aware owner rather than direct object mutation.
[[nodiscard]] CreativeStructuralMutationAdmission
resolveCreativeStructuralMutationAdmission(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const CreativeWorldLayout* worldLayout = nullptr) noexcept;
[[nodiscard]] CreativeStructuralMutationAdmission
resolveCreativeStructuralMutationAdmission(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeObjectId primaryObjectId = kInvalidObjectId,
    const CreativeWorldLayout* worldLayout = nullptr) noexcept;

}  // namespace iggy3d::creative
