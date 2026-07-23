#include "app/iggy3d/creative/tools/AssetScatter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/Pattern.hpp"

namespace iggy3d::creative {
namespace {

inline constexpr std::int32_t kMaximumGridRadius = 16;
inline constexpr std::size_t kMaximumOrderedCellCount =
    static_cast<std::size_t>((kMaximumGridRadius * 2 + 1) *
                             (kMaximumGridRadius * 2 + 1));

void normalizeScatterReceiptRevisionRange(
    CreativeAssetScatterRecipeMutationReceipt& receipt,
    std::uint64_t revisionAfter) noexcept {
  receipt.revisionAfter = revisionAfter;
  if (receipt.patternMutationReceipt.requested) {
    receipt.patternMutationReceipt.revisionBefore = receipt.revisionBefore;
    receipt.patternMutationReceipt.revisionAfter = revisionAfter;
  }
}

void clearGeneratedScatterOutputs(
    CreativeAssetScatterRecipeMutationReceipt& receipt) noexcept {
  receipt.generatedObjectIds.clear();
  receipt.generatedObjectCount = 0U;
}

void clearReplacedScatterOutputs(
    CreativeAssetScatterRecipeMutationReceipt& receipt) noexcept {
  receipt.replacedGeneratedObjectIds.clear();
  receipt.replacedGeneratedObjectCount = 0U;
}

struct OrderedCell {
  std::int32_t x = 0;
  std::int32_t z = 0;
  std::uint64_t order = 0;
};

[[nodiscard]] std::uint64_t mix64(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] std::uint64_t cellHash(std::uint64_t seed,
                                     std::int32_t x,
                                     std::int32_t z,
                                     std::uint64_t stream = 0U) noexcept {
  const std::uint64_t packed =
      (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32U) |
      static_cast<std::uint32_t>(z);
  return mix64(seed ^ mix64(packed) ^ mix64(stream));
}

[[nodiscard]] double unitDouble(std::uint64_t value) noexcept {
  return static_cast<double>(value >> 11U) * 0x1.0p-53;
}

[[nodiscard]] bool validYaw(CreativeAssetScatterYaw yaw) noexcept {
  return static_cast<std::size_t>(yaw) <
         static_cast<std::size_t>(CreativeAssetScatterYaw::Count);
}

[[nodiscard]] double candidateYaw(CreativeAssetScatterYaw yaw,
                                  std::uint64_t hash) noexcept {
  switch (yaw) {
    case CreativeAssetScatterYaw::Fixed:
      return 0.0;
    case CreativeAssetScatterYaw::QuarterTurns:
      return static_cast<double>(hash & 3U) * (std::numbers::pi * 0.5);
    case CreativeAssetScatterYaw::Full:
      return unitDouble(hash) * (std::numbers::pi * 2.0);
    case CreativeAssetScatterYaw::Count:
      return 0.0;
  }
  return 0.0;
}

[[nodiscard]] bool spacingAllows(
    const CreativeAssetScatterPlan& plan,
    CreativeVec3 position,
    double spacingSquared) noexcept {
  for (const CreativeAssetScatterCandidate& candidate : plan.items()) {
    const double dx = position.x - candidate.position.x;
    const double dz = position.z - candidate.position.z;
    if (dx * dx + dz * dz < spacingSquared) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] CreativeAssetScatterCandidate makeCandidate(
    const CreativeAssetScatterRequest& request,
    CreativeVec3 position,
    std::uint64_t hash) noexcept {
  CreativeAssetScatterCandidate candidate;
  candidate.position = position;
  candidate.yawOffsetRadians =
      candidateYaw(request.yaw, mix64(hash ^ 0xa24baed4963ee407ULL));
  const double centered =
      unitDouble(mix64(hash ^ 0x9fb21c651e98df25ULL)) * 2.0 - 1.0;
  candidate.uniformScale = 1.0 + centered * request.scaleVariation;
  return candidate;
}

void appendEvaluation(
    CreativeAssetScatterPlan& plan,
    const CreativeAssetScatterCandidate& candidate,
    CreativeAssetScatterEvaluationStatus status) noexcept {
  if (plan.evaluationCount >= plan.evaluations.size()) {
    plan.truncated = true;
    return;
  }
  plan.evaluations[plan.evaluationCount++] = {candidate, status};
}

void appendCandidate(CreativeAssetScatterPlan& plan,
                     const CreativeAssetScatterCandidate& candidate) noexcept {
  plan.candidates[plan.candidateCount++] = candidate;
  appendEvaluation(plan, candidate,
                   CreativeAssetScatterEvaluationStatus::Ready);
}

[[nodiscard]] bool quantizedCoordinate(double value,
                                       double quantum,
                                       std::int64_t& output) noexcept {
  const double quantized = std::nearbyint(value / quantum);
  if (!std::isfinite(quantized) ||
      quantized < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      quantized > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  output = static_cast<std::int64_t>(quantized);
  return true;
}

}  // namespace

std::string_view toString(
    CreativeAssetScatterRecipeMutationStatus status) noexcept {
  switch (status) {
    case CreativeAssetScatterRecipeMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeAssetScatterRecipeMutationStatus::Empty: return "Empty";
    case CreativeAssetScatterRecipeMutationStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeAssetScatterRecipeMutationStatus::CreateRejected:
      return "CreateRejected";
    case CreativeAssetScatterRecipeMutationStatus::RecipeNotFound:
      return "RecipeNotFound";
    case CreativeAssetScatterRecipeMutationStatus::RecipeKindMismatch:
      return "RecipeKindMismatch";
    case CreativeAssetScatterRecipeMutationStatus::RecipeDependencyConflict:
      return "RecipeDependencyConflict";
    case CreativeAssetScatterRecipeMutationStatus::RecipeRejected:
      return "RecipeRejected";
    case CreativeAssetScatterRecipeMutationStatus::RemoveRejected:
      return "RemoveRejected";
    case CreativeAssetScatterRecipeMutationStatus::Applied: return "Applied";
  }
  return "Unknown";
}

CreativeAssetScatterPlan planCreativeAssetScatter(
    const CreativeAssetScatterRequest& request) noexcept {
  CreativeAssetScatterPlan plan;
  plan.requested = true;
  if (!isFiniteCreativeVec3(request.center) ||
      !std::isfinite(request.radiusMeters) || request.radiusMeters <= 0.0 ||
      !std::isfinite(request.spacingMeters) || request.spacingMeters <= 0.0 ||
      !std::isfinite(request.densityFraction) ||
      request.densityFraction <= 0.0 || request.densityFraction > 1.0 ||
      !std::isfinite(request.scaleVariation) ||
      request.scaleVariation < 0.0 || request.scaleVariation >= 1.0 ||
      !validYaw(request.yaw) || request.maxCandidateCount == 0U ||
      request.maxCandidateCount > plan.candidates.size()) {
    plan.status = CreativeAssetScatterStatus::InvalidRequest;
    return plan;
  }
  const double radiusInCells = request.radiusMeters / request.spacingMeters;
  if (!std::isfinite(radiusInCells) ||
      radiusInCells > static_cast<double>(kMaximumGridRadius)) {
    plan.status = CreativeAssetScatterStatus::CapacityExceeded;
    return plan;
  }

  const std::int32_t gridRadius =
      static_cast<std::int32_t>(std::ceil(radiusInCells));
  std::array<OrderedCell, kMaximumOrderedCellCount> ordered{};
  std::size_t orderedCount = 0U;
  for (std::int32_t z = -gridRadius; z <= gridRadius; ++z) {
    for (std::int32_t x = -gridRadius; x <= gridRadius; ++x) {
      if (x == 0 && z == 0) {
        continue;
      }
      ordered[orderedCount++] = {x, z, cellHash(request.seed, x, z)};
    }
  }
  std::sort(ordered.begin(), ordered.begin() + orderedCount,
            [](const OrderedCell& lhs, const OrderedCell& rhs) {
              if (lhs.order != rhs.order) {
                return lhs.order < rhs.order;
              }
              return lhs.x != rhs.x ? lhs.x < rhs.x : lhs.z < rhs.z;
            });

  appendCandidate(plan, makeCandidate(request, request.center,
                                      cellHash(request.seed, 0, 0)));
  const double radiusSquared = request.radiusMeters * request.radiusMeters;
  const double spacingSquared =
      request.spacingMeters * request.spacingMeters * (1.0 - 1.0e-10);
  constexpr double kJitterFraction = 0.28;
  for (std::size_t index = 0U; index < orderedCount; ++index) {
    const OrderedCell& cell = ordered[index];
    ++plan.examinedCellCount;
    const std::uint64_t hash = cellHash(request.seed, cell.x, cell.z);
    const double jitterX =
        (unitDouble(mix64(hash ^ 0x94d049bb133111ebULL)) * 2.0 - 1.0) *
        request.spacingMeters * kJitterFraction;
    const double jitterZ =
        (unitDouble(mix64(hash ^ 0xbf58476d1ce4e5b9ULL)) * 2.0 - 1.0) *
        request.spacingMeters * kJitterFraction;
    const CreativeVec3 position{
        request.center.x + static_cast<double>(cell.x) *
                               request.spacingMeters +
            jitterX,
        request.center.y,
        request.center.z + static_cast<double>(cell.z) *
                               request.spacingMeters +
            jitterZ,
    };
    const double dx = position.x - request.center.x;
    const double dz = position.z - request.center.z;
    if (dx * dx + dz * dz > radiusSquared) {
      continue;
    }
    const CreativeAssetScatterCandidate candidate =
        makeCandidate(request, position, hash);
    if (unitDouble(mix64(hash ^ 0xd6e8feb86659fd93ULL)) >
        request.densityFraction) {
      ++plan.densityRejectedCount;
      appendEvaluation(plan, candidate,
                       CreativeAssetScatterEvaluationStatus::DensityRejected);
      continue;
    }
    if (!spacingAllows(plan, position, spacingSquared)) {
      ++plan.spacingRejectedCount;
      appendEvaluation(plan, candidate,
                       CreativeAssetScatterEvaluationStatus::SpacingRejected);
      continue;
    }
    if (plan.candidateCount >= request.maxCandidateCount) {
      plan.truncated = true;
      appendEvaluation(plan, candidate,
                       CreativeAssetScatterEvaluationStatus::CapacityRejected);
      continue;
    }
    appendCandidate(plan, candidate);
  }

  plan.accepted = plan.candidateCount > 0U;
  plan.status = plan.accepted ? CreativeAssetScatterStatus::Ready
                              : CreativeAssetScatterStatus::Empty;
  return plan;
}

std::uint64_t creativeAssetScatterSpatialKey(CreativeVec3 position,
                                             double quantumMeters) noexcept {
  if (!isFiniteCreativeVec3(position) || !std::isfinite(quantumMeters) ||
      quantumMeters <= 0.0) {
    return 0U;
  }
  std::int64_t x = 0;
  std::int64_t y = 0;
  std::int64_t z = 0;
  if (!quantizedCoordinate(position.x, quantumMeters, x) ||
      !quantizedCoordinate(position.y, quantumMeters, y) ||
      !quantizedCoordinate(position.z, quantumMeters, z)) {
    return 0U;
  }
  return mix64(static_cast<std::uint64_t>(x)) ^
         mix64(static_cast<std::uint64_t>(y) ^ 0xa24baed4963ee407ULL) ^
         mix64(static_cast<std::uint64_t>(z) ^ 0x9fb21c651e98df25ULL);
}

namespace {

[[nodiscard]] bool validScatterMutationInput(
    std::span<const CreativeDocumentCreateRequest> createRequests,
    std::span<const CreativeObjectId> sourceObjectIds,
    const CreativeAssetScatterRecipe& recipe) noexcept {
  if (createRequests.empty() ||
      createRequests.size() > recipe.maxGeneratedObjects ||
      createRequests.size() >
          kCreativeAssetScatterGeneratedObjectCapacity ||
      !isValidCreativeAssetScatterRecipe(recipe, sourceObjectIds)) {
    return false;
  }
  return std::all_of(
      createRequests.begin(), createRequests.end(),
      [&recipe](const CreativeDocumentCreateRequest& request) {
        return request.kind == recipe.objectKind &&
               request.assetId == recipe.assetId &&
               request.assetContentHash == recipe.assetContentHash &&
               request.assetMaterialVariant == recipe.assetMaterialVariant &&
               request.hasBoundsOverride &&
               measureCreativeBounds(request.bounds).valid;
      });
}

[[nodiscard]] bool sameScatterParameters(
    const CreativeAssetScatterRecipe& lhs,
    const CreativeAssetScatterRecipe& rhs) noexcept {
  return lhs.objectKind == rhs.objectKind && lhs.assetId == rhs.assetId &&
         lhs.assetContentHash == rhs.assetContentHash &&
         lhs.assetMaterialVariant == rhs.assetMaterialVariant &&
         creativeBoundsExactlyEqual(lhs.assetSourceBounds,
                                    rhs.assetSourceBounds) &&
         lhs.mask == rhs.mask && lhs.yaw == rhs.yaw &&
         lhs.baseYawRadians == rhs.baseYawRadians &&
         lhs.radiusMeters == rhs.radiusMeters &&
         lhs.spacingMeters == rhs.spacingMeters &&
         lhs.densityFraction == rhs.densityFraction &&
         lhs.scaleVariation == rhs.scaleVariation &&
         lhs.maximumSlopeRadians == rhs.maximumSlopeRadians &&
         lhs.projectToTerrainSurface == rhs.projectToTerrainSurface &&
         lhs.avoidCollisions == rhs.avoidCollisions && lhs.seed == rhs.seed &&
         lhs.maxGeneratedObjects == rhs.maxGeneratedObjects;
}

[[nodiscard]] bool scatterRecipeExtendsByOneCenter(
    const CreativeAssetScatterRecipe& existing,
    const CreativeAssetScatterRecipe& proposed) noexcept {
  return sameScatterParameters(existing, proposed) &&
         existing.exclusions == proposed.exclusions &&
         proposed.paintCenters.size() == existing.paintCenters.size() + 1U &&
         std::equal(existing.paintCenters.begin(), existing.paintCenters.end(),
                    proposed.paintCenters.begin(), creativeVec3ExactlyEqual);
}

[[nodiscard]] bool scatterRecipeAddsOneExclusion(
    const CreativeAssetScatterRecipe& existing,
    const CreativeAssetScatterRecipe& proposed) noexcept {
  return sameScatterParameters(existing, proposed) &&
         existing.paintCenters.size() == proposed.paintCenters.size() &&
         std::equal(existing.paintCenters.begin(), existing.paintCenters.end(),
                    proposed.paintCenters.begin(), creativeVec3ExactlyEqual) &&
         proposed.exclusions.size() == existing.exclusions.size() + 1U &&
         std::equal(existing.exclusions.begin(), existing.exclusions.end(),
                    proposed.exclusions.begin());
}

[[nodiscard]] bool scatterOutputCanBeRemoved(
    const CreativeDocument& document,
    const CreativePatternRecipe& owner,
    CreativeObjectId outputObjectId,
    CreativeObjectId& failedObjectId) noexcept {
  if (std::find(owner.generatedObjectIds.begin(), owner.generatedObjectIds.end(),
                outputObjectId) == owner.generatedObjectIds.end() ||
      document.findObject(outputObjectId) == nullptr) {
    failedObjectId = outputObjectId;
    return false;
  }
  for (const CreativeObject& object : document.objects()) {
    if (object.parentId == outputObjectId) {
      failedObjectId = object.id;
      return false;
    }
  }
  for (const CreativePatternRecipe& recipe :
       document.patternRecipeStore().recipes) {
    if (recipe.id != owner.id &&
        std::find(recipe.sourceObjectIds.begin(), recipe.sourceObjectIds.end(),
                  outputObjectId) != recipe.sourceObjectIds.end()) {
      failedObjectId = outputObjectId;
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool createScatterOutputs(
    CreativeDocument& staged,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    CreativeAssetScatterRecipeMutationReceipt& receipt) {
  receipt.generatedObjectIds.reserve(createRequests.size());
  for (const CreativeDocumentCreateRequest& request : createRequests) {
    const CreativeDocumentCreateReceipt created = staged.createObject(request);
    if (!created.accepted || !created.changed || !created.objectCreated) {
      receipt.failedObjectId = created.objectId;
      receipt.status =
          CreativeAssetScatterRecipeMutationStatus::CreateRejected;
      receipt.message = std::string{created.reasonCode};
      receipt.generatedObjectIds.clear();
      return false;
    }
    receipt.generatedObjectIds.push_back(created.objectId);
  }
  receipt.generatedObjectCount = receipt.generatedObjectIds.size();
  return true;
}

void unlockScatterOutputs(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds) noexcept {
  for (CreativeObjectId objectId : objectIds) {
    CreativeObject* object = document.findObject(objectId);
    if (object != nullptr) {
      object->locked = false;
    }
  }
}

}  // namespace

CreativeAssetScatterRecipeMutationReceipt
createCreativeAssetScatterRecipeAtomically(
    CreativeDocument& document,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    std::span<const CreativeObjectId> selectionFilterObjectIds,
    const CreativeAssetScatterRecipe& recipe) {
  CreativeAssetScatterRecipeMutationReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = createRequests.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (createRequests.empty()) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::Empty;
    receipt.message = "creative_asset_scatter_recipe_empty";
    return receipt;
  }
  if (!validScatterMutationInput(createRequests, selectionFilterObjectIds,
                                 recipe)) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::InvalidRequest;
    receipt.message = "creative_asset_scatter_recipe_request_invalid";
    return receipt;
  }

  CreativeDocument staged = document;
  if (!createScatterOutputs(staged, createRequests, receipt)) {
    return receipt;
  }
  CreativePatternRecipe relationship;
  relationship.kind = CreativePatternRecipeKind::AssetScatter;
  relationship.sourceObjectIds.assign(selectionFilterObjectIds.begin(),
                                      selectionFilterObjectIds.end());
  relationship.generatedObjectIds = receipt.generatedObjectIds;
  relationship.scatter = recipe;
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Add;
  mutation.recipe = std::move(relationship);
  receipt.patternMutationReceipt = staged.applyPatternRecipeMutation(mutation);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeRejected;
    receipt.message =
        std::string{receipt.patternMutationReceipt.reasonCode};
    clearGeneratedScatterOutputs(receipt);
    normalizeScatterReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  receipt.patternRecipeId = receipt.patternMutationReceipt.recipeId;
  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeScatterReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeRejected;
    receipt.message = std::string{publication.reasonCode};
    receipt.patternRecipeId = kInvalidCreativePatternRecipeId;
    clearGeneratedScatterOutputs(receipt);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeAssetScatterRecipeMutationStatus::Applied;
  receipt.message = "creative_asset_scatter_recipe_created";
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt
updateCreativeAssetScatterRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    const CreativeAssetScatterRecipe& recipe) {
  CreativeAssetScatterRecipeMutationReceipt receipt;
  receipt.requested = true;
  receipt.patternRecipeId = recipeId;
  receipt.requestedObjectCount = createRequests.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  const CreativePatternRecipe* existing =
      findCreativePatternRecipe(document.patternRecipeStore(), recipeId);
  if (existing == nullptr) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeNotFound;
    receipt.message = "creative_asset_scatter_recipe_not_found";
    return receipt;
  }
  if (existing->kind != CreativePatternRecipeKind::AssetScatter) {
    receipt.status =
        CreativeAssetScatterRecipeMutationStatus::RecipeKindMismatch;
    receipt.message = "creative_asset_scatter_recipe_kind_mismatch";
    return receipt;
  }
  if (!validScatterMutationInput(createRequests, existing->sourceObjectIds,
                                 recipe)) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::InvalidRequest;
    receipt.message = "creative_asset_scatter_recipe_request_invalid";
    return receipt;
  }
  const CreativePatternReplacementPreflight preflight =
      preflightCreativePatternReplacement(document, *existing);
  if (!preflight.accepted) {
    receipt.failedObjectId = preflight.failedObjectId;
    receipt.status = CreativeAssetScatterRecipeMutationStatus::
        RecipeDependencyConflict;
    receipt.message = std::string{preflight.reasonCode};
    return receipt;
  }

  const std::vector<CreativeObjectId> oldGeneratedObjectIds =
      existing->generatedObjectIds;
  const std::vector<CreativeObjectId> sourceObjectIds =
      existing->sourceObjectIds;
  receipt.replacedGeneratedObjectCount = oldGeneratedObjectIds.size();
  receipt.replacedGeneratedObjectIds = oldGeneratedObjectIds;
  CreativeDocument staged = document;
  if (!createScatterOutputs(staged, createRequests, receipt)) {
    clearReplacedScatterOutputs(receipt);
    return receipt;
  }
  CreativePatternRecipe replacement;
  replacement.kind = CreativePatternRecipeKind::AssetScatter;
  replacement.sourceObjectIds = sourceObjectIds;
  replacement.generatedObjectIds = receipt.generatedObjectIds;
  replacement.scatter = recipe;
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Replace;
  mutation.recipeId = recipeId;
  mutation.recipe = std::move(replacement);
  receipt.patternMutationReceipt = staged.applyPatternRecipeMutation(mutation);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeRejected;
    receipt.message =
        std::string{receipt.patternMutationReceipt.reasonCode};
    clearGeneratedScatterOutputs(receipt);
    clearReplacedScatterOutputs(receipt);
    normalizeScatterReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  unlockScatterOutputs(staged, oldGeneratedObjectIds);
  const CreativeHierarchyBatchRemoveReceipt removed =
      removeCreativeObjectHierarchiesAtomically(staged,
                                                preflight.rootObjectIds);
  if (!removed.accepted || !removed.changed ||
      removed.removedObjectIds.size() != oldGeneratedObjectIds.size()) {
    receipt.failedObjectId = removed.failedObjectId;
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RemoveRejected;
    receipt.message = std::string{removed.reasonCode};
    clearGeneratedScatterOutputs(receipt);
    clearReplacedScatterOutputs(receipt);
    normalizeScatterReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeScatterReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RemoveRejected;
    receipt.message = std::string{publication.reasonCode};
    clearGeneratedScatterOutputs(receipt);
    clearReplacedScatterOutputs(receipt);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.updatedExistingRecipe = true;
  receipt.status = CreativeAssetScatterRecipeMutationStatus::Applied;
  receipt.message = "creative_asset_scatter_recipe_updated";
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt
extendCreativeAssetScatterRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    const CreativeAssetScatterRecipe& recipe) {
  CreativeAssetScatterRecipeMutationReceipt receipt;
  receipt.requested = true;
  receipt.patternRecipeId = recipeId;
  receipt.requestedObjectCount = createRequests.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  const CreativePatternRecipe* existing =
      findCreativePatternRecipe(document.patternRecipeStore(), recipeId);
  if (existing == nullptr) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeNotFound;
    receipt.message = "creative_asset_scatter_recipe_not_found";
    return receipt;
  }
  if (existing->kind != CreativePatternRecipeKind::AssetScatter) {
    receipt.status =
        CreativeAssetScatterRecipeMutationStatus::RecipeKindMismatch;
    receipt.message = "creative_asset_scatter_recipe_kind_mismatch";
    return receipt;
  }
  if (createRequests.empty()) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::Empty;
    receipt.message = "creative_asset_scatter_extension_empty";
    return receipt;
  }
  if (!scatterRecipeExtendsByOneCenter(existing->scatter, recipe) ||
      existing->generatedObjectIds.size() + createRequests.size() >
          recipe.maxGeneratedObjects ||
      !validScatterMutationInput(createRequests, existing->sourceObjectIds,
                                 recipe)) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::InvalidRequest;
    receipt.message = "creative_asset_scatter_extension_invalid";
    return receipt;
  }

  CreativeDocument staged = document;
  if (!createScatterOutputs(staged, createRequests, receipt)) {
    return receipt;
  }
  CreativePatternRecipe replacement = *existing;
  replacement.scatter = recipe;
  replacement.generatedObjectIds.insert(replacement.generatedObjectIds.end(),
                                        receipt.generatedObjectIds.begin(),
                                        receipt.generatedObjectIds.end());
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Replace;
  mutation.recipeId = recipeId;
  mutation.recipe = std::move(replacement);
  receipt.patternMutationReceipt = staged.applyPatternRecipeMutation(mutation);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeRejected;
    receipt.message =
        std::string{receipt.patternMutationReceipt.reasonCode};
    clearGeneratedScatterOutputs(receipt);
    normalizeScatterReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeScatterReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeRejected;
    receipt.message = std::string{publication.reasonCode};
    clearGeneratedScatterOutputs(receipt);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.updatedExistingRecipe = true;
  receipt.status = CreativeAssetScatterRecipeMutationStatus::Applied;
  receipt.message = "creative_asset_scatter_recipe_extended";
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt
excludeCreativeAssetScatterOutputAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    CreativeObjectId outputObjectId,
    const CreativeAssetScatterRecipe& recipe) {
  CreativeAssetScatterRecipeMutationReceipt receipt;
  receipt.requested = true;
  receipt.patternRecipeId = recipeId;
  receipt.requestedObjectCount = 1U;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  const CreativePatternRecipe* existing =
      findCreativePatternRecipe(document.patternRecipeStore(), recipeId);
  if (existing == nullptr) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeNotFound;
    receipt.message = "creative_asset_scatter_recipe_not_found";
    return receipt;
  }
  if (existing->kind != CreativePatternRecipeKind::AssetScatter) {
    receipt.status =
        CreativeAssetScatterRecipeMutationStatus::RecipeKindMismatch;
    receipt.message = "creative_asset_scatter_recipe_kind_mismatch";
    return receipt;
  }
  if (!scatterRecipeAddsOneExclusion(existing->scatter, recipe) ||
      !isValidCreativeAssetScatterRecipe(recipe,
                                         existing->sourceObjectIds)) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::InvalidRequest;
    receipt.message = "creative_asset_scatter_exclusion_invalid";
    return receipt;
  }
  if (existing->generatedObjectIds.size() == 1U &&
      existing->generatedObjectIds.front() == outputObjectId) {
    return removeCreativeAssetScatterRecipeAtomically(document, recipeId);
  }
  if (!scatterOutputCanBeRemoved(document, *existing, outputObjectId,
                                 receipt.failedObjectId)) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::
        RecipeDependencyConflict;
    receipt.message = "creative_asset_scatter_output_dependency_conflict";
    return receipt;
  }

  CreativePatternRecipe replacement = *existing;
  replacement.scatter = recipe;
  replacement.generatedObjectIds.erase(
      std::remove(replacement.generatedObjectIds.begin(),
                  replacement.generatedObjectIds.end(), outputObjectId),
      replacement.generatedObjectIds.end());
  CreativeDocument staged = document;
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Replace;
  mutation.recipeId = recipeId;
  mutation.recipe = std::move(replacement);
  receipt.patternMutationReceipt = staged.applyPatternRecipeMutation(mutation);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeRejected;
    receipt.message =
        std::string{receipt.patternMutationReceipt.reasonCode};
    clearReplacedScatterOutputs(receipt);
    normalizeScatterReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }
  unlockScatterOutputs(staged, std::span{&outputObjectId, 1U});
  const CreativeHierarchyBatchRemoveReceipt removed =
      removeCreativeObjectHierarchiesAtomically(
          staged, std::span{&outputObjectId, 1U});
  if (!removed.accepted || !removed.changed ||
      removed.removedObjectIds.size() != 1U) {
    receipt.failedObjectId = removed.failedObjectId;
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RemoveRejected;
    receipt.message = std::string{removed.reasonCode};
    clearReplacedScatterOutputs(receipt);
    normalizeScatterReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  receipt.replacedGeneratedObjectIds.push_back(outputObjectId);
  receipt.replacedGeneratedObjectCount = 1U;
  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeScatterReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RemoveRejected;
    receipt.message = std::string{publication.reasonCode};
    receipt.replacedGeneratedObjectIds.clear();
    receipt.replacedGeneratedObjectCount = 0U;
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.updatedExistingRecipe = true;
  receipt.status = CreativeAssetScatterRecipeMutationStatus::Applied;
  receipt.message = "creative_asset_scatter_output_excluded";
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt
removeCreativeAssetScatterRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId) {
  CreativeAssetScatterRecipeMutationReceipt receipt;
  receipt.requested = true;
  receipt.patternRecipeId = recipeId;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  const CreativePatternRecipe* existing =
      findCreativePatternRecipe(document.patternRecipeStore(), recipeId);
  if (existing == nullptr) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeNotFound;
    receipt.message = "creative_asset_scatter_recipe_not_found";
    return receipt;
  }
  if (existing->kind != CreativePatternRecipeKind::AssetScatter) {
    receipt.status =
        CreativeAssetScatterRecipeMutationStatus::RecipeKindMismatch;
    receipt.message = "creative_asset_scatter_recipe_kind_mismatch";
    return receipt;
  }
  const CreativePatternReplacementPreflight preflight =
      preflightCreativePatternReplacement(document, *existing);
  if (!preflight.accepted) {
    receipt.failedObjectId = preflight.failedObjectId;
    receipt.status = CreativeAssetScatterRecipeMutationStatus::
        RecipeDependencyConflict;
    receipt.message = std::string{preflight.reasonCode};
    return receipt;
  }

  receipt.replacedGeneratedObjectIds = existing->generatedObjectIds;
  receipt.replacedGeneratedObjectCount =
      receipt.replacedGeneratedObjectIds.size();
  CreativeDocument staged = document;
  receipt.patternMutationReceipt =
      detachCreativePatternRecipe(staged, recipeId);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RecipeRejected;
    receipt.message =
        std::string{receipt.patternMutationReceipt.reasonCode};
    clearReplacedScatterOutputs(receipt);
    normalizeScatterReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }
  unlockScatterOutputs(staged, receipt.replacedGeneratedObjectIds);
  const CreativeHierarchyBatchRemoveReceipt removed =
      removeCreativeObjectHierarchiesAtomically(staged,
                                                preflight.rootObjectIds);
  if (!removed.accepted || !removed.changed ||
      removed.removedObjectIds.size() !=
          receipt.replacedGeneratedObjectIds.size()) {
    receipt.failedObjectId = removed.failedObjectId;
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RemoveRejected;
    receipt.message = std::string{removed.reasonCode};
    clearReplacedScatterOutputs(receipt);
    normalizeScatterReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeScatterReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.status = CreativeAssetScatterRecipeMutationStatus::RemoveRejected;
    receipt.message = std::string{publication.reasonCode};
    receipt.replacedGeneratedObjectIds.clear();
    receipt.replacedGeneratedObjectCount = 0U;
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.updatedExistingRecipe = true;
  receipt.status = CreativeAssetScatterRecipeMutationStatus::Applied;
  receipt.message = "creative_asset_scatter_recipe_removed";
  return receipt;
}

}  // namespace iggy3d::creative
