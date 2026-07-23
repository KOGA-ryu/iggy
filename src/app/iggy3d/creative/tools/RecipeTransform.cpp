#include "app/iggy3d/creative/tools/RecipeTransform.hpp"

#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace iggy3d::creative {
namespace {

void setPatternStatus(CreativePatternRecipeTranslationPlan& plan,
                      CreativeRecipeTranslationStatus status,
                      std::string_view reasonCode) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
}

void setTerrainStatus(CreativeTerrainOperationTranslationPlan& plan,
                      CreativeRecipeTranslationStatus status,
                      std::string_view reasonCode) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool addFinite(double value,
                             double delta,
                             double& output) noexcept {
  output = value + delta;
  return std::isfinite(output);
}

[[nodiscard]] bool translateVec3(CreativeVec3& value,
                                 CreativeVec3 delta) noexcept {
  CreativeVec3 translated{};
  if (!addFinite(value.x, delta.x, translated.x) ||
      !addFinite(value.y, delta.y, translated.y) ||
      !addFinite(value.z, delta.z, translated.z)) {
    return false;
  }
  value = translated;
  return true;
}

[[nodiscard]] bool translateCoord(CreativeTerrainCoord2& value,
                                  CreativeTerrainCoord2 delta) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(value.x) + delta.x;
  const std::int64_t z = static_cast<std::int64_t>(value.z) + delta.z;
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  value = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] bool appendUniqueId(std::vector<CreativeObjectId>& ids,
                                  CreativeObjectId objectId) {
  if (objectId == kInvalidObjectId ||
      std::find(ids.begin(), ids.end(), objectId) != ids.end()) {
    return false;
  }
  ids.push_back(objectId);
  return true;
}

[[nodiscard]] bool patternDependenciesAllowTranslation(
    const CreativeDocument& document,
    const CreativePatternRecipe& recipe,
    CreativeObjectId& failedObjectId) {
  std::unordered_set<CreativeObjectId> movedIds;
  movedIds.reserve(recipe.sourceObjectIds.size() +
                   recipe.generatedObjectIds.size());
  movedIds.insert(recipe.sourceObjectIds.begin(), recipe.sourceObjectIds.end());
  movedIds.insert(recipe.generatedObjectIds.begin(),
                  recipe.generatedObjectIds.end());

  for (const CreativePatternRecipe& other :
       document.patternRecipeStore().recipes) {
    if (other.id == recipe.id) {
      continue;
    }
    for (CreativeObjectId sourceId : other.sourceObjectIds) {
      if (movedIds.contains(sourceId)) {
        failedObjectId = sourceId;
        return false;
      }
    }
    for (CreativeObjectId sourceId : recipe.sourceObjectIds) {
      if (std::find(other.generatedObjectIds.begin(),
                    other.generatedObjectIds.end(), sourceId) !=
          other.generatedObjectIds.end()) {
        failedObjectId = sourceId;
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] bool translatePatternRecipe(
    CreativePatternRecipe& recipe,
    CreativeVec3 delta) noexcept {
  switch (recipe.kind) {
    case CreativePatternRecipeKind::LinearArray:
      return true;
    case CreativePatternRecipeKind::RadialArray:
      return translateVec3(recipe.radial.pivot, delta);
    case CreativePatternRecipeKind::AssetScatter:
      for (CreativeVec3& center : recipe.scatter.paintCenters) {
        if (!translateVec3(center, delta)) {
          return false;
        }
      }
      for (CreativeAssetScatterExclusion& exclusion :
           recipe.scatter.exclusions) {
        if (!translateVec3(exclusion.center, delta)) {
          return false;
        }
      }
      return true;
    case CreativePatternRecipeKind::Count:
      return false;
  }
  return false;
}

[[nodiscard]] CreativeTerrainOperationMutationRequest updateRequest(
    const CreativeTerrainOperation& operation) {
  CreativeTerrainOperationMutationRequest request;
  request.kind = CreativeTerrainOperationMutationKind::Update;
  request.operationId = operation.id;
  request.owner = operation.owner;
  request.sourceKey = operation.sourceKey;
  request.operationKind = operation.kind;
  request.generation = operation.generation;
  request.composition = operation.composition;
  request.region = operation.region;
  request.grade = operation.grade;
  request.profile = operation.profile;
  request.path = operation.path;
  request.stamp = operation.stamp;
  request.landform = operation.landform;
  request.enabled = operation.enabled;
  return request;
}

[[nodiscard]] bool translateTerrainOperation(
    CreativeTerrainOperation& operation,
    CreativeTerrainCoord2 delta) noexcept {
  switch (operation.kind) {
    case CreativeTerrainOperationKind::GeneratedTerrain:
      return false;
    case CreativeTerrainOperationKind::Region:
      return translateCoord(operation.region.bounds.minimum, delta);
    case CreativeTerrainOperationKind::Grade:
      return translateCoord(operation.grade.start, delta) &&
             translateCoord(operation.grade.end, delta);
    case CreativeTerrainOperationKind::Profile:
      return translateCoord(operation.profile.center, delta);
    case CreativeTerrainOperationKind::Path:
      for (CreativeTerrainPathSourcePoint& point : operation.path.points) {
        if (!translateCoord(point.coord, delta)) {
          return false;
        }
      }
      return true;
    case CreativeTerrainOperationKind::Stamp:
      return translateCoord(operation.stamp.targetMinimum, delta);
    case CreativeTerrainOperationKind::Landform:
      return translateCoord(operation.landform.bounds.minimum, delta);
    case CreativeTerrainOperationKind::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool makeBounds(std::int64_t minimumX,
                              std::int64_t minimumZ,
                              std::int64_t maximumX,
                              std::int64_t maximumZ,
                              CreativeTerrainHeightFieldBounds& bounds) noexcept {
  if (minimumX < std::numeric_limits<std::int32_t>::min() ||
      minimumX > std::numeric_limits<std::int32_t>::max() ||
      minimumZ < std::numeric_limits<std::int32_t>::min() ||
      minimumZ > std::numeric_limits<std::int32_t>::max() ||
      maximumX < minimumX || maximumZ < minimumZ) {
    return false;
  }
  const std::uint64_t width =
      static_cast<std::uint64_t>(maximumX - minimumX) + 1U;
  const std::uint64_t depth =
      static_cast<std::uint64_t>(maximumZ - minimumZ) + 1U;
  if (width == 0U || depth == 0U ||
      width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  bounds = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(depth)};
  return true;
}

[[nodiscard]] bool expandedPointBounds(CreativeTerrainCoord2 first,
                                       CreativeTerrainCoord2 second,
                                       std::uint64_t expansion,
                                       CreativeTerrainHeightFieldBounds& bounds)
    noexcept {
  const std::int64_t minimumX =
      std::min<std::int64_t>(first.x, second.x) -
      static_cast<std::int64_t>(expansion);
  const std::int64_t minimumZ =
      std::min<std::int64_t>(first.z, second.z) -
      static_cast<std::int64_t>(expansion);
  const std::int64_t maximumX =
      std::max<std::int64_t>(first.x, second.x) +
      static_cast<std::int64_t>(expansion);
  const std::int64_t maximumZ =
      std::max<std::int64_t>(first.z, second.z) +
      static_cast<std::int64_t>(expansion);
  return makeBounds(minimumX, minimumZ, maximumX, maximumZ, bounds);
}

}  // namespace

std::string_view toString(CreativeRecipeTranslationStatus status) noexcept {
  switch (status) {
    case CreativeRecipeTranslationStatus::NotRequested: return "NotRequested";
    case CreativeRecipeTranslationStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeRecipeTranslationStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeRecipeTranslationStatus::NotFound: return "NotFound";
    case CreativeRecipeTranslationStatus::UnsupportedOwner:
      return "UnsupportedOwner";
    case CreativeRecipeTranslationStatus::MissingMember:
      return "MissingMember";
    case CreativeRecipeTranslationStatus::LockedMember:
      return "LockedMember";
    case CreativeRecipeTranslationStatus::DependencyConflict:
      return "DependencyConflict";
    case CreativeRecipeTranslationStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeRecipeTranslationStatus::PlanRejected: return "PlanRejected";
    case CreativeRecipeTranslationStatus::StaleSource: return "StaleSource";
    case CreativeRecipeTranslationStatus::ApplyRejected:
      return "ApplyRejected";
    case CreativeRecipeTranslationStatus::NoChange: return "NoChange";
    case CreativeRecipeTranslationStatus::Planned: return "Planned";
    case CreativeRecipeTranslationStatus::Applied: return "Applied";
  }
  return "NotRequested";
}

CreativePatternRecipeTranslationPlan planCreativePatternRecipeTranslation(
    const CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    CreativeVec3 displacementMeters) {
  CreativePatternRecipeTranslationPlan plan;
  plan.requested = true;
  plan.sourceDocumentId = document.id();
  plan.sourceRevision = document.revision();
  plan.recipeId = recipeId;
  plan.displacementMeters = displacementMeters;
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    setPatternStatus(plan, CreativeRecipeTranslationStatus::InvalidDocument,
                     "creative_pattern_recipe_translation_document_invalid");
    return plan;
  }
  if (recipeId == kInvalidCreativePatternRecipeId ||
      !isFiniteCreativeVec3(displacementMeters)) {
    setPatternStatus(plan, CreativeRecipeTranslationStatus::InvalidRequest,
                     "creative_pattern_recipe_translation_request_invalid");
    return plan;
  }
  const CreativePatternRecipe* recipe =
      findCreativePatternRecipe(document.patternRecipeStore(), recipeId);
  if (recipe == nullptr) {
    setPatternStatus(plan, CreativeRecipeTranslationStatus::NotFound,
                     "creative_pattern_recipe_translation_not_found");
    return plan;
  }
  plan.recipeKind = recipe->kind;
  plan.translatedRecipe = *recipe;
  plan.memberObjectIds.reserve(recipe->sourceObjectIds.size() +
                               recipe->generatedObjectIds.size());
  for (CreativeObjectId objectId : recipe->sourceObjectIds) {
    if (!appendUniqueId(plan.memberObjectIds, objectId)) {
      plan.failedObjectId = objectId;
      setPatternStatus(plan, CreativeRecipeTranslationStatus::InvalidRequest,
                       "creative_pattern_recipe_translation_members_invalid");
      return plan;
    }
  }
  for (CreativeObjectId objectId : recipe->generatedObjectIds) {
    if (!appendUniqueId(plan.memberObjectIds, objectId)) {
      plan.failedObjectId = objectId;
      setPatternStatus(plan, CreativeRecipeTranslationStatus::InvalidRequest,
                       "creative_pattern_recipe_translation_members_invalid");
      return plan;
    }
  }
  for (CreativeObjectId objectId : plan.memberObjectIds) {
    if (document.findObject(objectId) == nullptr) {
      plan.failedObjectId = objectId;
      setPatternStatus(plan, CreativeRecipeTranslationStatus::MissingMember,
                       "creative_pattern_recipe_translation_member_missing");
      return plan;
    }
    if (creativeObjectEffectivelyLocked(document, objectId)) {
      plan.failedObjectId = objectId;
      setPatternStatus(plan, CreativeRecipeTranslationStatus::LockedMember,
                       "creative_pattern_recipe_translation_member_locked");
      return plan;
    }
  }
  if (!patternDependenciesAllowTranslation(document, *recipe,
                                            plan.failedObjectId)) {
    setPatternStatus(
        plan, CreativeRecipeTranslationStatus::DependencyConflict,
        "creative_pattern_recipe_translation_dependency_conflict");
    return plan;
  }
  if (creativeVec3ExactlyEqual(displacementMeters, {})) {
    plan.accepted = true;
    setPatternStatus(plan, CreativeRecipeTranslationStatus::NoChange,
                     "creative_pattern_recipe_translation_no_change");
    return plan;
  }
  if (!translatePatternRecipe(plan.translatedRecipe, displacementMeters) ||
      !validateCreativePatternRecipe(plan.translatedRecipe)) {
    setPatternStatus(plan, CreativeRecipeTranslationStatus::CoordinateOverflow,
                     "creative_pattern_recipe_translation_overflow");
    return plan;
  }

  plan.placementRequest.mode = CreativeSelectionPlacementMode::Move;
  plan.placementRequest.targetAnchor = displacementMeters;
  std::vector<CreativeObject> objects;
  objects.reserve(plan.memberObjectIds.size());
  for (CreativeObjectId objectId : plan.memberObjectIds) {
    objects.push_back(*document.findObject(objectId));
  }
  plan.placementPlan =
      planCreativeSelectionPlacement(objects, plan.placementRequest);
  if (!plan.placementPlan.accepted) {
    plan.failedObjectId = plan.placementPlan.failedObjectId;
    setPatternStatus(plan, CreativeRecipeTranslationStatus::PlanRejected,
                     plan.placementPlan.reasonCode);
    return plan;
  }
  plan.accepted = true;
  plan.changed = true;
  setPatternStatus(plan, CreativeRecipeTranslationStatus::Planned,
                   "creative_pattern_recipe_translation_planned");
  return plan;
}

CreativePatternRecipeTranslationReceipt applyCreativePatternRecipeTranslation(
    CreativeDocument& document,
    const CreativePatternRecipeTranslationPlan& plan) {
  CreativePatternRecipeTranslationReceipt receipt;
  receipt.requested = true;
  receipt.recipeId = plan.recipeId;
  receipt.recipeKind = plan.recipeKind;
  receipt.failedObjectId = plan.failedObjectId;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (!plan.requested || !plan.accepted) {
    receipt.status = CreativeRecipeTranslationStatus::InvalidRequest;
    receipt.reasonCode = "creative_pattern_recipe_translation_plan_invalid";
    return receipt;
  }
  if (!plan.changed) {
    receipt.accepted = true;
    receipt.status = CreativeRecipeTranslationStatus::NoChange;
    receipt.reasonCode = "creative_pattern_recipe_translation_no_change";
    return receipt;
  }
  if (document.id() != plan.sourceDocumentId ||
      document.revision() != plan.sourceRevision) {
    receipt.status = CreativeRecipeTranslationStatus::StaleSource;
    receipt.reasonCode = "creative_pattern_recipe_translation_source_stale";
    return receipt;
  }

  CreativeDocument staged = document;
  receipt.placement = placeDocumentObjectsAtomically(
      staged, plan.memberObjectIds, plan.placementRequest);
  if (!receipt.placement.accepted || !receipt.placement.changed) {
    receipt.failedObjectId = receipt.placement.failedObjectId;
    receipt.status = CreativeRecipeTranslationStatus::ApplyRejected;
    receipt.reasonCode = receipt.placement.reasonCode;
    return receipt;
  }
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Replace;
  mutation.recipeId = plan.recipeId;
  mutation.recipe = plan.translatedRecipe;
  receipt.recipeMutation = staged.applyPatternRecipeMutation(mutation);
  if (!receipt.recipeMutation.accepted) {
    receipt.status = CreativeRecipeTranslationStatus::ApplyRejected;
    receipt.reasonCode = receipt.recipeMutation.reasonCode;
    return receipt;
  }
  if (!document.commitStagedMutation(std::move(staged))) {
    receipt.status = CreativeRecipeTranslationStatus::StaleSource;
    receipt.reasonCode = "creative_pattern_recipe_translation_publish_stale";
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeRecipeTranslationStatus::Applied;
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_pattern_recipe_translation_applied";
  return receipt;
}

bool creativeTerrainOperationSpatialBounds(
    const CreativeTerrainOperation& operation,
    CreativeTerrainHeightFieldBounds& bounds) noexcept {
  bounds = {};
  switch (operation.kind) {
    case CreativeTerrainOperationKind::GeneratedTerrain:
      return false;
    case CreativeTerrainOperationKind::Region:
      bounds = operation.region.bounds;
      return isValidCreativeTerrainHeightFieldBounds(bounds);
    case CreativeTerrainOperationKind::Grade:
      return expandedPointBounds(
          operation.grade.start, operation.grade.end,
          static_cast<std::uint64_t>(operation.grade.halfWidthCells) +
              operation.grade.falloffCells,
          bounds);
    case CreativeTerrainOperationKind::Profile:
      return expandedPointBounds(operation.profile.center,
                                 operation.profile.center,
                                 operation.profile.radiusCells, bounds);
    case CreativeTerrainOperationKind::Path: {
      if (operation.path.points.empty()) {
        return false;
      }
      std::int64_t minimumX = operation.path.points.front().coord.x;
      std::int64_t minimumZ = operation.path.points.front().coord.z;
      std::int64_t maximumX = minimumX;
      std::int64_t maximumZ = minimumZ;
      std::uint64_t expansion = operation.path.falloffCells;
      for (const CreativeTerrainPathSourcePoint& point : operation.path.points) {
        minimumX = std::min<std::int64_t>(minimumX, point.coord.x);
        minimumZ = std::min<std::int64_t>(minimumZ, point.coord.z);
        maximumX = std::max<std::int64_t>(maximumX, point.coord.x);
        maximumZ = std::max<std::int64_t>(maximumZ, point.coord.z);
        expansion = std::max<std::uint64_t>(
            expansion, static_cast<std::uint64_t>(point.halfWidthCells) +
                           operation.path.falloffCells);
      }
      return makeBounds(minimumX - static_cast<std::int64_t>(expansion),
                        minimumZ - static_cast<std::int64_t>(expansion),
                        maximumX + static_cast<std::int64_t>(expansion),
                        maximumZ + static_cast<std::int64_t>(expansion),
                        bounds);
    }
    case CreativeTerrainOperationKind::Stamp: {
      const bool swap = (operation.stamp.quarterTurns & 1U) != 0U;
      const std::uint16_t width = swap ? operation.stamp.stamp.depthCells
                                       : operation.stamp.stamp.widthCells;
      const std::uint16_t depth = swap ? operation.stamp.stamp.widthCells
                                       : operation.stamp.stamp.depthCells;
      if (width == 0U || depth == 0U) {
        return false;
      }
      bounds = {operation.stamp.targetMinimum, width, depth};
      return isValidCreativeTerrainHeightFieldBounds(bounds);
    }
    case CreativeTerrainOperationKind::Landform:
      bounds = operation.landform.bounds;
      return isValidCreativeTerrainHeightFieldBounds(bounds);
    case CreativeTerrainOperationKind::Count:
      return false;
  }
  return false;
}

CreativeTerrainOperationTranslationPlan planCreativeTerrainOperationTranslation(
    const CreativeDocument& document,
    CreativeTerrainOperationId operationId,
    CreativeTerrainCoord2 deltaCells) {
  CreativeTerrainOperationTranslationPlan plan;
  plan.requested = true;
  plan.sourceDocumentId = document.id();
  plan.sourceRevision = document.revision();
  plan.operationId = operationId;
  plan.deltaCells = deltaCells;
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    setTerrainStatus(plan, CreativeRecipeTranslationStatus::InvalidDocument,
                     "creative_terrain_operation_translation_document_invalid");
    return plan;
  }
  if (operationId == kInvalidCreativeTerrainOperationId) {
    setTerrainStatus(plan, CreativeRecipeTranslationStatus::InvalidRequest,
                     "creative_terrain_operation_translation_request_invalid");
    return plan;
  }
  const CreativeTerrainOperation* operation = findCreativeTerrainOperation(
      document.terrainOperationStack(), operationId);
  if (operation == nullptr) {
    setTerrainStatus(plan, CreativeRecipeTranslationStatus::NotFound,
                     "creative_terrain_operation_translation_not_found");
    return plan;
  }
  plan.operationKind = operation->kind;
  plan.sourceOperation = *operation;
  plan.translatedOperation = *operation;
  if (operation->owner != CreativeTerrainOperationOwner::Manual ||
      operation->kind == CreativeTerrainOperationKind::GeneratedTerrain) {
    setTerrainStatus(
        plan, CreativeRecipeTranslationStatus::UnsupportedOwner,
        "creative_terrain_operation_translation_owner_unsupported");
    return plan;
  }
  if (deltaCells == CreativeTerrainCoord2{}) {
    plan.accepted = true;
    setTerrainStatus(plan, CreativeRecipeTranslationStatus::NoChange,
                     "creative_terrain_operation_translation_no_change");
    return plan;
  }
  if (!translateTerrainOperation(plan.translatedOperation, deltaCells)) {
    setTerrainStatus(plan, CreativeRecipeTranslationStatus::CoordinateOverflow,
                     "creative_terrain_operation_translation_overflow");
    return plan;
  }
  plan.mutationRequest = updateRequest(plan.translatedOperation);
  const std::span<const CreativeTerrainHardEdge> currentHardEdges =
      document.terrainHardEdges();
  const std::vector<CreativeTerrainHardEdge> hardEdges{
      currentHardEdges.begin(), currentHardEdges.end()};
  plan.mutationPlan = planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      plan.mutationRequest, nullptr, &hardEdges);
  if (!plan.mutationPlan.receipt.accepted) {
    setTerrainStatus(plan, CreativeRecipeTranslationStatus::PlanRejected,
                     plan.mutationPlan.receipt.reasonCode);
    return plan;
  }
  plan.accepted = true;
  plan.changed = plan.mutationPlan.receipt.changed;
  setTerrainStatus(plan,
                   plan.changed ? CreativeRecipeTranslationStatus::Planned
                                : CreativeRecipeTranslationStatus::NoChange,
                   plan.changed
                       ? "creative_terrain_operation_translation_planned"
                       : "creative_terrain_operation_translation_no_change");
  return plan;
}

CreativeTerrainOperationTranslationReceipt applyCreativeTerrainOperationTranslation(
    CreativeDocument& document,
    const CreativeTerrainOperationTranslationPlan& plan) {
  CreativeTerrainOperationTranslationReceipt receipt;
  receipt.requested = true;
  receipt.operationId = plan.operationId;
  receipt.operationKind = plan.operationKind;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (!plan.requested || !plan.accepted) {
    receipt.status = CreativeRecipeTranslationStatus::InvalidRequest;
    receipt.reasonCode = "creative_terrain_operation_translation_plan_invalid";
    return receipt;
  }
  if (!plan.changed) {
    receipt.accepted = true;
    receipt.status = CreativeRecipeTranslationStatus::NoChange;
    receipt.reasonCode = "creative_terrain_operation_translation_no_change";
    return receipt;
  }
  if (document.id() != plan.sourceDocumentId ||
      document.revision() != plan.sourceRevision) {
    receipt.status = CreativeRecipeTranslationStatus::StaleSource;
    receipt.reasonCode = "creative_terrain_operation_translation_source_stale";
    return receipt;
  }
  receipt.mutation =
      document.applyTerrainOperationMutation(plan.mutationRequest);
  if (!receipt.mutation.accepted || !receipt.mutation.changed) {
    receipt.status = CreativeRecipeTranslationStatus::ApplyRejected;
    receipt.reasonCode = receipt.mutation.reasonCode;
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeRecipeTranslationStatus::Applied;
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_terrain_operation_translation_applied";
  return receipt;
}

}  // namespace iggy3d::creative
