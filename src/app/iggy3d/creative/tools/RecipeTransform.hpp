#pragma once

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeRecipeTranslationStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidRequest,
  NotFound,
  UnsupportedOwner,
  MissingMember,
  LockedMember,
  DependencyConflict,
  CoordinateOverflow,
  PlanRejected,
  StaleSource,
  ApplyRejected,
  NoChange,
  Planned,
  Applied,
};

struct CreativePatternRecipeTranslationPlan {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRecipeTranslationStatus status =
      CreativeRecipeTranslationStatus::NotRequested;
  CreativeDocumentId sourceDocumentId = kInvalidDocumentId;
  std::uint64_t sourceRevision = 0U;
  CreativePatternRecipeId recipeId = kInvalidCreativePatternRecipeId;
  CreativePatternRecipeKind recipeKind = CreativePatternRecipeKind::Count;
  CreativeVec3 displacementMeters{};
  CreativePatternRecipe translatedRecipe{};
  CreativeSelectionPlacementRequest placementRequest{};
  CreativeSelectionPlacementPlan placementPlan{};
  std::vector<CreativeObjectId> memberObjectIds;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::string_view reasonCode =
      "creative_pattern_recipe_translation_not_requested";
};

struct CreativePatternRecipeTranslationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRecipeTranslationStatus status =
      CreativeRecipeTranslationStatus::NotRequested;
  CreativePatternRecipeId recipeId = kInvalidCreativePatternRecipeId;
  CreativePatternRecipeKind recipeKind = CreativePatternRecipeKind::Count;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  CreativeSelectionPlacementReceipt placement{};
  CreativePatternRecipeMutationReceipt recipeMutation{};
  std::string_view reasonCode =
      "creative_pattern_recipe_translation_not_requested";
};

struct CreativeTerrainOperationTranslationPlan {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRecipeTranslationStatus status =
      CreativeRecipeTranslationStatus::NotRequested;
  CreativeDocumentId sourceDocumentId = kInvalidDocumentId;
  std::uint64_t sourceRevision = 0U;
  CreativeTerrainOperationId operationId =
      kInvalidCreativeTerrainOperationId;
  CreativeTerrainOperationKind operationKind =
      CreativeTerrainOperationKind::Count;
  CreativeTerrainCoord2 deltaCells{};
  CreativeTerrainOperation sourceOperation{};
  CreativeTerrainOperation translatedOperation{};
  CreativeTerrainOperationMutationRequest mutationRequest{};
  CreativeTerrainOperationMutationPlan mutationPlan{};
  std::string_view reasonCode =
      "creative_terrain_operation_translation_not_requested";
};

struct CreativeTerrainOperationTranslationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRecipeTranslationStatus status =
      CreativeRecipeTranslationStatus::NotRequested;
  CreativeTerrainOperationId operationId =
      kInvalidCreativeTerrainOperationId;
  CreativeTerrainOperationKind operationKind =
      CreativeTerrainOperationKind::Count;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  CreativeTerrainOperationMutationReceipt mutation{};
  std::string_view reasonCode =
      "creative_terrain_operation_translation_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeRecipeTranslationStatus status) noexcept;

// Plans a rigid translation of one complete pattern relationship. The plan
// includes every source and generated member; moving only the picked output is
// never representable here. Radial pivots and scatter source regions move with
// their materialized members.
[[nodiscard]] CreativePatternRecipeTranslationPlan
planCreativePatternRecipeTranslation(
    const CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    CreativeVec3 displacementMeters);

// Applies a still-current plan through one staged document publication. The
// externally visible document revision advances exactly once.
[[nodiscard]] CreativePatternRecipeTranslationReceipt
applyCreativePatternRecipeTranslation(
    CreativeDocument& document,
    const CreativePatternRecipeTranslationPlan& plan);

// Returns the bounded horizontal source footprint used by the shared 3D
// transform preview. GeneratedTerrain has no local footprint and returns false.
[[nodiscard]] bool creativeTerrainOperationSpatialBounds(
    const CreativeTerrainOperation& operation,
    CreativeTerrainHeightFieldBounds& bounds) noexcept;

// Plans a grid-exact translation and replays the complete terrain stack. Only
// manually authored local operations are movable; generated and World Layout
// owners remain under their own source editors.
[[nodiscard]] CreativeTerrainOperationTranslationPlan
planCreativeTerrainOperationTranslation(
    const CreativeDocument& document,
    CreativeTerrainOperationId operationId,
    CreativeTerrainCoord2 deltaCells);

[[nodiscard]] CreativeTerrainOperationTranslationReceipt
applyCreativeTerrainOperationTranslation(
    CreativeDocument& document,
    const CreativeTerrainOperationTranslationPlan& plan);

}  // namespace iggy3d::creative
