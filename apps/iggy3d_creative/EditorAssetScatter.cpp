#include "EditorAssetScatter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <numeric>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "EditorGroup.hpp"
#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacementClearance.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] std::uint64_t stableStringHash(std::string_view value) noexcept {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const char character : value) {
    hash ^= static_cast<unsigned char>(character);
    hash *= 1099511628211ULL;
  }
  return hash;
}

[[nodiscard]] std::uint64_t scatterSeed(
    const cr::CreativeHotbarEntry& held,
    cr::CreativeVec3 center,
    double cellSize) noexcept {
  return stableStringHash(cr::creativeHotbarAssetId(held)) ^
         cr::creativeAssetScatterSpatialKey(center, cellSize);
}

[[nodiscard]] cr::CreativeAssetScatterRecipeMask recipeMask(
    cr::CreativeAssetScatterMask mask) noexcept {
  switch (mask) {
    case cr::CreativeAssetScatterMask::Circle:
      return cr::CreativeAssetScatterRecipeMask::Circle;
    case cr::CreativeAssetScatterMask::Box:
      return cr::CreativeAssetScatterRecipeMask::Box;
    case cr::CreativeAssetScatterMask::Selection:
      return cr::CreativeAssetScatterRecipeMask::Selection;
    case cr::CreativeAssetScatterMask::Count:
      return cr::CreativeAssetScatterRecipeMask::Count;
  }
  return cr::CreativeAssetScatterRecipeMask::Count;
}

[[nodiscard]] cr::CreativeAssetScatterYaw runtimeYaw(
    cr::CreativeAssetScatterRecipeYaw yaw) noexcept {
  switch (yaw) {
    case cr::CreativeAssetScatterRecipeYaw::Fixed:
      return cr::CreativeAssetScatterYaw::Fixed;
    case cr::CreativeAssetScatterRecipeYaw::QuarterTurns:
      return cr::CreativeAssetScatterYaw::QuarterTurns;
    case cr::CreativeAssetScatterRecipeYaw::Full:
      return cr::CreativeAssetScatterYaw::Full;
    case cr::CreativeAssetScatterRecipeYaw::Count:
      return cr::CreativeAssetScatterYaw::Count;
  }
  return cr::CreativeAssetScatterYaw::Count;
}

[[nodiscard]] cr::CreativeAssetScatterRecipeYaw recipeYaw(
    cr::CreativeAssetScatterYaw yaw) noexcept {
  switch (yaw) {
    case cr::CreativeAssetScatterYaw::Fixed:
      return cr::CreativeAssetScatterRecipeYaw::Fixed;
    case cr::CreativeAssetScatterYaw::QuarterTurns:
      return cr::CreativeAssetScatterRecipeYaw::QuarterTurns;
    case cr::CreativeAssetScatterYaw::Full:
      return cr::CreativeAssetScatterRecipeYaw::Full;
    case cr::CreativeAssetScatterYaw::Count:
      return cr::CreativeAssetScatterRecipeYaw::Count;
  }
  return cr::CreativeAssetScatterRecipeYaw::Count;
}

[[nodiscard]] std::vector<cr::CreativeObjectId>
selectedHierarchyObjectIds(const cr::CreativeAppState& appState) {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  std::vector<cr::CreativeObjectId> selected;
  const std::span<const cr::TargetRef> targets =
      cr::selectedTargetList(selection);
  selected.reserve(targets.empty() ? 1U : targets.size());
  for (cr::TargetRef target : targets) {
    if (target.value != cr::kInvalidId) {
      selected.push_back(static_cast<cr::CreativeObjectId>(target.value));
    }
  }
  if (selected.empty() && selection.selectedTarget.value != cr::kInvalidId) {
    selected.push_back(static_cast<cr::CreativeObjectId>(
        selection.selectedTarget.value));
  }
  const cr::CreativeHierarchySelection hierarchy =
      cr::resolveCreativeObjectHierarchy(appState.facade.document(), selected);
  return hierarchy.accepted ? hierarchy.objectIds : selected;
}

[[nodiscard]] cr::CreativeGridTarget syntheticScatterTarget(
    const CreativeEditorState& editor,
    cr::CreativeVec3 position,
    bool terrainSurface) noexcept {
  cr::CreativeGridTarget target;
  if (!cr::isFiniteCreativeVec3(position) ||
      !std::isfinite(editor.placeCellSize) || editor.placeCellSize <= 0.0) {
    return target;
  }
  const double half = editor.placeCellSize * 0.5;
  target.hitPoint = position;
  target.faceNormal = {0.0, 1.0, 0.0};
  target.placerForward = editor.interaction.target.grid.placerForward;
  target.placementAnchor = position;
  target.adjacentCellBounds = {
      {position.x - half, position.y, position.z - half},
      {position.x + half, position.y + editor.placeCellSize,
       position.z + half},
  };
  target.targetCellBounds = {
      {position.x - half, position.y - editor.placeCellSize,
       position.z - half},
      {position.x + half, position.y, position.z + half},
  };
  target.targetFacts = cr::makeCreativePlacementTargetFacts(
      terrainSurface
          ? cr::CreativePlacementTargetSource::Terrain
          : cr::CreativePlacementTargetSource::EmptyPlane,
      terrainSurface
          ? cr::CreativeObjectKind::TerrainPatch
          : cr::CreativeObjectKind::Unknown);
  target.valid = true;
  return target;
}

[[nodiscard]] bool existingAssetWithinSpacing(
    const cr::CreativeDocument& document,
    std::string_view assetId,
    const CreativeBrushPlacementPlan& placement,
    double spacingMeters,
    std::span<const cr::CreativeObjectId> ignoredObjectIds) noexcept {
  const cr::CreativeTransformedBounds candidate =
      cr::resolveCreativeTransformedBounds(placement.previewBounds,
                                           placement.transform);
  if (!candidate.valid) {
    return true;
  }
  const double spacingSquared = spacingMeters * spacingMeters;
  for (const cr::CreativeObject& object : document.objects()) {
    if (object.assetId != assetId ||
        std::find(ignoredObjectIds.begin(), ignoredObjectIds.end(), object.id) !=
            ignoredObjectIds.end()) {
      continue;
    }
    const cr::CreativeObjectWorldExtent extent =
        cr::resolveCreativeObjectWorldExtent(object);
    if (!extent.valid) {
      continue;
    }
    const double centerX = std::midpoint(extent.min.x, extent.max.x);
    const double centerZ = std::midpoint(extent.min.z, extent.max.z);
    const double dx = centerX - candidate.center.x;
    const double dz = centerZ - candidate.center.z;
    if (dx * dx + dz * dz < spacingSquared) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool pointInsideSelectionMask(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> selectionFilterObjectIds,
    cr::CreativeVec3 position) noexcept {
  for (cr::CreativeObjectId objectId : selectionFilterObjectIds) {
    const cr::CreativeObject* object = document.findObject(objectId);
    if (object == nullptr) {
      continue;
    }
    const cr::CreativeObjectWorldExtent extent =
        cr::resolveCreativeObjectWorldExtent(*object);
    if (extent.valid && position.x >= extent.min.x &&
        position.x <= extent.max.x && position.z >= extent.min.z &&
        position.z <= extent.max.z) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool candidateInsideMask(
    const cr::CreativeDocument& document,
    const cr::CreativeAssetScatterRecipe& recipe,
    std::span<const cr::CreativeObjectId> selectionFilterObjectIds,
    cr::CreativeVec3 center,
    cr::CreativeVec3 position) noexcept {
  switch (recipe.mask) {
    case cr::CreativeAssetScatterRecipeMask::Circle: {
      const double dx = position.x - center.x;
      const double dz = position.z - center.z;
      return dx * dx + dz * dz <=
             recipe.radiusMeters * recipe.radiusMeters + 1.0e-10;
    }
    case cr::CreativeAssetScatterRecipeMask::Box:
      return std::fabs(position.x - center.x) <=
                 recipe.radiusMeters + 1.0e-10 &&
             std::fabs(position.z - center.z) <=
                 recipe.radiusMeters + 1.0e-10;
    case cr::CreativeAssetScatterRecipeMask::Selection:
      return pointInsideSelectionMask(document, selectionFilterObjectIds,
                                      position);
    case cr::CreativeAssetScatterRecipeMask::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool candidateExcluded(
    const cr::CreativeAssetScatterRecipe& recipe,
    cr::CreativeVec3 position) noexcept {
  for (const cr::CreativeAssetScatterExclusion& exclusion :
       recipe.exclusions) {
    const double dx = position.x - exclusion.center.x;
    const double dz = position.z - exclusion.center.z;
    if (dx * dx + dz * dz <=
        exclusion.radiusMeters * exclusion.radiusMeters) {
      return true;
    }
  }
  return false;
}

enum class ScatterPlanConflict : std::uint8_t {
  None,
  Spacing,
  Collision,
};

[[nodiscard]] ScatterPlanConflict candidateConflictWithPlan(
    const CreativeEditorAssetScatterPlan& plan,
    const CreativeEditorAssetScatterCandidate& candidate,
    double spacingMeters,
    bool avoidCollisions) noexcept {
  const cr::CreativeTransformedBounds candidateBounds =
      cr::resolveCreativeTransformedBounds(candidate.placement.previewBounds,
                                           candidate.placement.transform);
  const double spacingSquared = spacingMeters * spacingMeters *
                                (1.0 - 1.0e-10);
  for (const CreativeEditorAssetScatterCandidate& existing : plan.items()) {
    if (!existing.placeable) {
      continue;
    }
    const double dx = candidate.surfacePosition.x - existing.surfacePosition.x;
    const double dz = candidate.surfacePosition.z - existing.surfacePosition.z;
    if (dx * dx + dz * dz < spacingSquared) {
      return ScatterPlanConflict::Spacing;
    }
    if (!avoidCollisions || !candidateBounds.valid) {
      continue;
    }
    const cr::CreativeTransformedBounds existingBounds =
        cr::resolveCreativeTransformedBounds(existing.placement.previewBounds,
                                             existing.placement.transform);
    if (existingBounds.valid &&
        candidateBounds.worldBounds.min.x < existingBounds.worldBounds.max.x &&
        candidateBounds.worldBounds.max.x > existingBounds.worldBounds.min.x &&
        candidateBounds.worldBounds.min.y < existingBounds.worldBounds.max.y &&
        candidateBounds.worldBounds.max.y > existingBounds.worldBounds.min.y &&
        candidateBounds.worldBounds.min.z < existingBounds.worldBounds.max.z &&
        candidateBounds.worldBounds.max.z > existingBounds.worldBounds.min.z) {
      return ScatterPlanConflict::Collision;
    }
  }
  return ScatterPlanConflict::None;
}

[[nodiscard]] CreativeEditorAssetScatterCandidate buildEditorCandidate(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeAssetScatterRecipe& recipe,
    const cr::CreativeAssetScatterCandidate& kernelCandidate,
    std::span<const cr::CreativeObjectId> ignoredObjectIds,
    const CreativePlacementClearanceCache* clearanceCache) noexcept {
  CreativeEditorAssetScatterCandidate candidate;
  candidate.surfacePosition = kernelCandidate.position;
  bool surfaceAllowed = true;
  if (recipe.projectToTerrainSurface) {
    const cr::CreativeGridSettings grid = document.gridSettings();
    const cr::CreativeTerrainSurfacePose surface =
        cr::sampleCreativeTerrainSurfacePose(
            {&document.terrainField(), kernelCandidate.position, grid.origin,
             grid.cellSizeMeters});
    if (!surface.present) {
      candidate.status =
          CreativeEditorAssetScatterCandidateStatus::MissingSurface;
      surfaceAllowed = false;
    } else {
      candidate.surfacePosition = surface.position;
      if (surface.slopeRadians > recipe.maximumSlopeRadians + 1.0e-10) {
        candidate.status =
            CreativeEditorAssetScatterCandidateStatus::SlopeRejected;
        surfaceAllowed = false;
      }
    }
  }

  const cr::CreativeGridTarget target =
      syntheticScatterTarget(editor, candidate.surfacePosition,
                             recipe.projectToTerrainSurface);
  CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      recipe.objectKind, target, cr::CreativePlacementYaw::Degrees0);
  if (!admission.allowed) {
    candidate.status =
        CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
    return candidate;
  }
  admission.plan.transform.rotationEulerRadians.y +=
      recipe.baseYawRadians + kernelCandidate.yawOffsetRadians;
  admission.plan.transform.scale = {kernelCandidate.uniformScale,
                                    kernelCandidate.uniformScale,
                                    kernelCandidate.uniformScale};
  if (!applyCreativeAssetPlacementBounds(admission.plan,
                                         recipe.assetSourceBounds)) {
    candidate.status =
        CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
    return candidate;
  }
  candidate.placement = admission.plan;
  candidate.visitedKey = cr::creativeAssetScatterSpatialKey(
      candidate.surfacePosition, std::max(0.01, recipe.spacingMeters * 0.5));
  if (!surfaceAllowed) {
    return candidate;
  }
  applyCreativeBrushPlacementClearance(admission, document, clearanceCache,
                                       ignoredObjectIds);
  candidate.placement = admission.plan;
  if (!admission.allowed) {
    if (!recipe.avoidCollisions &&
        admission.plan.clearance.status !=
            cr::CreativePlacementClearanceStatus::OutsideWorldBounds &&
        admission.plan.clearance.status !=
            cr::CreativePlacementClearanceStatus::InvalidRequest) {
      candidate.placement.clearance.status =
          cr::CreativePlacementClearanceStatus::Ready;
      candidate.placement.clearance.allowed = true;
    } else {
      candidate.status =
          admission.status ==
                  CreativeBrushPlacementAdmissionStatus::ClearanceBlocked
              ? CreativeEditorAssetScatterCandidateStatus::Obstructed
              : CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
      return candidate;
    }
  }
  if (existingAssetWithinSpacing(document, recipe.assetId,
                                 candidate.placement, recipe.spacingMeters,
                                 ignoredObjectIds)) {
    candidate.status = CreativeEditorAssetScatterCandidateStatus::Occupied;
    return candidate;
  }
  candidate.status = CreativeEditorAssetScatterCandidateStatus::Ready;
  candidate.placeable = true;
  return candidate;
}

[[nodiscard]] CreativeEditorAssetScatterCandidateStatus
editorKernelRejectionStatus(
    cr::CreativeAssetScatterEvaluationStatus status) noexcept {
  switch (status) {
    case cr::CreativeAssetScatterEvaluationStatus::DensityRejected:
      return CreativeEditorAssetScatterCandidateStatus::DensityRejected;
    case cr::CreativeAssetScatterEvaluationStatus::SpacingRejected:
      return CreativeEditorAssetScatterCandidateStatus::SpacingRejected;
    case cr::CreativeAssetScatterEvaluationStatus::CapacityRejected:
      return CreativeEditorAssetScatterCandidateStatus::CapacityRejected;
    case cr::CreativeAssetScatterEvaluationStatus::Ready:
    case cr::CreativeAssetScatterEvaluationStatus::Count:
      return CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
  }
  return CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
}

[[nodiscard]] CreativeEditorAssetScatterRejectedCandidate
buildKernelRejectedPreview(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeAssetScatterRecipe& recipe,
    const cr::CreativeAssetScatterEvaluation& evaluation) noexcept {
  CreativeEditorAssetScatterRejectedCandidate rejected;
  rejected.status = editorKernelRejectionStatus(evaluation.status);
  cr::CreativeVec3 surfacePosition = evaluation.candidate.position;
  if (recipe.projectToTerrainSurface) {
    const cr::CreativeGridSettings grid = document.gridSettings();
    const cr::CreativeTerrainSurfacePose surface =
        cr::sampleCreativeTerrainSurfacePose(
            {&document.terrainField(), evaluation.candidate.position,
             grid.origin, grid.cellSizeMeters});
    if (surface.present) {
      surfacePosition = surface.position;
    }
  }
  const cr::CreativeGridTarget target = syntheticScatterTarget(
      editor, surfacePosition, recipe.projectToTerrainSurface);
  CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      recipe.objectKind, target, cr::CreativePlacementYaw::Degrees0);
  if (!admission.allowed) {
    return rejected;
  }
  admission.plan.transform.rotationEulerRadians.y +=
      recipe.baseYawRadians + evaluation.candidate.yawOffsetRadians;
  admission.plan.transform.scale = {evaluation.candidate.uniformScale,
                                    evaluation.candidate.uniformScale,
                                    evaluation.candidate.uniformScale};
  if (!applyCreativeAssetPlacementBounds(admission.plan,
                                         recipe.assetSourceBounds)) {
    return rejected;
  }
  const cr::CreativeTransformedBounds bounds =
      cr::resolveCreativeTransformedBounds(admission.plan.previewBounds,
                                           admission.plan.transform);
  if (!bounds.valid) {
    return rejected;
  }
  rejected.worldBounds = bounds.worldBounds;
  rejected.valid = true;
  return rejected;
}

void rejectScatter(CreativeEditorState& editor,
                   cr::CreativeObjectKind objectKind,
                   const cr::CreativePlacementClearanceResult& clearance = {},
                   CreativeEditorPlacementRejectionReason reason =
                       CreativeEditorPlacementRejectionReason::ActionRejected) {
  setCreativeEditorPlacementRejectionFeedback(
      editor.interaction, editor.frameIndex, objectKind, clearance, reason);
}

[[nodiscard]] bool ensureScatterTransaction(
    cr::CreativeAppState& appState,
    CreativeAssetScatterStrokeState& stroke,
    cr::CreativeWorldGestureKind kind) {
  if (stroke.transaction.active) {
    return true;
  }
  stroke.transaction = beginEditTransaction(
      appState.facade,
      kind == cr::CreativeWorldGestureKind::Remove
          ? "creative_asset_scatter_remove_stroke"
          : "creative_asset_scatter_place_stroke");
  return stroke.transaction.active;
}

[[nodiscard]] bool setScatterTransactionOperation(
    CreativeAssetScatterStrokeState& stroke,
    cr::CreativePatternRecipeId recipeId,
    std::span<const cr::CreativeObjectId> sourceObjectIds,
    const cr::CreativeAssetScatterRecipe& recipe,
    cr::CreativeAuthoringOperationKind kind,
    std::string_view action,
    std::uint64_t additionalAffectedMembers) {
  if (!stroke.transaction.active) {
    return false;
  }
  cr::CreativePatternRecipe source;
  source.id = recipeId;
  source.kind = cr::CreativePatternRecipeKind::AssetScatter;
  source.sourceObjectIds.assign(sourceObjectIds.begin(), sourceObjectIds.end());
  source.scatter = recipe;
  const std::uint64_t affectedBefore =
      stroke.transaction.operation.has_value()
          ? stroke.transaction.operation->affectedMemberCount
          : 0U;
  const cr::CreativeAuthoringOperationKind transactionKind =
      stroke.transaction.operation.has_value()
          ? stroke.transaction.operation->kind
          : kind;
  return setEditTransactionOperation(
      stroke.transaction, cr::CreativeAuthoringFamily::AssetScatter,
      transactionKind, action,
      cr::fingerprintCreativePatternRecipeSource(source),
      affectedBefore + additionalAffectedMembers);
}

[[nodiscard]] std::uint64_t removalVisitedKey(
    const CreativeEditorWorldTarget& target) noexcept {
  if (target.objectHit) {
    return 0x8000000000000000ULL ^ target.objectId;
  }
  if (!target.voxelHit) {
    return 0U;
  }
  const cr::CreativeVec3 cell{
      static_cast<double>(target.voxelCell.x),
      static_cast<double>(target.voxelCell.y),
      static_cast<double>(target.voxelCell.z),
  };
  return 0x4000000000000000ULL ^
         cr::creativeAssetScatterSpatialKey(cell, 1.0);
}

enum class ScatterRecipeEraseResult : std::uint8_t {
  NotScatterRecipe,
  Applied,
  Rejected,
};

[[nodiscard]] ScatterRecipeEraseResult applyScatterRecipeExclusion(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor);

void applyScatterRemoval(cr::CreativeAppState& appState,
                         CreativeEditorState& editor,
                         const cr::CreativeHotbarEntry& held) {
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if (target.objectHit) {
    const ScatterRecipeEraseResult recipeErase =
        applyScatterRecipeExclusion(appState, editor);
    if (recipeErase != ScatterRecipeEraseResult::NotScatterRecipe) {
      return;
    }
  }
  const std::uint64_t key = removalVisitedKey(target);
  if (stroke.capacityReached || key == 0U ||
      cr::creativeWorldGestureVisited(stroke.visited, key)) {
    if (key == 0U || stroke.capacityReached) {
      rejectScatter(editor, target.objectKind);
    }
    return;
  }
  if (target.objectHit) {
    const cr::CreativeSemanticObjectActionAdmission deleteAdmission =
        cr::resolveCreativeSemanticObjectActionAdmission(
            appState.facade.document(), target.objectId,
            cr::CreativeSemanticObjectAction::Delete,
            &editor.worldLayout.source,
            editor.worldLayout.generatedRevision ==
                editor.worldLayout.revision);
    if (!cr::creativeSemanticActionUsesDocumentMutation(deleteAdmission)) {
      rejectScatter(
          editor, target.objectKind, {},
          CreativeEditorPlacementRejectionReason::SemanticSourceOwned);
      return;
    }
  }
  if (!ensureScatterTransaction(appState, stroke,
                                cr::CreativeWorldGestureKind::Remove)) {
    rejectScatter(editor, target.objectKind);
    return;
  }
  bool changed = false;
  if (target.voxelHit) {
    const cr::CreativeVoxelEdit edit{target.voxelCell,
                                     cr::CreativeObjectKind::Unknown};
    const cr::CreativeVoxelMutationReceipt receipt =
        appState.facade.applyVoxelEdits(std::span{&edit, 1U});
    changed = receipt.accepted && receipt.changed;
  } else {
    const cr::CreativeSemanticDeleteReceipt receipt =
        appState.facade.deleteDocumentObjectsSemantically(
            std::span{&target.objectId, 1U});
    changed = receipt.accepted && receipt.changed;
    if (!changed && receipt.status ==
                        cr::CreativeSemanticDeleteStatus::ExternalReference) {
      rejectScatter(editor, target.objectKind, {},
                    CreativeEditorPlacementRejectionReason::ExternalReference);
      return;
    }
  }
  if (!changed || cr::rememberCreativeWorldGestureKey(stroke.visited, key) !=
                      cr::CreativeWorldGestureVisitStatus::Inserted) {
    rejectScatter(editor, held.objectKind);
    return;
  }
  ++stroke.acceptedMutationCount;
  stroke.capacityReached = stroke.visited.count >= stroke.visited.keys.size();
  clearCreativeEditorPlacementFeedback(editor.interaction);
}

[[nodiscard]] const cr::CreativePatternRecipe* activeScatterPattern(
    const cr::CreativeDocument& document,
    const CreativeAssetScatterStrokeState& stroke) noexcept {
  if (!stroke.recipeActive ||
      stroke.recipeId == cr::kInvalidCreativePatternRecipeId) {
    return nullptr;
  }
  const cr::CreativePatternRecipe* pattern = cr::findCreativePatternRecipe(
      document.patternRecipeStore(), stroke.recipeId);
  return pattern != nullptr &&
                 pattern->kind == cr::CreativePatternRecipeKind::AssetScatter
             ? pattern
             : nullptr;
}

[[nodiscard]] cr::CreativeObjectId scatterOutputParentId(
    const cr::CreativeDocument& document,
    const cr::CreativePatternRecipe* pattern,
    cr::CreativeObjectId fallback) noexcept {
  if (pattern == nullptr || pattern->generatedObjectIds.empty()) {
    return fallback;
  }
  const cr::CreativeObject* first =
      document.findObject(pattern->generatedObjectIds.front());
  const std::optional<cr::CreativeObjectId> parent =
      first != nullptr ? first->parentId : std::nullopt;
  for (cr::CreativeObjectId objectId : pattern->generatedObjectIds) {
    const cr::CreativeObject* object = document.findObject(objectId);
    if (object == nullptr || object->parentId != parent) {
      return fallback;
    }
  }
  return parent.value_or(cr::kInvalidObjectId);
}

[[nodiscard]] std::vector<cr::CreativeDocumentCreateRequest>
scatterCreateRequests(const CreativeEditorAssetScatterPlan& plan,
                      const cr::CreativeAssetScatterRecipe& recipe,
                      cr::CreativeObjectId parentId,
                      std::uint64_t firstOrdinal) {
  std::vector<cr::CreativeDocumentCreateRequest> requests;
  requests.reserve(plan.placeableCount);
  for (const CreativeEditorAssetScatterCandidate& candidate :
       plan.items()) {
    if (!candidate.placeable) {
      continue;
    }
    cr::CreativeDocumentCreateRequest request = buildBrushCreateRequest(
        candidate.placement, firstOrdinal + requests.size(), recipe.assetId,
        recipe.assetContentHash, recipe.assetMaterialVariant);
    if (parentId != cr::kInvalidObjectId) {
      request.parentId = parentId;
    }
    requests.push_back(std::move(request));
  }
  return requests;
}

[[nodiscard]] ScatterRecipeEraseResult applyScatterRecipeExclusion(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativePatternRecipe* pattern =
      cr::findCreativePatternRecipeByGeneratedObject(
          document.patternRecipeStore(), target.objectId);
  if (pattern == nullptr ||
      pattern->kind != cr::CreativePatternRecipeKind::AssetScatter) {
    return ScatterRecipeEraseResult::NotScatterRecipe;
  }
  const cr::CreativeObject* object = document.findObject(target.objectId);
  const cr::CreativeTransformedBounds bounds =
      object != nullptr ? cr::resolveCreativeObjectBounds(*object)
                        : cr::CreativeTransformedBounds{};
  if (!bounds.valid ||
      pattern->scatter.exclusions.size() >=
          cr::kCreativeAssetScatterExclusionCapacity) {
    rejectScatter(editor, target.objectKind);
    return ScatterRecipeEraseResult::Rejected;
  }

  const double exclusionRadius =
      std::max(0.05, pattern->scatter.spacingMeters * 0.45);
  const std::uint64_t key =
      0x2000000000000000ULL ^
      cr::creativeAssetScatterSpatialKey(bounds.center, exclusionRadius);
  if (stroke.capacityReached || key == 0U) {
    rejectScatter(editor, target.objectKind);
    return ScatterRecipeEraseResult::Rejected;
  }
  if (cr::creativeWorldGestureVisited(stroke.visited, key)) {
    return ScatterRecipeEraseResult::Applied;
  }

  cr::CreativeAssetScatterRecipe proposed = pattern->scatter;
  proposed.exclusions.push_back({bounds.center, exclusionRadius});
  const cr::CreativePatternRecipeId recipeId = pattern->id;
  if (!ensureScatterTransaction(appState, stroke,
                                cr::CreativeWorldGestureKind::Remove)) {
    rejectScatter(editor, target.objectKind);
    return ScatterRecipeEraseResult::Rejected;
  }
  if (!setScatterTransactionOperation(
          stroke, recipeId, pattern->sourceObjectIds, proposed,
          cr::CreativeAuthoringOperationKind::Reconcile,
          "AssetScatter.Exclude", 1U)) {
    rejectScatter(editor, target.objectKind);
    return ScatterRecipeEraseResult::Rejected;
  }
  const cr::CreativeAssetScatterRecipeMutationReceipt receipt =
      appState.facade.excludeAssetScatterOutput(recipeId, target.objectId,
                                                proposed);
  if (!receipt.accepted || !receipt.changed) {
    rejectScatter(editor, target.objectKind);
    return ScatterRecipeEraseResult::Rejected;
  }
  if (cr::rememberCreativeWorldGestureKey(stroke.visited, key) !=
      cr::CreativeWorldGestureVisitStatus::Inserted) {
    rejectScatter(editor, target.objectKind);
    return ScatterRecipeEraseResult::Rejected;
  }
  ++stroke.acceptedMutationCount;
  stroke.capacityReached = stroke.visited.count >= stroke.visited.keys.size();
  clearCreativeEditorPlacementFeedback(editor.interaction);
  return ScatterRecipeEraseResult::Applied;
}

[[nodiscard]] bool appendScatterPaintCenter(
    cr::CreativeAssetScatterRecipe& recipe,
    cr::CreativeVec3 center) {
  if (!cr::isFiniteCreativeVec3(center) ||
      recipe.paintCenters.size() >=
          cr::kCreativeAssetScatterPaintCenterCapacity) {
    return false;
  }
  const bool duplicate = std::any_of(
      recipe.paintCenters.begin(), recipe.paintCenters.end(),
      [center](cr::CreativeVec3 existing) {
        return cr::creativeVec3ExactlyEqual(existing, center);
      });
  if (!duplicate) {
    recipe.paintCenters.push_back(center);
  }
  return !duplicate;
}

[[nodiscard]] std::uint64_t scatterPaintCenterKey(
    const CreativeEditorState& editor) noexcept {
  return editor.interaction.target.grid.valid
             ? cr::creativeAssetScatterSpatialKey(
                   editor.interaction.target.grid.placementAnchor,
                   std::max(0.01, editor.placeCellSize * 0.5))
             : 0U;
}

void applyScatterPlacement(cr::CreativeAppState& appState,
                           CreativeEditorState& editor,
                           const cr::CreativeHotbarEntry& held) {
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  const std::uint64_t centerKey = scatterPaintCenterKey(editor);
  if (stroke.capacityReached || centerKey == 0U ||
      cr::creativeWorldGestureVisited(stroke.visited, centerKey)) {
    if (stroke.capacityReached || centerKey == 0U) {
      rejectScatter(editor, held.objectKind);
    }
    return;
  }

  cr::CreativeAssetScatterRecipe proposed =
      stroke.recipeActive ? stroke.recipe
                          : makeCreativeEditorAssetScatterRecipe(editor, held);
  if (stroke.recipeActive &&
      !appendScatterPaintCenter(
          proposed, editor.interaction.target.grid.placementAnchor)) {
    stroke.capacityReached =
        proposed.paintCenters.size() >=
        cr::kCreativeAssetScatterPaintCenterCapacity;
    rejectScatter(editor, held.objectKind);
    return;
  }

  std::vector<cr::CreativeObjectId> selectionFilter =
      stroke.recipeActive ? stroke.selectionFilterObjectIds
                          : std::vector<cr::CreativeObjectId>{};
  if (!stroke.recipeActive &&
      proposed.mask == cr::CreativeAssetScatterRecipeMask::Selection) {
    selectionFilter = selectedHierarchyObjectIds(appState);
  }
  const cr::CreativePatternRecipe* pattern =
      activeScatterPattern(appState.facade.document(), stroke);
  if (pattern != nullptr) {
    if (pattern->generatedObjectIds.size() >= proposed.maxGeneratedObjects) {
      stroke.capacityReached = true;
      rejectScatter(editor, held.objectKind);
      return;
    }
  }
  const CreativeEditorAssetScatterPlan& plan = stroke.preview;
  const cr::CreativeObjectId parentId = scatterOutputParentId(
      appState.facade.document(), pattern,
      activeCreativeEditorGroupFocusId(editor.groupFocus));
  std::vector<cr::CreativeDocumentCreateRequest> requests =
      scatterCreateRequests(plan, proposed, parentId,
                            editor.placedCount + 1U);
  if (requests.empty()) {
    for (const CreativeEditorAssetScatterCandidate& candidate : plan.items()) {
      if (candidate.status ==
          CreativeEditorAssetScatterCandidateStatus::Obstructed) {
        rejectScatter(editor, held.objectKind,
                      candidate.placement.clearance);
        return;
      }
    }
    rejectScatter(editor, held.objectKind);
    return;
  }
  if (!ensureScatterTransaction(appState, stroke,
                                cr::CreativeWorldGestureKind::Place)) {
    rejectScatter(editor, held.objectKind);
    return;
  }
  const cr::CreativePatternRecipeId prospectiveRecipeId =
      pattern != nullptr
          ? pattern->id
          : appState.facade.document().patternRecipeStore().nextRecipeId;
  if (!setScatterTransactionOperation(
          stroke, prospectiveRecipeId, selectionFilter, proposed,
          pattern == nullptr ? cr::CreativeAuthoringOperationKind::Apply
                             : cr::CreativeAuthoringOperationKind::Reconcile,
          "AssetScatter.Paint", requests.size())) {
    rejectScatter(editor, held.objectKind);
    return;
  }
  const cr::CreativeAssetScatterRecipeMutationReceipt receipt =
      pattern == nullptr
          ? appState.facade.createAssetScatterRecipe(requests, selectionFilter,
                                                     proposed)
          : appState.facade.extendAssetScatterRecipe(stroke.recipeId, requests,
                                                     proposed);
  if (!receipt.accepted || !receipt.changed ||
      receipt.generatedObjectCount != requests.size()) {
    rejectScatter(editor, held.objectKind);
    return;
  }
  static_cast<void>(
      cr::rememberCreativeWorldGestureKey(stroke.visited, centerKey));
  stroke.recipe = std::move(proposed);
  stroke.selectionFilterObjectIds = std::move(selectionFilter);
  stroke.recipeId = receipt.patternRecipeId;
  stroke.recipeActive = true;
  editor.placedCount += receipt.generatedObjectCount;
  ++stroke.acceptedMutationCount;
  stroke.capacityReached =
      stroke.recipe.paintCenters.size() >=
          cr::kCreativeAssetScatterPaintCenterCapacity ||
      stroke.visited.count >= stroke.visited.keys.size();
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Placed,
      editor.frameIndex, held.objectKind,
      receipt.generatedObjectIds.empty() ? cr::kInvalidObjectId
                                         : receipt.generatedObjectIds.back());
}

}  // namespace

bool creativeEditorUsesAssetScatter(
    const cr::CreativeHotbarEntry& held,
    const cr::CreativeToolSettings& settings) noexcept {
  return held.kind == cr::CreativeHeldItemKind::Material &&
         held.objectKind != cr::CreativeObjectKind::PrefabInstance &&
         !cr::creativeHotbarAssetId(held).empty() &&
         held.hasAssetBounds &&
         settings.assetPlacementMode == cr::CreativeAssetPlacementMode::Scatter;
}

CreativeEditorAssetScatterPlan buildCreativeEditorAssetScatterPlan(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeHotbarEntry& held,
    const CreativePlacementClearanceCache* clearanceCache) {
  if (!creativeEditorUsesAssetScatter(held, editor.toolSettings) ||
      !editor.interaction.target.grid.valid ||
      !std::isfinite(editor.placeCellSize) || editor.placeCellSize <= 0.0) {
    return {};
  }
  const cr::CreativeAssetScatterRecipe recipe =
      makeCreativeEditorAssetScatterRecipe(editor, held);
  return buildCreativeEditorAssetScatterRecipePlan(
      document, editor, recipe, {}, {}, clearanceCache, true);
}

cr::CreativeAssetScatterRecipe makeCreativeEditorAssetScatterRecipe(
    const CreativeEditorState& editor,
    const cr::CreativeHotbarEntry& held) {
  cr::CreativeAssetScatterRecipe recipe;
  if (!creativeEditorUsesAssetScatter(held, editor.toolSettings) ||
      !editor.interaction.target.grid.valid ||
      !std::isfinite(editor.placeCellSize) || editor.placeCellSize <= 0.0) {
    return recipe;
  }
  recipe.objectKind = held.objectKind;
  recipe.assetId = std::string{cr::creativeHotbarAssetId(held)};
  recipe.assetContentHash =
      held.hasAssetContentHash ? held.assetContentHash : 0U;
  recipe.assetMaterialVariant =
      std::string{cr::creativeHotbarAssetMaterialVariant(held)};
  recipe.assetSourceBounds = held.assetSourceBounds;
  recipe.paintCenters.push_back(
      editor.interaction.target.grid.placementAnchor);
  recipe.mask = recipeMask(editor.toolSettings.assetScatterMask);
  recipe.yaw = recipeYaw(editor.toolSettings.assetScatterYaw);
  recipe.baseYawRadians =
      cr::creativePlacementYawRadians(editor.toolSettings.placementYaw);
  recipe.radiusMeters =
      static_cast<double>(cr::creativeAssetScatterRadiusCells(
          editor.toolSettings.assetScatterRadius)) *
      editor.placeCellSize;
  recipe.spacingMeters =
      static_cast<double>(cr::creativeAssetScatterSpacingCells(
          editor.toolSettings.assetScatterSpacing)) *
      editor.placeCellSize;
  recipe.densityFraction = cr::creativeAssetScatterDensityFraction(
      editor.toolSettings.assetScatterDensity);
  recipe.scaleVariation = cr::creativeAssetScatterScaleVariation(
      editor.toolSettings.assetScatterScale);
  recipe.maximumSlopeRadians = cr::creativeAssetScatterMaximumSlopeRadians(
      editor.toolSettings.assetScatterSlope);
  recipe.projectToTerrainSurface = editor.interaction.target.terrainHit;
  recipe.avoidCollisions =
      editor.toolSettings.assetScatterCollision ==
      cr::CreativeAssetScatterCollision::Avoid;
  recipe.seed = scatterSeed(held, recipe.paintCenters.front(),
                            editor.placeCellSize);
  recipe.maxGeneratedObjects =
      cr::kCreativeAssetScatterGeneratedObjectCapacity;
  return recipe;
}

CreativeEditorAssetScatterPlan buildCreativeEditorAssetScatterRecipePlan(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeAssetScatterRecipe& recipe,
    std::span<const cr::CreativeObjectId> selectionFilterObjectIds,
    std::span<const cr::CreativeObjectId> ignoredObjectIds,
    const CreativePlacementClearanceCache* clearanceCache,
    bool includeKernelRejections) {
  CreativeEditorAssetScatterPlan plan;
  plan.requested = true;
  if (!std::isfinite(editor.placeCellSize) || editor.placeCellSize <= 0.0 ||
      !cr::isValidCreativeAssetScatterRecipe(recipe,
                                             selectionFilterObjectIds)) {
    plan.kernelStatus = cr::CreativeAssetScatterStatus::InvalidRequest;
    return plan;
  }

  for (std::size_t centerIndex = 0U;
       centerIndex < recipe.paintCenters.size(); ++centerIndex) {
    const cr::CreativeVec3 center = recipe.paintCenters[centerIndex];
    cr::CreativeAssetScatterRequest request;
    request.center = center;
    request.radiusMeters =
        recipe.mask == cr::CreativeAssetScatterRecipeMask::Box
            ? recipe.radiusMeters * std::numbers::sqrt2
            : recipe.radiusMeters;
    request.spacingMeters = recipe.spacingMeters;
    request.densityFraction = recipe.densityFraction;
    request.yaw = runtimeYaw(recipe.yaw);
    request.scaleVariation = recipe.scaleVariation;
    request.seed = recipe.seed ^
                   cr::creativeAssetScatterSpatialKey(center,
                                                      editor.placeCellSize);
    request.maxCandidateCount = cr::kCreativeAssetScatterCandidateCapacity;
    const cr::CreativeAssetScatterPlan kernel =
        cr::planCreativeAssetScatter(request);
    if (!kernel.accepted) {
      plan.kernelStatus = kernel.status;
      plan.truncated = plan.truncated || kernel.truncated;
      return plan;
    }
    plan.kernelStatus = cr::CreativeAssetScatterStatus::Ready;
    plan.truncated = plan.truncated || kernel.truncated;
    if (includeKernelRejections) {
      for (const cr::CreativeAssetScatterEvaluation& evaluation :
           kernel.evaluatedItems()) {
        if (evaluation.status ==
            cr::CreativeAssetScatterEvaluationStatus::Ready) {
          continue;
        }
        if (plan.rejectedCandidateCount >=
            plan.rejectedCandidates.size()) {
          plan.truncated = true;
          break;
        }
        CreativeEditorAssetScatterRejectedCandidate rejected =
            buildKernelRejectedPreview(document, editor, recipe, evaluation);
        if (rejected.valid) {
          plan.rejectedCandidates[plan.rejectedCandidateCount++] =
              std::move(rejected);
        }
      }
    }
    for (const cr::CreativeAssetScatterCandidate& kernelCandidate :
         kernel.items()) {
      if (plan.candidateCount >= plan.candidates.size()) {
        plan.truncated = true;
        break;
      }
      CreativeEditorAssetScatterCandidate candidate = buildEditorCandidate(
          document, editor, recipe, kernelCandidate, ignoredObjectIds,
          clearanceCache);
      if (!candidateInsideMask(document, recipe, selectionFilterObjectIds,
                               center, candidate.surfacePosition)) {
        candidate.status =
            CreativeEditorAssetScatterCandidateStatus::OutsideMask;
        candidate.placeable = false;
      } else if (candidateExcluded(recipe, candidate.surfacePosition)) {
        candidate.status = CreativeEditorAssetScatterCandidateStatus::Excluded;
        candidate.placeable = false;
      } else if (candidate.placeable) {
        const ScatterPlanConflict conflict = candidateConflictWithPlan(
            plan, candidate, recipe.spacingMeters, recipe.avoidCollisions);
        if (conflict != ScatterPlanConflict::None) {
          candidate.status =
              conflict == ScatterPlanConflict::Spacing
                  ? CreativeEditorAssetScatterCandidateStatus::Duplicate
                  : CreativeEditorAssetScatterCandidateStatus::Obstructed;
          candidate.placeable = false;
        }
      }
      if (candidate.placeable &&
          plan.placeableCount >= recipe.maxGeneratedObjects) {
        candidate.status =
            CreativeEditorAssetScatterCandidateStatus::CapacityRejected;
        candidate.placeable = false;
        plan.truncated = true;
      }
      if (candidate.placeable) {
        ++plan.placeableCount;
      }
      plan.candidates[plan.candidateCount++] = std::move(candidate);
    }
    if (plan.candidateCount >= plan.candidates.size()) {
      break;
    }
  }
  plan.accepted =
      plan.candidateCount > 0U || plan.rejectedCandidateCount > 0U;
  return plan;
}

cr::CreativeAssetScatterRecipeMutationReceipt
regenerateCreativeEditorAssetScatterRecipeWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    cr::CreativePatternRecipeId recipeId,
    const CreativePlacementClearanceCache* clearanceCache,
    std::string_view source) {
  cr::CreativeAssetScatterRecipeMutationReceipt receipt;
  receipt.requested = true;
  receipt.patternRecipeId = recipeId;
  const cr::CreativePatternRecipe* pattern = cr::findCreativePatternRecipe(
      appState.facade.document().patternRecipeStore(), recipeId);
  if (pattern == nullptr) {
    receipt.status =
        cr::CreativeAssetScatterRecipeMutationStatus::RecipeNotFound;
    receipt.message = "creative_asset_scatter_recipe_not_found";
    return receipt;
  }
  if (pattern->kind != cr::CreativePatternRecipeKind::AssetScatter) {
    receipt.status =
        cr::CreativeAssetScatterRecipeMutationStatus::RecipeKindMismatch;
    receipt.message = "creative_asset_scatter_recipe_kind_mismatch";
    return receipt;
  }

  cr::CreativeAssetScatterRecipe proposed = pattern->scatter;
  proposed.seed += 0x9e3779b97f4a7c15ULL;
  const std::vector<cr::CreativeObjectId> sourceObjectIds =
      pattern->sourceObjectIds;
  const std::vector<cr::CreativeObjectId> oldOutputIds =
      pattern->generatedObjectIds;
  const cr::CreativeObjectId parentId = scatterOutputParentId(
      appState.facade.document(), pattern,
      activeCreativeEditorGroupFocusId(editor.groupFocus));
  const CreativeEditorAssetScatterPlan plan =
      buildCreativeEditorAssetScatterRecipePlan(
          appState.facade.document(), editor, proposed, sourceObjectIds,
          oldOutputIds, clearanceCache);
  std::vector<cr::CreativeDocumentCreateRequest> requests =
      scatterCreateRequests(plan, proposed, parentId,
                            editor.placedCount + 1U);
  if (requests.empty()) {
    receipt.status = cr::CreativeAssetScatterRecipeMutationStatus::Empty;
    receipt.message = "creative_asset_scatter_regenerate_empty";
    return receipt;
  }

  cr::CreativePatternRecipe prospective = *pattern;
  prospective.generatedObjectIds.clear();
  prospective.scatter = proposed;
  std::optional<cr::CreativeAuthoringOperationRecord> operation =
      cr::makeCreativeAuthoringOperationRecord(
          cr::CreativeAuthoringFamily::AssetScatter,
          cr::CreativeAuthoringOperationKind::Reconcile,
          "AssetScatter.Regenerate",
          cr::fingerprintCreativePatternRecipeSource(prospective),
          oldOutputIds.size() + requests.size());
  if (!operation.has_value()) {
    receipt.status =
        cr::CreativeAssetScatterRecipeMutationStatus::InvalidRequest;
    receipt.message =
        "creative_asset_scatter_operation_record_invalid";
    return receipt;
  }
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source, std::move(*operation));
  receipt = appState.facade.updateAssetScatterRecipe(recipeId, requests,
                                                     proposed);
  if (receipt.accepted && receipt.changed &&
      !receipt.generatedObjectIds.empty()) {
    const cr::CreativeObjectId primary = receipt.generatedObjectIds.back();
    static_cast<void>(
        appState.facade.selectTargets(receipt.generatedObjectIds, primary));
  }
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.message));
  return receipt;
}

namespace {

void refreshScatterStrokePreview(
    const cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeHotbarEntry& held,
    const CreativePlacementClearanceCache* clearanceCache) {
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  cr::CreativeAssetScatterRecipe proposed =
      stroke.recipeActive ? stroke.recipe
                          : makeCreativeEditorAssetScatterRecipe(editor, held);
  bool plannerRequested = !proposed.paintCenters.empty();
  if (stroke.recipeActive) {
    const std::uint64_t centerKey = scatterPaintCenterKey(editor);
    const cr::CreativePatternRecipe* pattern =
        activeScatterPattern(appState.facade.document(), stroke);
    plannerRequested = pattern != nullptr && centerKey != 0U &&
                       !cr::creativeWorldGestureVisited(stroke.visited,
                                                        centerKey) &&
                       pattern->generatedObjectIds.size() <
                           proposed.maxGeneratedObjects;
    if (plannerRequested) {
      proposed.paintCenters = {
          editor.interaction.target.grid.placementAnchor};
      proposed.maxGeneratedObjects -= pattern->generatedObjectIds.size();
    } else {
      proposed.paintCenters.clear();
    }
  }
  std::vector<cr::CreativeObjectId> selectionFilter =
      stroke.recipeActive ? stroke.selectionFilterObjectIds
                          : std::vector<cr::CreativeObjectId>{};
  if (!stroke.recipeActive &&
      proposed.mask == cr::CreativeAssetScatterRecipeMask::Selection) {
    selectionFilter = selectedHierarchyObjectIds(appState);
  }
  const cr::CreativeDocument& document = appState.facade.document();
  if (stroke.previewCacheValid &&
      stroke.previewDocumentId == document.id() &&
      stroke.previewDocumentRevision == document.revision() &&
      stroke.previewPlannerRequested == plannerRequested &&
      stroke.previewRecipeKey == proposed &&
      stroke.previewSelectionFilterKey == selectionFilter) {
    return;
  }

  stroke.preview = {};
  if (plannerRequested) {
    stroke.preview = buildCreativeEditorAssetScatterRecipePlan(
        document, editor, proposed, selectionFilter, {}, clearanceCache, true);
    ++stroke.previewBuildCount;
  }
  stroke.previewRecipeKey = std::move(proposed);
  stroke.previewSelectionFilterKey = std::move(selectionFilter);
  stroke.previewDocumentId = document.id();
  stroke.previewDocumentRevision = document.revision();
  stroke.previewPlannerRequested = plannerRequested;
  stroke.previewCacheValid = true;
}

}  // namespace

void processCreativeAssetScatterFrame(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds,
    const CreativePlacementClearanceCache* clearanceCache) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (!creativeEditorUsesAssetScatter(held, editor.toolSettings)) {
    finalizeCreativeAssetScatterStroke(
        appState, editor, "creative_asset_scatter_non_scatter_tool");
    return;
  }
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  refreshScatterStrokePreview(appState, editor, held, clearanceCache);
  const cr::CreativeWorldGestureRepeatRequest repeatRequest =
      cr::makeCreativeWorldStrokeRepeatRequest(actions,
                                               monotonicTimeNanoseconds);
  const cr::CreativeWorldGestureRepeatResult repeat =
      cr::stepCreativeWorldGestureRepeat(stroke.repeat, repeatRequest);
  stroke.repeat = repeat.next;
  if (repeat.finalized) {
    finalizeCreativeAssetScatterStroke(
        appState, editor, "creative_asset_scatter_released");
    refreshScatterStrokePreview(appState, editor, held, clearanceCache);
    return;
  }
  if (!repeat.mutationDue) {
    return;
  }
  if (repeat.dueKind == cr::CreativeWorldGestureKind::Remove) {
    applyScatterRemoval(appState, editor, held);
  } else if (repeat.dueKind == cr::CreativeWorldGestureKind::Place) {
    applyScatterPlacement(appState, editor, held);
  }
  refreshScatterStrokePreview(appState, editor, held, clearanceCache);
}

void finalizeCreativeAssetScatterStroke(cr::CreativeAppState& appState,
                                        CreativeEditorState& editor,
                                        std::string_view reasonCode) {
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  if (!stroke.repeat.active && !stroke.transaction.active &&
      stroke.preview.candidateCount == 0U && stroke.visited.count == 0U &&
      stroke.acceptedMutationCount == 0U && !stroke.capacityReached) {
    return;
  }
  StandaloneEditTransaction transaction = std::move(stroke.transaction);
  const bool changed = stroke.acceptedMutationCount > 0U;
  stroke = {};
  if (transaction.active) {
    static_cast<void>(completeEditTransaction(
        appState.history, std::move(transaction), appState.facade, changed,
        reasonCode));
  }
}

std::size_t appendCreativeEditorAssetScatterWireframes(
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const CreativeEditorAssetScatterPlan& preview =
      editor.interaction.assetScatter.preview;
  if (!preview.accepted) {
    return 0U;
  }
  const std::size_t before = wireLines.size();
  for (const CreativeEditorAssetScatterCandidate& candidate :
       preview.items()) {
    if (!candidate.placement.valid) {
      continue;
    }
    const cr::CreativeTransformedBounds bounds =
        cr::resolveCreativeTransformedBounds(candidate.placement.previewBounds,
                                             candidate.placement.transform);
    if (!bounds.valid) {
      continue;
    }
    const cr::CreativeCoreVec3Conversion minimum =
        cr::creativeVec3ToCoreChecked(bounds.worldBounds.min);
    const cr::CreativeCoreVec3Conversion maximum =
        cr::creativeVec3ToCoreChecked(bounds.worldBounds.max);
    if (!minimum.converted || !maximum.converted) {
      continue;
    }
    appendStandaloneWireframeBoxEdges(
        wireLines, minimum.value, maximum.value,
        candidate.placeable
            ? iggy3d::RenderLineColor{0.22F, 1.0F, 0.34F, 0.92F}
            : iggy3d::RenderLineColor{1.0F, 0.20F, 0.16F, 0.92F},
        std::max(0.035F, wireThickness * 0.8F));
  }
  for (const CreativeEditorAssetScatterRejectedCandidate& candidate :
       preview.rejectedItems()) {
    if (!candidate.valid) {
      continue;
    }
    const cr::CreativeCoreVec3Conversion minimum =
        cr::creativeVec3ToCoreChecked(candidate.worldBounds.min);
    const cr::CreativeCoreVec3Conversion maximum =
        cr::creativeVec3ToCoreChecked(candidate.worldBounds.max);
    if (!minimum.converted || !maximum.converted) {
      continue;
    }
    appendStandaloneWireframeBoxEdges(
        wireLines, minimum.value, maximum.value,
        iggy3d::RenderLineColor{1.0F, 0.20F, 0.16F, 0.72F},
        std::max(0.025F, wireThickness * 0.65F));
  }
  return wireLines.size() - before;
}

}  // namespace iggy3d_creative_app
