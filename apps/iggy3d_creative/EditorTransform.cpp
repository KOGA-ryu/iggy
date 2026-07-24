#include "EditorTransform.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorPlacementClearance.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutInternal.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] cr::CreativeVec3 subtract(cr::CreativeVec3 lhs,
                                        cr::CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] cr::CreativeVec3 add(cr::CreativeVec3 lhs,
                                   cr::CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] cr::CreativeVec3 scale(cr::CreativeVec3 value,
                                     double factor) noexcept {
  return {value.x * factor, value.y * factor, value.z * factor};
}

[[nodiscard]] double dot(cr::CreativeVec3 lhs,
                         cr::CreativeVec3 rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] bool normalized(cr::CreativeVec3 value,
                              cr::CreativeVec3& output) noexcept {
  const double lengthSquared = dot(value, value);
  if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-16) {
    return false;
  }
  output = scale(value, 1.0 / std::sqrt(lengthSquared));
  return cr::isFiniteCreativeVec3(output);
}

[[nodiscard]] std::vector<cr::CreativeObjectId> clipboardObjectIds(
    const cr::CreativeClipboard& clipboard) {
  std::vector<cr::CreativeObjectId> ids;
  ids.reserve(clipboard.objects.size());
  for (const cr::CreativeObject& object : clipboard.objects) {
    ids.push_back(object.id);
  }
  return ids;
}

[[nodiscard]] bool moveSourceAvailable(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard) noexcept {
  const cr::CreativeDocument& document = appState.facade.document();
  if (clipboard.sourceDocumentId != document.id() ||
      clipboard.sourceRevision != document.revision() ||
      cr::creativeClipboardEmpty(clipboard)) {
    return false;
  }
  return std::all_of(
      clipboard.objects.begin(), clipboard.objects.end(),
      [&document](const cr::CreativeObject& object) {
        return document.containsObject(object.id);
      });
}

[[nodiscard]] CreativeEditorTransformPreflight inspectTransformSource(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    cr::CreativeSelectionPlacementMode mode,
    const CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorTransformPreflight result;
  result.requested = true;
  if (cr::creativeClipboardEmpty(clipboard) || !clipboard.hasPlacementAnchor ||
      !cr::isFiniteCreativeVec3(clipboard.placementAnchor)) {
    result.reasonCode = "editor_transform_source_invalid";
    return result;
  }

  result.capabilities = cr::resolveCreativeSelectionPlacementCapabilities(
      clipboard.objects);
  if (!result.capabilities.resolved) {
    result.reasonCode = "editor_transform_capabilities_unresolved";
    return result;
  }
  const bool anyOperation =
      result.capabilities.translate || result.capabilities.mirror ||
      result.capabilities.rotation != cr::CreativeObjectRotationSupport::None ||
      result.capabilities.scale != cr::CreativeObjectScaleSupport::None;
  if (!anyOperation) {
    result.reasonCode = "editor_transform_source_not_transformable";
    return result;
  }

  const cr::CreativeDocument& document = appState.facade.document();
  if (mode == cr::CreativeSelectionPlacementMode::Move &&
      clipboard.sourceDocumentId == document.id() &&
      !clipboard.patternRecipes.empty()) {
    const cr::CreativeSemanticObjectActionPolicy transformPolicy =
        cr::resolveCreativeSemanticObjectAction(
            cr::CreativeSemanticSelectionOwner::PatternRecipe,
            cr::CreativeSemanticObjectAction::TransformSelection);
    if (!transformPolicy.allowed ||
        transformPolicy.route !=
            cr::CreativeSemanticObjectActionRoute::PatternRecipe) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.reasonCode = std::string{transformPolicy.reasonCode};
      return result;
    }
    if (clipboard.sourceRevision != document.revision() ||
        clipboard.patternRecipes.size() != 1U) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.reasonCode =
          "editor_transform_pattern_recipe_dependency_conflict";
      return result;
    }
    const cr::CreativePatternRecipeId recipeId =
        clipboard.patternRecipes.front().id;
    const cr::CreativePatternRecipeTranslationPlan patternPlan =
        cr::planCreativePatternRecipeTranslation(document, recipeId, {});
    if (!patternPlan.accepted) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.failedObjectId = patternPlan.failedObjectId;
      result.reasonCode = std::string{patternPlan.reasonCode};
      return result;
    }
    std::unordered_set<cr::CreativeObjectId> clipboardIds;
    clipboardIds.reserve(clipboard.objects.size());
    for (const cr::CreativeObject& object : clipboard.objects) {
      clipboardIds.insert(object.id);
    }
    if (clipboardIds.size() != patternPlan.memberObjectIds.size() ||
        !std::all_of(patternPlan.memberObjectIds.begin(),
                     patternPlan.memberObjectIds.end(),
                     [&clipboardIds](cr::CreativeObjectId objectId) {
                       return clipboardIds.contains(objectId);
                     })) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.reasonCode =
          "editor_transform_pattern_recipe_scope_incomplete";
      return result;
    }
    if (worldLayout != nullptr) {
      const cr::CreativePatternRecipe* liveRecipe =
          cr::findCreativePatternRecipe(document.patternRecipeStore(),
                                        recipeId);
      if (liveRecipe == nullptr) {
        result.reasonCode = "editor_transform_pattern_recipe_missing";
        return result;
      }
      for (cr::CreativeObjectId sourceObjectId :
           liveRecipe->sourceObjectIds) {
        const cr::CreativeSemanticSelectionResolution semantic =
            cr::resolveCreativeSemanticSelection(
                document, sourceObjectId, &worldLayout->source);
        if (semantic.accepted &&
            semantic.worldLayoutSource.owned) {
          result.blockedOwner =
              cr::CreativeSemanticSelectionOwner::WorldLayoutSource;
          result.failedObjectId = sourceObjectId;
          result.reasonCode =
              "editor_transform_pattern_recipe_world_layout_source_owned";
          return result;
        }
      }
    }
    result.ownershipRoute =
        CreativeEditorTransformOwnershipRoute::PatternRecipe;
    result.patternRecipeId = recipeId;
    result.sourceDocumentRevision = document.revision();
    result.capabilities = {true, patternPlan.memberObjectIds.size(), true,
                           cr::CreativeObjectRotationSupport::None,
                           cr::CreativeObjectScaleSupport::None, false};
    result.accepted = true;
    result.reasonCode = "editor_transform_pattern_recipe_ready";
    return result;
  }

  std::unordered_set<cr::CreativeObjectId> sourceIds;
  sourceIds.reserve(clipboard.objects.size());
  for (const cr::CreativeObject& object : clipboard.objects) {
    sourceIds.insert(object.id);
    if (mode == cr::CreativeSelectionPlacementMode::Move) {
      const cr::CreativeObjectHierarchyState hierarchyState =
          cr::resolveCreativeObjectHierarchyState(document, object.id);
      if (!hierarchyState.resolved || hierarchyState.effectivelyLocked) {
        result.failedObjectId =
            hierarchyState.lockedByObjectId != cr::kInvalidObjectId
                ? hierarchyState.lockedByObjectId
                : object.id;
        result.reasonCode = "editor_transform_object_locked";
        return result;
      }
    }
  }
  if (mode == cr::CreativeSelectionPlacementMode::Move) {
    for (const cr::CreativeObject& object : clipboard.objects) {
      if (object.parentId.has_value() &&
          !sourceIds.contains(*object.parentId)) {
        result.failedObjectId = object.id;
        result.reasonCode = "editor_transform_external_parent";
        return result;
      }
    }
  }

  if (clipboard.sourceDocumentId == document.id()) {
    const std::vector<cr::CreativeObjectId> objectIds =
        clipboardObjectIds(clipboard);
    const cr::CreativeSemanticSelectionSetResolution semanticSet =
        cr::resolveCreativeSemanticSelectionSet(
            document, objectIds,
            objectIds.empty() ? cr::kInvalidObjectId : objectIds.front(),
            worldLayout != nullptr ? &worldLayout->source : nullptr);
    if (!semanticSet.accepted) {
      result.reasonCode = semanticSet.reasonCode;
      return result;
    }
    const bool onlyWorldLayoutOwned =
        semanticSet.worldLayoutOwnerCount > 0U &&
        semanticSet.authoredOwnerCount == 0U &&
        semanticSet.patternOwnerCount == 0U;
    if (onlyWorldLayoutOwned) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::WorldLayoutSource;
      result.failedObjectId = semanticSet.primaryObjectId;
      result.worldLayoutSource = semanticSet.commonWorldLayoutSource;
      if (worldLayout == nullptr ||
          worldLayout->generatedRevision != worldLayout->revision) {
        result.reasonCode =
            "editor_transform_world_layout_source_unsynchronized";
        return result;
      }
      const cr::CreativeWorldLayoutSourceRef buildingSource =
          cr::resolveCompleteCreativeWorldLayoutBuildingSelectionSource(
              document, objectIds, worldLayout->source);
      if (buildingSource.table ==
          cr::CreativeWorldLayoutTable::Building) {
        result.worldLayoutSource = buildingSource;
      }
      const cr::CreativeSemanticObjectActionPolicy transformPolicy =
          cr::resolveCreativeSemanticObjectAction(
              cr::CreativeSemanticSelectionOwner::WorldLayoutSource,
              cr::CreativeSemanticObjectAction::TransformSelection,
              result.worldLayoutSource.table);
      if (!transformPolicy.allowed ||
          transformPolicy.route !=
              cr::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
        result.reasonCode = std::string{transformPolicy.reasonCode};
        return result;
      }
      if (result.worldLayoutSource.index >=
          worldLayout->source.buildings.size()) {
        result.reasonCode =
            "editor_transform_world_layout_building_scope_required";
        return result;
      }
      result.ownershipRoute =
          CreativeEditorTransformOwnershipRoute::WorldLayoutBuilding;
      result.capabilities = {true, clipboard.objects.size(), true,
                             cr::CreativeObjectRotationSupport::QuarterTurns,
                             cr::CreativeObjectScaleSupport::None, true};
      result.worldLayoutRevision = worldLayout->revision;
      result.worldLayoutSourceEpoch = worldLayout->sourceEpoch;
      result.accepted = true;
      result.reasonCode = "editor_transform_world_layout_building_ready";
      return result;
    }

    const cr::CreativeSemanticObjectActionPolicy transformPolicy =
        cr::resolveCreativeSemanticObjectAction(
            semanticSet,
            cr::CreativeSemanticObjectAction::TransformSelection);
    if (!transformPolicy.allowed) {
      result.blockedOwner = semanticSet.primaryOwner;
      result.failedObjectId = semanticSet.primaryObjectId;
      result.reasonCode = std::string{transformPolicy.reasonCode};
      return result;
    }
    if (transformPolicy.route ==
        cr::CreativeSemanticObjectActionRoute::PatternRecipe) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.failedObjectId = semanticSet.primaryObjectId;
      result.reasonCode = "editor_transform_pattern_recipe_owned";
      return result;
    }
    if (transformPolicy.route !=
        cr::CreativeSemanticObjectActionRoute::Document) {
      result.blockedOwner = semanticSet.primaryOwner;
      result.failedObjectId = semanticSet.primaryObjectId;
      result.reasonCode = std::string{transformPolicy.reasonCode};
      return result;
    }
  }

  result.accepted = true;
  result.reasonCode = "editor_transform_preflight_ready";
  return result;
}

[[nodiscard]] bool worldLayoutBuildingRoute(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.ownershipRoute ==
         CreativeEditorTransformOwnershipRoute::WorldLayoutBuilding;
}

[[nodiscard]] bool patternRecipeRoute(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.ownershipRoute ==
         CreativeEditorTransformOwnershipRoute::PatternRecipe;
}

[[nodiscard]] bool terrainOperationRoute(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.ownershipRoute ==
         CreativeEditorTransformOwnershipRoute::TerrainOperation;
}

[[nodiscard]] bool planarSourceRoute(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return worldLayoutBuildingRoute(state) || terrainOperationRoute(state);
}

[[nodiscard]] bool quarterTurnDegrees(double degrees) noexcept;

[[nodiscard]] bool unitScale(cr::CreativeVec3 scale) noexcept {
  return std::abs(scale.x - 1.0) <= 1.0e-12 &&
         std::abs(scale.y - 1.0) <= 1.0e-12 &&
         std::abs(scale.z - 1.0) <= 1.0e-12;
}

[[nodiscard]] bool exactGridDelta(double meters,
                                  double cellSizeMeters,
                                  std::int64_t& cells) noexcept {
  if (!std::isfinite(meters) || !std::isfinite(cellSizeMeters) ||
      cellSizeMeters <= 0.0) {
    return false;
  }
  const double exact = meters / cellSizeMeters;
  const double rounded = std::round(exact);
  constexpr double kGridDeltaTolerance = 1.0e-7;
  if (!std::isfinite(exact) ||
      std::abs(exact - rounded) > kGridDeltaTolerance ||
      rounded < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      rounded > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  cells = static_cast<std::int64_t>(rounded);
  return true;
}

struct WorldLayoutTransformCandidate {
  bool accepted = false;
  bool changed = false;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t nextStableOrdinal = 1U;
  cr::CreativeWorldLayout layout{};
  std::string reasonCode =
      "editor_transform_world_layout_candidate_not_requested";
};

[[nodiscard]] bool applyWorldLayoutBuildingOperation(
    WorldLayoutTransformCandidate& candidate,
    cr::CreativeWorldLayoutBuildingTransformOperation operation) {
  cr::CreativeWorldLayoutBuildingTransformResult transformed =
      cr::transformCreativeWorldLayoutBuilding(
          candidate.layout, {candidate.buildingIndex, operation});
  if (!transformed.accepted) {
    candidate.reasonCode = transformed.reasonCode;
    return false;
  }
  candidate.layout = std::move(transformed.transformed);
  candidate.changed = true;
  return true;
}

[[nodiscard]] WorldLayoutTransformCandidate buildWorldLayoutTransformCandidate(
    const cr::CreativeAppState& appState,
    const CreativeEditorSelectionTransformState& state) {
  WorldLayoutTransformCandidate candidate;
  candidate.layout = state.sourceWorldLayout;
  candidate.nextStableOrdinal = state.sourceWorldLayoutNextStableOrdinal;
  candidate.buildingIndex = state.preflight.worldLayoutSource.index;
  if (!worldLayoutBuildingRoute(state) ||
      candidate.buildingIndex >= candidate.layout.buildings.size() ||
      state.request.coordinateSpace !=
          cr::CreativeSelectionPlacementCoordinateSpace::World ||
      state.request.pivotMode !=
          cr::CreativeSelectionPlacementPivotMode::SharedAnchor ||
      !unitScale(state.request.scaleFactor)) {
    candidate.reasonCode =
        "editor_transform_world_layout_request_not_representable";
    return candidate;
  }
  if (state.request.hasAxisAngleRotation &&
      (state.request.rotationAxis != cr::CreativeAxis3::Y ||
       !quarterTurnDegrees(state.rotationDegrees))) {
    candidate.reasonCode =
        "editor_transform_world_layout_rotation_not_representable";
    return candidate;
  }

  const cr::CreativeGridSettings grid =
      appState.facade.document().gridSettings();
  const cr::CreativeVec3 displacement =
      subtract(state.request.targetAnchor, state.request.sourceAnchor);
  constexpr double kVerticalToleranceMeters = 1.0e-7;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
  if (std::abs(displacement.y) > kVerticalToleranceMeters) {
    candidate.reasonCode =
        "editor_transform_world_layout_vertical_move_unsupported";
    return candidate;
  }
  if (!exactGridDelta(displacement.x, grid.cellSizeMeters, deltaXCells) ||
      !exactGridDelta(displacement.z, grid.cellSizeMeters, deltaZCells)) {
    candidate.reasonCode =
        "editor_transform_world_layout_move_requires_grid_cells";
    return candidate;
  }

  if (state.mode == cr::CreativeSelectionPlacementMode::Copy) {
    if (deltaXCells == 0 && deltaZCells == 0) {
      candidate.reasonCode =
          "editor_transform_world_layout_duplicate_offset_required";
      return candidate;
    }
    cr::CreativeWorldLayoutBuildingEditResult duplicated =
        cr::duplicateCreativeWorldLayoutBuilding(
            candidate.layout,
            {candidate.buildingIndex, deltaXCells, deltaZCells,
             candidate.nextStableOrdinal});
    if (!duplicated.accepted || !duplicated.changed) {
      candidate.reasonCode = duplicated.reasonCode;
      return candidate;
    }
    candidate.layout = std::move(duplicated.edited);
    candidate.buildingIndex = duplicated.resultBuildingIndex;
    candidate.nextStableOrdinal = duplicated.nextStableOrdinal;
    candidate.changed = true;
  }

  if (state.request.mirrorX &&
      !applyWorldLayoutBuildingOperation(
          candidate,
          cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX)) {
    return candidate;
  }
  if (state.request.mirrorZ &&
      !applyWorldLayoutBuildingOperation(
          candidate,
          cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ)) {
    return candidate;
  }
  if (state.request.hasAxisAngleRotation) {
    int turns = static_cast<int>(std::llround(state.rotationDegrees / 90.0));
    turns %= 4;
    if (turns < 0) {
      turns += 4;
    }
    for (int index = 0; index < turns; ++index) {
      if (!applyWorldLayoutBuildingOperation(
              candidate,
              cr::CreativeWorldLayoutBuildingTransformOperation::
                  RotateRight90)) {
        return candidate;
      }
    }
  }

  if (state.mode == cr::CreativeSelectionPlacementMode::Move &&
      (deltaXCells != 0 || deltaZCells != 0)) {
    cr::CreativeWorldLayoutBuildingEditResult moved =
        cr::moveCreativeWorldLayoutBuilding(
            candidate.layout,
            {candidate.buildingIndex, deltaXCells, deltaZCells});
    if (!moved.accepted || !moved.changed) {
      candidate.reasonCode = moved.reasonCode;
      return candidate;
    }
    candidate.layout = std::move(moved.edited);
    candidate.changed = true;
  }

  candidate.accepted = true;
  candidate.reasonCode = candidate.changed
                             ? "editor_transform_world_layout_candidate_ready"
                             : "editor_transform_world_layout_no_change";
  return candidate;
}

void includePlanObjectBounds(cr::CreativeSelectionPlacementPlan& plan,
                             const cr::CreativeObject& object) noexcept {
  const cr::CreativeTransformedBounds bounds =
      cr::resolveCreativeObjectBounds(object);
  if (!bounds.valid) {
    return;
  }
  if (!plan.hasAggregateBounds) {
    plan.aggregateBounds = bounds.worldBounds;
    plan.hasAggregateBounds = true;
    return;
  }
  plan.aggregateBounds.min.x =
      std::min(plan.aggregateBounds.min.x, bounds.worldBounds.min.x);
  plan.aggregateBounds.min.y =
      std::min(plan.aggregateBounds.min.y, bounds.worldBounds.min.y);
  plan.aggregateBounds.min.z =
      std::min(plan.aggregateBounds.min.z, bounds.worldBounds.min.z);
  plan.aggregateBounds.max.x =
      std::max(plan.aggregateBounds.max.x, bounds.worldBounds.max.x);
  plan.aggregateBounds.max.y =
      std::max(plan.aggregateBounds.max.y, bounds.worldBounds.max.y);
  plan.aggregateBounds.max.z =
      std::max(plan.aggregateBounds.max.z, bounds.worldBounds.max.z);
}

void refreshWorldLayoutTransformPlan(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state) {
  state.plan = {};
  state.plan.requested = true;
  state.plan.request = state.request;
  state.candidateWorldLayoutReady = false;
  state.candidateWorldLayoutChanged = false;
  state.candidateWorldLayout = {};
  state.candidateWorldLayoutPlan = {};
  const WorldLayoutTransformCandidate candidate =
      buildWorldLayoutTransformCandidate(appState, state);
  if (!candidate.accepted) {
    state.plan.status = cr::CreativeSelectionPlacementStatus::InvalidRequest;
    state.plan.reasonCode = candidate.reasonCode;
    return;
  }

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       candidate.layout);
  if (!compiled.receipt.accepted) {
    state.plan.status = cr::CreativeSelectionPlacementStatus::Rejected;
    state.plan.reasonCode = compiled.receipt.reasonCode;
    return;
  }
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(appState.facade.document(),
                                         compiled.plan);
  if (!preview.accepted) {
    state.plan.status = cr::CreativeSelectionPlacementStatus::Rejected;
    state.plan.reasonCode = preview.reasonCode;
    return;
  }

  for (const cr::CreativeObject& object : preview.document.objects()) {
    if (!cr::creativeWorldLayoutObjectBelongsToSource(
            candidate.layout, object, cr::CreativeWorldLayoutTable::Building,
            candidate.buildingIndex)) {
      continue;
    }
    state.plan.objects.push_back(object);
    includePlanObjectBounds(state.plan, object);
  }
  if (state.plan.objects.empty() || !state.plan.hasAggregateBounds) {
    state.plan.status = cr::CreativeSelectionPlacementStatus::Rejected;
    state.plan.reasonCode =
        "editor_transform_world_layout_preview_empty";
    return;
  }
  state.plan.accepted = true;
  state.plan.status = cr::CreativeSelectionPlacementStatus::Planned;
  state.plan.objectCount = state.plan.objects.size();
  state.plan.reasonCode = candidate.reasonCode;
  state.candidateWorldLayout = candidate.layout;
  state.candidateWorldLayoutPlan = compiled.plan;
  state.candidateWorldLayoutBuildingIndex = candidate.buildingIndex;
  state.candidateWorldLayoutNextStableOrdinal = candidate.nextStableOrdinal;
  state.candidateWorldLayoutChanged = candidate.changed;
  state.candidateWorldLayoutReady = true;
}

void refreshTransformPlan(const cr::CreativeAppState& appState,
                          CreativeEditorSelectionTransformState& state) {
  state.clearance = {};
  state.clearanceCandidateObjectId = cr::kInvalidObjectId;
  state.clearanceCandidateObjectCount = 0U;
  if (!state.active || !state.targetPositionable) {
    state.plan = {};
    return;
  }
  if (worldLayoutBuildingRoute(state)) {
    refreshWorldLayoutTransformPlan(appState, state);
    state.moveAvailable = moveSourceAvailable(appState, state.sourceClipboard);
    return;
  }
  if (patternRecipeRoute(state)) {
    const cr::CreativeVec3 displacement =
        subtract(state.request.targetAnchor, state.request.sourceAnchor);
    state.candidatePatternTranslation =
        cr::planCreativePatternRecipeTranslation(
            appState.facade.document(), state.preflight.patternRecipeId,
            displacement);
    cr::CreativeSelectionPlacementRequest previewRequest = state.request;
    previewRequest.mode = cr::CreativeSelectionPlacementMode::Copy;
    state.plan = cr::planCreativeSelectionPlacement(
        state.sourceClipboard.objects, previewRequest);
    const bool sourceCurrent =
        appState.facade.document().revision() ==
        state.preflight.sourceDocumentRevision;
    state.moveAvailable =
        sourceCurrent && state.candidatePatternTranslation.accepted;
    if (!state.candidatePatternTranslation.accepted || !sourceCurrent) {
      state.plan.accepted = false;
      state.plan.status = cr::CreativeSelectionPlacementStatus::InvalidSource;
      state.plan.failedObjectId =
          state.candidatePatternTranslation.failedObjectId;
      state.plan.reasonCode =
          sourceCurrent
              ? std::string{state.candidatePatternTranslation.reasonCode}
              : "editor_transform_pattern_recipe_source_stale";
    } else {
      state.plan.reasonCode =
          std::string{state.candidatePatternTranslation.reasonCode};
    }
    return;
  }
  if (terrainOperationRoute(state)) {
    state.candidateTerrainTranslation = {};
    state.moveAvailable =
        appState.facade.document().revision() ==
            state.preflight.sourceDocumentRevision &&
        cr::findCreativeTerrainOperation(
            appState.facade.document().terrainOperationStack(),
            state.preflight.terrainOperationId) != nullptr;
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    std::int64_t deltaX = 0;
    std::int64_t deltaZ = 0;
    const cr::CreativeVec3 displacement =
        subtract(state.request.targetAnchor, state.request.sourceAnchor);
    const bool exact = state.moveAvailable &&
                       std::abs(displacement.y) <= 1.0e-9 &&
                       exactGridDelta(displacement.x, grid.cellSizeMeters,
                                      deltaX) &&
                       exactGridDelta(displacement.z, grid.cellSizeMeters,
                                      deltaZ) &&
                       deltaX >= std::numeric_limits<std::int32_t>::min() &&
                       deltaX <= std::numeric_limits<std::int32_t>::max() &&
                       deltaZ >= std::numeric_limits<std::int32_t>::min() &&
                       deltaZ <= std::numeric_limits<std::int32_t>::max();
    if (exact) {
      state.candidateTerrainTranslation =
          cr::planCreativeTerrainOperationTranslation(
              appState.facade.document(),
              state.preflight.terrainOperationId,
              {static_cast<std::int32_t>(deltaX),
               static_cast<std::int32_t>(deltaZ)});
    }
    cr::CreativeSelectionPlacementRequest previewRequest = state.request;
    previewRequest.mode = cr::CreativeSelectionPlacementMode::Copy;
    state.plan = cr::planCreativeSelectionPlacement(
        state.sourceClipboard.objects, previewRequest);
    if (!exact || !state.candidateTerrainTranslation.accepted) {
      state.plan.accepted = false;
      state.plan.status = cr::CreativeSelectionPlacementStatus::InvalidSource;
      state.plan.reasonCode =
          !state.moveAvailable
              ? "editor_transform_terrain_operation_source_stale"
          : !exact
              ? "editor_transform_terrain_operation_grid_delta_required"
              : std::string{state.candidateTerrainTranslation.reasonCode};
    } else {
      state.plan.reasonCode =
          std::string{state.candidateTerrainTranslation.reasonCode};
    }
    return;
  }
  state.request.mode = state.mode;
  state.plan = cr::planCreativeSelectionPlacement(
      state.sourceClipboard.objects, state.request);
  state.moveAvailable = moveSourceAvailable(appState, state.sourceClipboard);
  if (state.mode == cr::CreativeSelectionPlacementMode::Move &&
      !state.moveAvailable) {
    state.plan.accepted = false;
    state.plan.status = cr::CreativeSelectionPlacementStatus::InvalidSource;
    state.plan.reasonCode = "editor_transform_move_source_stale";
  }
}

void refreshTransformClearance(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    const CreativePlacementClearanceCache* cache) {
  state.clearance = {};
  state.clearanceCandidateObjectId = cr::kInvalidObjectId;
  state.clearanceCandidateObjectCount = 0U;
  if (!state.active || !state.targetPositionable || !state.plan.accepted) {
    return;
  }
  if (terrainOperationRoute(state)) {
    return;
  }
  const std::span<const cr::CreativeObjectId> ignoredObjectIds =
      state.mode == cr::CreativeSelectionPlacementMode::Move
          ? std::span<const cr::CreativeObjectId>{state.sourceObjectIds}
          : std::span<const cr::CreativeObjectId>{};
  const CreativeObjectSetClearanceResult result =
      evaluateCreativeObjectSetPlacementClearance(
          appState.facade.document(), state.plan.objects, ignoredObjectIds,
          cache);
  state.clearance = result.clearance;
  state.clearanceCandidateObjectId = result.candidateObjectId;
  state.clearanceCandidateObjectCount = result.candidateObjectCount;
  if (state.clearance.allowed) {
    return;
  }
  state.plan.accepted = false;
  state.plan.status = cr::CreativeSelectionPlacementStatus::Rejected;
  state.plan.failedObjectId = result.candidateObjectId;
  state.plan.reasonCode =
      std::string{"editor_transform_clearance_"} +
      std::string{cr::toString(state.clearance.status)};
}

void refreshResolvedTarget(const cr::CreativeAppState& appState,
                           CreativeEditorSelectionTransformState& state) {
  state.targetResolution = {};
  state.targetPositionable = false;
  if (!state.active || !state.aimTargetPositionable) {
    refreshTransformPlan(appState, state);
    return;
  }
  cr::CreativeSelectionPlacementTargetRequest target;
  target.sourceAnchor = state.request.sourceAnchor;
  target.aimedAnchor = state.aimTargetAnchor;
  target.nudgeOffset = state.nudgeOffset;
  target.axis = state.constraint;
  target.coordinateSpace = state.request.coordinateSpace;
  target.coordinateBasisEulerRadians =
      state.request.coordinateBasisEulerRadians;
  target.snapStepMeters = state.snapStepMeters;
  if (planarSourceRoute(state)) {
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    if (std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0) {
      target.snapStepMeters = grid.cellSizeMeters;
    }
    target.aimedAnchor.y = target.sourceAnchor.y;
    target.nudgeOffset.y = 0.0;
  }
  state.targetResolution =
      cr::resolveCreativeSelectionPlacementTarget(target);
  if (state.targetResolution.accepted) {
    state.targetPositionable = true;
    state.request.targetAnchor = state.targetResolution.targetAnchor;
  }
  refreshTransformPlan(appState, state);
}

[[nodiscard]] cr::CreativeSelectionPlacementAxis nextConstraint(
    cr::CreativeSelectionPlacementAxis constraint) noexcept {
  switch (constraint) {
    case cr::CreativeSelectionPlacementAxis::Free:
      return cr::CreativeSelectionPlacementAxis::X;
    case cr::CreativeSelectionPlacementAxis::X:
      return cr::CreativeSelectionPlacementAxis::Y;
    case cr::CreativeSelectionPlacementAxis::Y:
      return cr::CreativeSelectionPlacementAxis::Z;
    case cr::CreativeSelectionPlacementAxis::Z:
    case cr::CreativeSelectionPlacementAxis::Count:
      return cr::CreativeSelectionPlacementAxis::Free;
  }
  return cr::CreativeSelectionPlacementAxis::Free;
}

[[nodiscard]] cr::CreativeSelectionPlacementAxis stepConstraint(
    cr::CreativeSelectionPlacementAxis constraint,
    std::int32_t direction) noexcept {
  constexpr std::array rows{
      cr::CreativeSelectionPlacementAxis::Free,
      cr::CreativeSelectionPlacementAxis::X,
      cr::CreativeSelectionPlacementAxis::Y,
      cr::CreativeSelectionPlacementAxis::Z,
  };
  const auto found = std::find(rows.begin(), rows.end(), constraint);
  const std::size_t current =
      found == rows.end() ? 0U
                          : static_cast<std::size_t>(found - rows.begin());
  const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
      current, rows.size(), direction < 0 ? -1 : 1);
  return next.valid ? rows[next.index]
                    : cr::CreativeSelectionPlacementAxis::Free;
}

[[nodiscard]] cr::CreativeSelectionPlacementAxis stepPlanarConstraint(
    cr::CreativeSelectionPlacementAxis constraint,
    std::int32_t direction) noexcept {
  constexpr std::array rows{
      cr::CreativeSelectionPlacementAxis::Free,
      cr::CreativeSelectionPlacementAxis::X,
      cr::CreativeSelectionPlacementAxis::Z,
  };
  const auto found = std::find(rows.begin(), rows.end(), constraint);
  const std::size_t current =
      found == rows.end() ? 0U
                          : static_cast<std::size_t>(found - rows.begin());
  const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
      current, rows.size(), direction < 0 ? -1 : 1);
  return next.valid ? rows[next.index]
                    : cr::CreativeSelectionPlacementAxis::Free;
}

[[nodiscard]] bool transformScaleAvailable(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.capabilities.scale !=
         cr::CreativeObjectScaleSupport::None;
}

[[nodiscard]] bool transformRotationAvailable(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.capabilities.rotation !=
         cr::CreativeObjectRotationSupport::None;
}

[[nodiscard]] bool transformTranslationAvailable(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.capabilities.translate;
}

[[nodiscard]] bool transformModeAvailable(
    const CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformMode mode) noexcept {
  switch (mode) {
    case CreativeEditorTransformMode::Move:
      return transformTranslationAvailable(state);
    case CreativeEditorTransformMode::Rotate:
      return transformRotationAvailable(state);
    case CreativeEditorTransformMode::Scale:
      return transformScaleAvailable(state);
    case CreativeEditorTransformMode::Count:
      return false;
  }
  return false;
}

[[nodiscard]] CreativeEditorTransformMode firstAvailableTransformMode(
    const CreativeEditorSelectionTransformState& state) noexcept {
  for (const CreativeEditorTransformMode mode : {
           CreativeEditorTransformMode::Move,
           CreativeEditorTransformMode::Rotate,
           CreativeEditorTransformMode::Scale}) {
    if (transformModeAvailable(state, mode)) {
      return mode;
    }
  }
  return CreativeEditorTransformMode::Count;
}

[[nodiscard]] bool quarterTurnDegrees(double degrees) noexcept {
  const double turns = degrees / 90.0;
  return std::isfinite(turns) &&
         std::abs(turns - std::round(turns)) <= 1.0e-9;
}

[[nodiscard]] std::size_t scaleAxisIndex(
    cr::CreativeSelectionPlacementAxis axis) noexcept {
  switch (axis) {
    case cr::CreativeSelectionPlacementAxis::X: return 0U;
    case cr::CreativeSelectionPlacementAxis::Y: return 1U;
    case cr::CreativeSelectionPlacementAxis::Z: return 2U;
    case cr::CreativeSelectionPlacementAxis::Free:
    case cr::CreativeSelectionPlacementAxis::Count:
      return 3U;
  }
  return 3U;
}

[[nodiscard]] cr::CreativeAxis3 nextRotationAxis(
    cr::CreativeAxis3 axis) noexcept {
  switch (axis) {
    case cr::CreativeAxis3::Y: return cr::CreativeAxis3::X;
    case cr::CreativeAxis3::X: return cr::CreativeAxis3::Z;
    case cr::CreativeAxis3::Z:
    case cr::CreativeAxis3::Count:
      return cr::CreativeAxis3::Y;
  }
  return cr::CreativeAxis3::Y;
}

[[nodiscard]] cr::CreativeVec3 scaleFactorFromIndices(
    const CreativeEditorSelectionTransformState& state) noexcept {
  const auto factor = [](std::size_t index) {
    return index < kCreativeEditorScaleFactors.size()
               ? kCreativeEditorScaleFactors[index]
               : 1.0;
  };
  return {factor(state.scaleFactorIndices[0]),
          factor(state.scaleFactorIndices[1]),
          factor(state.scaleFactorIndices[2])};
}

void syncScaleFactor(CreativeEditorSelectionTransformState& state) noexcept {
  state.request.scaleFactor = scaleFactorFromIndices(state);
}

void resetScaleFactor(CreativeEditorSelectionTransformState& state) noexcept {
  state.scaleFactorIndices.fill(kCreativeEditorDefaultScaleIndex);
  syncScaleFactor(state);
}

void syncRotation(CreativeEditorSelectionTransformState& state) noexcept {
  state.request.quarterTurns = 0U;
  state.request.rotationAxis = state.rotationAxis;
  state.request.rotationRadians =
      state.rotationDegrees * std::numbers::pi / 180.0;
  state.request.hasAxisAngleRotation =
      std::abs(state.rotationDegrees) > 1.0e-12;
}

[[nodiscard]] bool stepScaleIndex(std::size_t& index,
                                  std::int32_t direction) noexcept {
  const std::size_t before = index;
  if (direction < 0 && index > 0U) {
    --index;
  } else if (direction > 0 &&
             index + 1U < kCreativeEditorScaleFactors.size()) {
    ++index;
  }
  return index != before;
}

[[nodiscard]] bool adjustScaleFactor(
    CreativeEditorSelectionTransformState& state,
    std::int32_t direction) noexcept {
  bool changed = false;
  const std::size_t axisIndex = scaleAxisIndex(state.constraint);
  if (axisIndex < state.scaleFactorIndices.size()) {
    changed = stepScaleIndex(state.scaleFactorIndices[axisIndex], direction);
  } else {
    for (std::size_t& index : state.scaleFactorIndices) {
      changed = stepScaleIndex(index, direction) || changed;
    }
  }
  if (changed) {
    syncScaleFactor(state);
  }
  return changed;
}

[[nodiscard]] bool adjustRotation(
    CreativeEditorSelectionTransformState& state,
    std::int32_t direction) noexcept {
  if (direction == 0) {
    return false;
  }
  const std::int8_t before = state.rotationQuarterSteps;
  const std::int32_t next =
      (static_cast<std::int32_t>(before) + (direction < 0 ? -1 : 1)) % 4;
  state.rotationQuarterSteps = static_cast<std::int8_t>(next);
  state.rotationDegrees =
      static_cast<double>(state.rotationQuarterSteps) * 90.0;
  syncRotation(state);
  return state.rotationQuarterSteps != before;
}

[[nodiscard]] const cr::CreativeObject* transformSourceObject(
    const CreativeEditorSelectionTransformState& state,
    cr::CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      state.sourceClipboard.objects.begin(), state.sourceClipboard.objects.end(),
      [objectId](const cr::CreativeObject& object) {
        return object.id == objectId;
      });
  return found == state.sourceClipboard.objects.end() ? nullptr : &*found;
}

[[nodiscard]] bool resolveTransformPivot(
    const CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformPivot pivot,
    cr::CreativeVec3& sourceAnchor,
    cr::CreativeSelectionPlacementPivotMode& pivotMode) noexcept {
  sourceAnchor = state.sourceClipboard.placementAnchor;
  pivotMode = cr::CreativeSelectionPlacementPivotMode::SharedAnchor;
  switch (pivot) {
    case CreativeEditorTransformPivot::SelectionAnchor:
      return cr::isFiniteCreativeVec3(sourceAnchor);
    case CreativeEditorTransformPivot::ActiveObjectOrigin: {
      const cr::CreativeObject* active =
          transformSourceObject(state, state.activeObjectId);
      return active != nullptr &&
             cr::resolveCreativeSelectionPlacementObjectOrigin(
                 *active, sourceAnchor);
    }
    case CreativeEditorTransformPivot::IndividualOrigins:
      pivotMode = cr::CreativeSelectionPlacementPivotMode::IndividualOrigins;
      return cr::isFiniteCreativeVec3(sourceAnchor);
    case CreativeEditorTransformPivot::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool resolveTransformCoordinateBasis(
    const CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementCoordinateSpace coordinateSpace,
    cr::CreativeVec3& basisEulerRadians) noexcept {
  basisEulerRadians = {};
  if (coordinateSpace ==
      cr::CreativeSelectionPlacementCoordinateSpace::World) {
    return true;
  }
  if (coordinateSpace !=
      cr::CreativeSelectionPlacementCoordinateSpace::Local) {
    return false;
  }
  const cr::CreativeObject* active =
      transformSourceObject(state, state.activeObjectId);
  if (active == nullptr) {
    return false;
  }
  if (cr::objectHasTransform(active->kind)) {
    basisEulerRadians = active->transform.rotationEulerRadians;
  }
  return cr::isFiniteCreativeVec3(basisEulerRadians);
}

[[nodiscard]] bool beginTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorTransformSource transformSource,
    cr::CreativeSelectionPlacementMode mode,
    CreativeEditorTransformAnchorPolicy anchorPolicy,
    cr::CreativeObjectId activeObjectId,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    const CreativeEditorWorldLayoutState* worldLayout) {
  state = {};
  state.preflight =
      inspectTransformSource(appState, clipboard, mode, worldLayout);
  if (!state.preflight.accepted) {
    SDL_Log("iggy3d_creative: TRANSFORM preview rejected source='%s' "
            "objectCount=%zu reasonCode='%s' failedObject=%llu",
            std::string(source).c_str(), clipboard.objects.size(),
            state.preflight.reasonCode.c_str(),
            static_cast<unsigned long long>(state.preflight.failedObjectId));
    return false;
  }
  state.active = true;
  state.source = transformSource;
  state.anchorPolicy = anchorPolicy;
  state.mode = mode;
  state.sourceClipboard = clipboard;
  state.sourceObjectIds.reserve(clipboard.objects.size());
  for (const cr::CreativeObject& object : clipboard.objects) {
    state.sourceObjectIds.push_back(object.id);
  }
  if (worldLayoutBuildingRoute(state) && worldLayout != nullptr) {
    state.sourceWorldLayout = worldLayout->source;
    state.sourceWorldLayoutNextStableOrdinal =
        worldLayout->nextStableOrdinal;
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    if (std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0) {
      state.snapStepMeters = grid.cellSizeMeters;
    }
  }
  state.transformMode = firstAvailableTransformMode(state);
  const cr::CreativeObject* activeSource =
      transformSourceObject(state, activeObjectId);
  state.activeObjectId =
      activeSource != nullptr ? activeSource->id : clipboard.objects.front().id;
  state.request.mode = mode;
  state.request.sourceAnchor = clipboard.placementAnchor;
  if (activeSource != nullptr &&
      activeSource->kind == cr::CreativeObjectKind::Group) {
    cr::CreativeVec3 groupPivot{};
    if (cr::resolveCreativeSelectionPlacementObjectOrigin(*activeSource,
                                                          groupPivot)) {
      state.pivot = CreativeEditorTransformPivot::ActiveObjectOrigin;
      state.request.sourceAnchor = groupPivot;
    }
  }
  state.moveAvailable = moveSourceAvailable(appState, clipboard);
  if (mode == cr::CreativeSelectionPlacementMode::Move &&
      !state.moveAvailable) {
    state.active = false;
    state.preflight.accepted = false;
    state.preflight.reasonCode = "editor_transform_move_source_stale";
    return false;
  }
  if (anchorPolicy == CreativeEditorTransformAnchorPolicy::FixedSource) {
    state.aimTargetPositionable = true;
    state.aimTargetAnchor = state.request.sourceAnchor;
    refreshResolvedTarget(appState, state);
  }
  SDL_Log("iggy3d_creative: TRANSFORM preview begun source='%s' mode='%s' "
          "objectCount=%zu anchor=(%.3f, %.3f, %.3f)",
          std::string(source).c_str(),
          std::string(cr::toString(mode)).c_str(), clipboard.objects.size(),
          clipboard.placementAnchor.x, clipboard.placementAnchor.y,
          clipboard.placementAnchor.z);
  return true;
}

[[nodiscard]] cr::CreativeSelectionPlacementReceipt placeObjectsWithHistory(
    cr::CreativeAppState& appState,
    std::span<const cr::CreativeObjectId> objectIds,
    const cr::CreativeSelectionPlacementRequest& request,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  cr::CreativeSelectionPlacementReceipt receipt =
      appState.facade.placeObjects(objectIds, request);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode));
  SDL_Log("iggy3d_creative: TRANSFORM move source='%s' accepted=%d changed=%d "
          "status='%s' objects=%llu reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          receipt.changed ? 1 : 0,
          std::string(cr::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.objectCount),
          receipt.reasonCode.c_str());
  return receipt;
}

[[nodiscard]] cr::CreativeClipboardPasteRequest clipboardPasteRequest(
    const CreativeEditorSelectionTransformState& state) noexcept {
  cr::CreativeClipboardPasteRequest request;
  request.offset = subtract(state.request.targetAnchor,
                            state.request.sourceAnchor);
  request.hasTransformAnchor = true;
  request.transformAnchor = state.request.sourceAnchor;
  request.pivotMode = state.request.pivotMode;
  request.coordinateSpace = state.request.coordinateSpace;
  request.coordinateBasisEulerRadians =
      state.request.coordinateBasisEulerRadians;
  request.scaleFactor = state.request.scaleFactor;
  request.quarterTurns = state.request.quarterTurns;
  request.mirrorX = state.request.mirrorX;
  request.mirrorZ = state.request.mirrorZ;
  request.hasAxisAngleRotation = state.request.hasAxisAngleRotation;
  request.rotationAxis = state.request.rotationAxis;
  request.rotationRadians = state.request.rotationRadians;
  return request;
}

[[nodiscard]] CreativeEditorTransformCommitReceipt
commitPatternRecipeTransform(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.patternReceipt = appState.facade.applyPatternRecipeTranslation(
      state.candidatePatternTranslation);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.patternReceipt.accepted && receipt.patternReceipt.changed,
      receipt.patternReceipt.reasonCode));
  receipt.accepted = receipt.patternReceipt.accepted;
  receipt.changed = receipt.patternReceipt.changed;
  receipt.reasonCode = std::string{receipt.patternReceipt.reasonCode};
  return receipt;
}

[[nodiscard]] CreativeEditorTransformCommitReceipt
commitTerrainOperationTransform(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.terrainReceipt = appState.facade.applyTerrainOperationTranslation(
      state.candidateTerrainTranslation);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.terrainReceipt.accepted && receipt.terrainReceipt.changed,
      receipt.terrainReceipt.reasonCode));
  receipt.accepted = receipt.terrainReceipt.accepted;
  receipt.changed = receipt.terrainReceipt.changed;
  receipt.reasonCode = std::string{receipt.terrainReceipt.reasonCode};
  return receipt;
}

[[nodiscard]] CreativeEditorTransformCommitReceipt
commitWorldLayoutBuildingTransform(
    cr::CreativeAppState& appState,
    CreativeEditorWorldLayoutState* worldLayout,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  if (worldLayout == nullptr || !state.candidateWorldLayoutReady ||
      state.preflight.worldLayoutSource.table !=
          cr::CreativeWorldLayoutTable::Building) {
    receipt.reasonCode =
        "editor_transform_world_layout_commit_owner_missing";
    return receipt;
  }
  if (worldLayout->revision != state.preflight.worldLayoutRevision ||
      worldLayout->sourceEpoch != state.preflight.worldLayoutSourceEpoch ||
      worldLayout->generatedRevision != worldLayout->revision) {
    receipt.reasonCode = "editor_transform_world_layout_commit_stale";
    return receipt;
  }
  if (!state.candidateWorldLayoutChanged) {
    receipt.accepted = true;
    receipt.reasonCode = "editor_transform_world_layout_no_change";
    return receipt;
  }
  if (worldLayout->revision ==
      std::numeric_limits<std::uint64_t>::max()) {
    receipt.reasonCode =
        "editor_transform_world_layout_revision_exhausted";
    return receipt;
  }

  CreativeEditorWorldLayoutSourceHistoryEntry sourceOnlyUndo =
      detail::captureWorldLayoutSourceHistoryEntry(*worldLayout);
  sourceOnlyUndo.source = std::string(source);
  CreativeEditorWorldLayoutSnapshot committed =
      captureCreativeEditorWorldLayoutSnapshot(*worldLayout);
  committed.source = state.candidateWorldLayout;
  committed.nextStableOrdinal =
      state.candidateWorldLayoutNextStableOrdinal;
  ++committed.revision;
  receipt.worldLayoutReceipt = applyCreativeEditorWorldLayoutPlanWithHistory(
      *worldLayout, appState, state.candidateWorldLayoutPlan,
      std::move(committed), source);
  receipt.accepted = receipt.worldLayoutReceipt.accepted;
  receipt.changed = receipt.accepted;
  receipt.reasonCode = receipt.worldLayoutReceipt.reasonCode;
  if (!receipt.accepted) {
    worldLayout->statusMessage = receipt.reasonCode;
    return receipt;
  }

  worldLayout->selection = {
      CreativeEditorWorldLayoutSelectionKind::Building,
      state.candidateWorldLayoutBuildingIndex};
  worldLayout->activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  repairCreativeEditorWorldLayoutActiveLevel(
      *worldLayout, state.candidateWorldLayoutBuildingIndex);
  if (!receipt.worldLayoutReceipt.changed) {
    detail::appendWorldLayoutSourceHistoryEntry(
        worldLayout->sourceHistory.undoEntries, std::move(sourceOnlyUndo),
        worldLayout->sourceHistory.maxDepth);
    worldLayout->sourceHistory.redoEntries.clear();
  }
  worldLayout->statusMessage =
      state.mode == cr::CreativeSelectionPlacementMode::Copy
          ? "building duplicated in 3D"
          : "building transformed in 3D";
  receipt.reasonCode =
      state.mode == cr::CreativeSelectionPlacementMode::Copy
          ? "editor_transform_world_layout_building_duplicated"
          : "editor_transform_world_layout_building_applied";
  return receipt;
}

}  // namespace

std::string_view toString(CreativeEditorTransformControl control) noexcept {
  switch (control) {
    case CreativeEditorTransformControl::RotatePositive: return "RotatePositive";
    case CreativeEditorTransformControl::MirrorX: return "MirrorX";
    case CreativeEditorTransformControl::CycleConstraint:
      return "CycleConstraint";
    case CreativeEditorTransformControl::ToggleMode: return "ToggleMode";
    case CreativeEditorTransformControl::Confirm: return "Confirm";
    case CreativeEditorTransformControl::Cancel: return "Cancel";
    case CreativeEditorTransformControl::MirrorZ: return "MirrorZ";
    case CreativeEditorTransformControl::RotateNegative: return "RotateNegative";
    case CreativeEditorTransformControl::Reset: return "Reset";
    case CreativeEditorTransformControl::Count: return "Count";
  }
  return "Unknown";
}

std::string_view toString(CreativeEditorTransformMode mode) noexcept {
  switch (mode) {
    case CreativeEditorTransformMode::Move: return "MOVE";
    case CreativeEditorTransformMode::Rotate: return "ROTATE";
    case CreativeEditorTransformMode::Scale: return "SCALE";
    case CreativeEditorTransformMode::Count: break;
  }
  return "INVALID";
}

cr::CreativeVec3 creativeEditorTransformScaleFactor(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.request.scaleFactor;
}

bool beginCreativeEditorClipboardTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    const CreativeEditorWorldLayoutState* worldLayout) {
  return beginTransformPreview(
      appState, clipboard, CreativeEditorTransformSource::Clipboard,
      cr::CreativeSelectionPlacementMode::Copy,
      CreativeEditorTransformAnchorPolicy::FollowAim,
      clipboard.objects.empty() ? cr::kInvalidObjectId
                                : clipboard.objects.front().id,
      state, source, worldLayout);
}

bool beginCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    CreativeEditorTransformAnchorPolicy anchorPolicy,
    const CreativeEditorWorldLayoutState* worldLayout) {
  cr::CreativeClipboard selection;
  const cr::CreativeClipboardCopyReceipt copied =
      appState.facade.copySelectedObjectsToClipboard(selection);
  if (!copied.accepted) {
    return false;
  }
  cr::CreativeObjectId activeObjectId = cr::kInvalidObjectId;
  const cr::TargetRef primary = appState.facade.selectionState().selectedTarget;
  if (primary.value != cr::kInvalidId) {
    activeObjectId = static_cast<cr::CreativeObjectId>(primary.value);
  }
  return beginTransformPreview(
      appState, selection, CreativeEditorTransformSource::Selection,
      cr::CreativeSelectionPlacementMode::Move, anchorPolicy, activeObjectId,
      state, source, worldLayout);
}

bool beginCreativeEditorTerrainOperationTransformPreview(
    const cr::CreativeAppState& appState,
    cr::CreativeTerrainOperationId operationId,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  state = {};
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativeTerrainOperation* operation =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       operationId);
  cr::CreativeTerrainHeightFieldBounds sourceBounds{};
  const cr::CreativeGridSettings grid = document.gridSettings();
  if (operation == nullptr ||
      operation->owner != cr::CreativeTerrainOperationOwner::Manual ||
      operation->kind == cr::CreativeTerrainOperationKind::GeneratedTerrain ||
      !cr::creativeTerrainOperationSpatialBounds(*operation, sourceBounds) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    state.preflight.requested = true;
    state.preflight.terrainOperationId = operationId;
    state.preflight.reasonCode =
        "editor_transform_terrain_operation_source_invalid";
    return false;
  }

  const double minimumX =
      static_cast<double>(sourceBounds.minimum.x) * grid.cellSizeMeters;
  const double minimumZ =
      static_cast<double>(sourceBounds.minimum.z) * grid.cellSizeMeters;
  const double maximumX =
      static_cast<double>(static_cast<std::int64_t>(sourceBounds.minimum.x) +
                          sourceBounds.widthCells) *
      grid.cellSizeMeters;
  const double maximumZ =
      static_cast<double>(static_cast<std::int64_t>(sourceBounds.minimum.z) +
                          sourceBounds.depthCells) *
      grid.cellSizeMeters;
  const cr::CreativeBounds worldBounds{
      {minimumX, 0.0, minimumZ},
      {maximumX, grid.cellSizeMeters, maximumZ}};
  if (!cr::measureCreativeBounds(worldBounds).valid) {
    state.preflight.requested = true;
    state.preflight.terrainOperationId = operationId;
    state.preflight.reasonCode =
        "editor_transform_terrain_operation_bounds_invalid";
    return false;
  }

  cr::CreativeObject proxy;
  proxy.id = std::numeric_limits<cr::CreativeObjectId>::max();
  proxy.kind = cr::CreativeObjectKind::Room;
  proxy.name = "Terrain operation transform preview";
  proxy.bounds = worldBounds;
  cr::CreativeClipboard clipboard;
  clipboard.sourceDocumentId = document.id();
  clipboard.sourceRevision = document.revision();
  clipboard.hasPlacementAnchor = true;
  clipboard.placementAnchor = worldBounds.min;
  clipboard.objects.push_back(std::move(proxy));

  state.active = true;
  state.source = CreativeEditorTransformSource::Selection;
  state.transformMode = CreativeEditorTransformMode::Move;
  state.anchorPolicy = CreativeEditorTransformAnchorPolicy::FixedSource;
  state.pivot = CreativeEditorTransformPivot::SelectionAnchor;
  state.mode = cr::CreativeSelectionPlacementMode::Move;
  state.constraint = cr::CreativeSelectionPlacementAxis::Free;
  state.activeObjectId = clipboard.objects.front().id;
  state.snapStepMeters = grid.cellSizeMeters;
  state.sourceClipboard = std::move(clipboard);
  state.request.mode = state.mode;
  state.request.sourceAnchor = worldBounds.min;
  state.preflight.requested = true;
  state.preflight.accepted = true;
  state.preflight.ownershipRoute =
      CreativeEditorTransformOwnershipRoute::TerrainOperation;
  state.preflight.terrainOperationId = operationId;
  state.preflight.sourceDocumentRevision = document.revision();
  state.preflight.capabilities = {
      true, 1U, true, cr::CreativeObjectRotationSupport::None,
      cr::CreativeObjectScaleSupport::None, false};
  state.preflight.reasonCode =
      "editor_transform_terrain_operation_ready";
  state.moveAvailable = true;
  state.aimTargetPositionable = true;
  state.aimTargetAnchor = state.request.sourceAnchor;
  refreshResolvedTarget(appState, state);
  SDL_Log("iggy3d_creative: TERRAIN_OPERATION transform begun source='%s' "
          "operation=%llu kind='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(operationId),
          std::string(cr::toString(operation->kind)).c_str());
  return state.plan.accepted;
}

bool requestCreativeEditorSelectionTransformCommit(
    CreativeEditorSelectionTransformState& state) noexcept {
  if (!state.active || state.commitRequested) {
    return false;
  }
  state.commitRequested = true;
  return true;
}

bool cancelCreativeEditorSelectionTransformPreview(
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  if (!state.active) {
    return false;
  }
  SDL_Log("iggy3d_creative: TRANSFORM preview cancelled source='%s'",
          std::string(source).c_str());
  state = {};
  return true;
}

bool setCreativeEditorTransformConstraint(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis constraint) {
  if (!state.active || !transformTranslationAvailable(state) ||
      static_cast<std::size_t>(constraint) >=
          static_cast<std::size_t>(
              cr::CreativeSelectionPlacementAxis::Count) ||
      (planarSourceRoute(state) &&
       constraint == cr::CreativeSelectionPlacementAxis::Y)) {
    return false;
  }
  if (state.constraint == constraint) {
    return false;
  }
  state.constraint = constraint;
  state.lastCommit = {};
  state.lastNudge = {};
  refreshResolvedTarget(appState, state);
  return true;
}

bool setCreativeEditorTransformTargetAnchor(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 targetAnchor) {
  if (!state.active || !transformTranslationAvailable(state) ||
      !cr::isFiniteCreativeVec3(targetAnchor)) {
    return false;
  }
  const bool changed =
      state.anchorPolicy != CreativeEditorTransformAnchorPolicy::FixedTarget ||
      !cr::creativeVec3ExactlyEqual(state.aimTargetAnchor, targetAnchor) ||
      !cr::creativeVec3ExactlyEqual(state.nudgeOffset, {});
  if (!changed) {
    return false;
  }
  state.anchorPolicy = CreativeEditorTransformAnchorPolicy::FixedTarget;
  state.aimTargetPositionable = true;
  state.aimTargetAnchor = targetAnchor;
  state.nudgeOffset = {};
  state.lastNudge = {};
  state.lastCommit = {};
  refreshResolvedTarget(appState, state);
  return true;
}

bool setCreativeEditorTransformRotationDegrees(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeAxis3 axis, double degrees) {
  if (!state.active || !transformRotationAvailable(state) ||
      !cr::isValidCreativeAxis3(axis) ||
      !std::isfinite(degrees) ||
      (worldLayoutBuildingRoute(state) && axis != cr::CreativeAxis3::Y)) {
    return false;
  }
  const double normalized = std::remainder(degrees, 360.0);
  if (state.preflight.capabilities.rotation ==
          cr::CreativeObjectRotationSupport::QuarterTurns &&
      !quarterTurnDegrees(normalized)) {
    return false;
  }
  if (state.rotationAxis == axis && state.rotationDegrees == normalized) {
    return false;
  }
  state.transformMode = CreativeEditorTransformMode::Rotate;
  state.rotationAxis = axis;
  state.rotationDegrees = normalized;
  const double quarterTurns = normalized / 90.0;
  state.rotationQuarterSteps =
      std::abs(quarterTurns - std::round(quarterTurns)) <= 1.0e-12
          ? static_cast<std::int8_t>(
                static_cast<std::int32_t>(std::llround(quarterTurns)) % 4)
          : 0;
  syncRotation(state);
  state.lastCommit = {};
  refreshTransformPlan(appState, state);
  return true;
}

bool setCreativeEditorTransformScaleFactor(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 scaleFactor) {
  if (!state.active || !transformScaleAvailable(state) ||
      !cr::isPositiveCreativeVec3(scaleFactor)) {
    return false;
  }
  if (cr::creativeVec3ExactlyEqual(state.request.scaleFactor, scaleFactor)) {
    return false;
  }
  if (state.preflight.capabilities.scale ==
          cr::CreativeObjectScaleSupport::Uniform &&
      (std::abs(scaleFactor.x - scaleFactor.y) > 1.0e-12 ||
       std::abs(scaleFactor.x - scaleFactor.z) > 1.0e-12)) {
    return false;
  }
  state.transformMode = CreativeEditorTransformMode::Scale;
  state.request.scaleFactor = scaleFactor;
  const auto nearestIndex = [](double value) {
    std::size_t best = 0U;
    double distance = std::abs(kCreativeEditorScaleFactors.front() - value);
    for (std::size_t index = 1U;
         index < kCreativeEditorScaleFactors.size(); ++index) {
      const double candidate =
          std::abs(kCreativeEditorScaleFactors[index] - value);
      if (candidate < distance) {
        best = index;
        distance = candidate;
      }
    }
    return best;
  };
  state.scaleFactorIndices = {
      nearestIndex(scaleFactor.x), nearestIndex(scaleFactor.y),
      nearestIndex(scaleFactor.z)};
  state.lastCommit = {};
  refreshTransformPlan(appState, state);
  return true;
}

bool setCreativeEditorTransformPlacementMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementMode mode) {
  if (!state.active ||
      (mode != cr::CreativeSelectionPlacementMode::Move &&
       mode != cr::CreativeSelectionPlacementMode::Copy) ||
      state.mode == mode ||
      (mode == cr::CreativeSelectionPlacementMode::Move &&
       !state.moveAvailable) ||
      (mode == cr::CreativeSelectionPlacementMode::Copy &&
       (patternRecipeRoute(state) || terrainOperationRoute(state)))) {
    return false;
  }
  state.mode = mode;
  state.request.mode = mode;
  state.lastCommit = {};
  refreshTransformPlan(appState, state);
  return true;
}

bool setCreativeEditorTransformPivot(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformPivot pivot) {
  if (!state.active || pivot == state.pivot ||
      pivot == CreativeEditorTransformPivot::Count ||
      (planarSourceRoute(state) &&
       pivot != CreativeEditorTransformPivot::SelectionAnchor)) {
    return false;
  }

  cr::CreativeVec3 sourceAnchor{};
  cr::CreativeSelectionPlacementPivotMode pivotMode{};
  if (!resolveTransformPivot(state, pivot, sourceAnchor, pivotMode)) {
    return false;
  }

  state.pivot = pivot;
  state.request.sourceAnchor = sourceAnchor;
  state.request.pivotMode = pivotMode;
  if (state.anchorPolicy == CreativeEditorTransformAnchorPolicy::FixedSource) {
    state.aimTargetPositionable = true;
    state.aimTargetAnchor = sourceAnchor;
  }
  state.nudgeOffset = {};
  state.lastNudge = {};
  state.lastCommit = {};
  refreshResolvedTarget(appState, state);
  return true;
}

bool setCreativeEditorTransformCoordinateSpace(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementCoordinateSpace coordinateSpace) {
  if (!state.active || state.request.coordinateSpace == coordinateSpace) {
    return false;
  }
  if (planarSourceRoute(state) &&
      coordinateSpace !=
          cr::CreativeSelectionPlacementCoordinateSpace::World) {
    return false;
  }
  cr::CreativeVec3 basisEulerRadians{};
  if (!resolveTransformCoordinateBasis(state, coordinateSpace,
                                       basisEulerRadians)) {
    return false;
  }
  state.request.coordinateSpace = coordinateSpace;
  state.request.coordinateBasisEulerRadians = basisEulerRadians;
  state.nudgeOffset = {};
  state.lastNudge = {};
  state.lastCommit = {};
  refreshResolvedTarget(appState, state);
  return true;
}

bool resumeCreativeEditorTransformAim(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state) {
  if (!state.active ||
      state.anchorPolicy == CreativeEditorTransformAnchorPolicy::FollowAim) {
    return false;
  }
  state.anchorPolicy = CreativeEditorTransformAnchorPolicy::FollowAim;
  state.lastCommit = {};
  refreshResolvedTarget(appState, state);
  return true;
}

CreativeEditorTransformAxisRaySample sampleCreativeEditorTransformAxisRay(
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection) noexcept {
  CreativeEditorTransformAxisRaySample result;
  if (!cr::isFiniteCreativeVec3(rayOrigin) ||
      !cr::isFiniteCreativeVec3(axisOrigin)) {
    return result;
  }
  cr::CreativeVec3 ray{};
  cr::CreativeVec3 axis{};
  if (!normalized(rayDirection, ray) || !normalized(axisDirection, axis)) {
    return result;
  }

  const cr::CreativeVec3 fromAxis = subtract(rayOrigin, axisOrigin);
  const double rayAxisDot = dot(ray, axis);
  const double rayFromAxisDot = dot(ray, fromAxis);
  const double axisFromAxisDot = dot(axis, fromAxis);
  const double denominator = 1.0 - rayAxisDot * rayAxisDot;
  if (!std::isfinite(denominator) || denominator <= 1.0e-8) {
    return result;
  }
  result.rayParameter =
      (rayAxisDot * axisFromAxisDot - rayFromAxisDot) / denominator;
  result.axisParameter =
      (axisFromAxisDot - rayAxisDot * rayFromAxisDot) / denominator;
  result.valid = std::isfinite(result.rayParameter) &&
                 std::isfinite(result.axisParameter) &&
                 result.rayParameter >= 0.0;
  if (!result.valid) {
    result = {};
  }
  return result;
}

bool beginCreativeEditorFreeTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 initialAimAnchor) {
  if (!state.active || !transformTranslationAvailable(state) ||
      !cr::isFiniteCreativeVec3(initialAimAnchor)) {
    return false;
  }
  if (state.constraint != cr::CreativeSelectionPlacementAxis::Free) {
    static_cast<void>(setCreativeEditorTransformConstraint(
        appState, state, cr::CreativeSelectionPlacementAxis::Free));
  }
  state.pointerGesture = {};
  state.pointerGesture.kind = CreativeEditorTransformPointerGestureKind::Free;
  state.pointerGesture.initialAimAnchor = initialAimAnchor;
  return true;
}

bool beginCreativeEditorAxisTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis axis,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection) {
  if (!state.active || !transformTranslationAvailable(state) ||
      axis == cr::CreativeSelectionPlacementAxis::Free ||
      axis == cr::CreativeSelectionPlacementAxis::Count) {
    return false;
  }
  cr::CreativeVec3 normalizedAxis{};
  if (!normalized(axisDirection, normalizedAxis)) {
    return false;
  }
  const CreativeEditorTransformAxisRaySample sample =
      sampleCreativeEditorTransformAxisRay(
          rayOrigin, rayDirection, axisOrigin, normalizedAxis);
  if (!sample.valid) {
    return false;
  }
  if (state.constraint != axis) {
    static_cast<void>(setCreativeEditorTransformConstraint(appState, state,
                                                           axis));
  }
  if (state.constraint != axis) {
    return false;
  }
  state.pointerGesture = {};
  state.pointerGesture.kind = CreativeEditorTransformPointerGestureKind::Axis;
  state.pointerGesture.axis = axis;
  state.pointerGesture.axisOrigin = axisOrigin;
  state.pointerGesture.axisDirection = normalizedAxis;
  state.pointerGesture.initialAxisParameter = sample.axisParameter;
  return true;
}

bool updateCreativeEditorTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection) {
  if (!state.active ||
      state.pointerGesture.kind ==
          CreativeEditorTransformPointerGestureKind::None) {
    return false;
  }

  cr::CreativeVec3 resolvedTarget{};
  if (state.pointerGesture.kind ==
      CreativeEditorTransformPointerGestureKind::Free) {
    if (!targetPositionable || !cr::isFiniteCreativeVec3(targetAnchor)) {
      return false;
    }
    resolvedTarget = add(
        state.request.sourceAnchor,
        subtract(targetAnchor, state.pointerGesture.initialAimAnchor));
  } else {
    const CreativeEditorTransformAxisRaySample sample =
        sampleCreativeEditorTransformAxisRay(
            rayOrigin, rayDirection, state.pointerGesture.axisOrigin,
            state.pointerGesture.axisDirection);
    if (!sample.valid) {
      return false;
    }
    resolvedTarget = add(
        state.request.sourceAnchor,
        scale(state.pointerGesture.axisDirection,
              sample.axisParameter -
                  state.pointerGesture.initialAxisParameter));
  }
  if (!cr::isFiniteCreativeVec3(resolvedTarget)) {
    return false;
  }
  static_cast<void>(
      setCreativeEditorTransformTargetAnchor(appState, state, resolvedTarget));
  state.pointerGesture.changed =
      state.pointerGesture.changed ||
      !cr::creativeVec3ExactlyEqual(state.request.targetAnchor,
                                    state.request.sourceAnchor);
  return true;
}

bool finishCreativeEditorTransformPointerGesture(
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  if (!state.active ||
      state.pointerGesture.kind ==
          CreativeEditorTransformPointerGestureKind::None) {
    return false;
  }
  const bool changed =
      state.pointerGesture.changed && state.targetPositionable &&
      state.plan.accepted &&
      !cr::creativeVec3ExactlyEqual(state.request.targetAnchor,
                                    state.request.sourceAnchor);
  state.pointerGesture = {};
  if (!changed) {
    return cancelCreativeEditorSelectionTransformPreview(state, source);
  }
  return requestCreativeEditorSelectionTransformCommit(state);
}

bool nudgeCreativeEditorSelectionTransform(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t steps,
    bool fine) {
  if (!state.active || !transformTranslationAvailable(state)) {
    return false;
  }
  cr::CreativeSelectionPlacementNudgeRequest nudge;
  nudge.offset = state.nudgeOffset;
  nudge.axis = state.constraint;
  nudge.coordinateSpace = state.request.coordinateSpace;
  nudge.coordinateBasisEulerRadians =
      state.request.coordinateBasisEulerRadians;
  nudge.snapStepMeters = state.snapStepMeters;
  nudge.steps = steps;
  nudge.fine = planarSourceRoute(state) ? false : fine;
  state.lastNudge = cr::nudgeCreativeSelectionPlacementOffset(nudge);
  if (!state.lastNudge.changed) {
    return false;
  }
  state.nudgeOffset = state.lastNudge.offset;
  state.lastCommit = {};
  refreshResolvedTarget(appState, state);
  return true;
}

bool cycleCreativeEditorTransformMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state) {
  if (!state.active) {
    return false;
  }
  const CreativeEditorTransformMode before = state.transformMode;
  constexpr std::array modes{
      CreativeEditorTransformMode::Move,
      CreativeEditorTransformMode::Rotate,
      CreativeEditorTransformMode::Scale};
  const auto found = std::find(modes.begin(), modes.end(), before);
  const std::size_t current =
      found == modes.end() ? 0U : static_cast<std::size_t>(found - modes.begin());
  for (std::size_t offset = 1U; offset <= modes.size(); ++offset) {
    const CreativeEditorTransformMode candidate =
        modes[(current + offset) % modes.size()];
    if (transformModeAvailable(state, candidate)) {
      state.transformMode = candidate;
      break;
    }
  }
  if (state.transformMode == CreativeEditorTransformMode::Rotate) {
    syncRotation(state);
  }
  if (state.transformMode == before) {
    return false;
  }
  state.lastCommit = {};
  refreshTransformPlan(appState, state);
  return true;
}

bool adjustCreativeEditorTransformSetting(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t direction) {
  if (!state.active || direction == 0) {
    return false;
  }
  switch (state.transformMode) {
    case CreativeEditorTransformMode::Move: {
      if (!transformTranslationAvailable(state)) {
        return false;
      }
      const cr::CreativeSelectionPlacementAxis next =
          planarSourceRoute(state)
              ? stepPlanarConstraint(state.constraint, direction)
              : stepConstraint(state.constraint, direction);
      if (next == state.constraint) {
        return false;
      }
      state.constraint = next;
      state.lastNudge = {};
      state.lastCommit = {};
      refreshResolvedTarget(appState, state);
      return true;
    }
    case CreativeEditorTransformMode::Rotate: {
      if (!transformRotationAvailable(state)) {
        return false;
      }
      const bool changed = adjustRotation(state, direction);
      if (!changed) {
        return false;
      }
      state.lastCommit = {};
      refreshTransformPlan(appState, state);
      return true;
    }
    case CreativeEditorTransformMode::Scale: {
      if (!transformScaleAvailable(state)) {
        return false;
      }
      if (!adjustScaleFactor(state, direction)) {
        return false;
      }
      state.lastCommit = {};
      refreshTransformPlan(appState, state);
      return true;
    }
    case CreativeEditorTransformMode::Count:
      return false;
  }
  return false;
}

bool applyCreativeEditorTransformControl(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformControl control) {
  if (!state.active || control == CreativeEditorTransformControl::Count) {
    return false;
  }
  bool changed = true;
  bool targetChanged = false;
  switch (control) {
    case CreativeEditorTransformControl::RotatePositive:
      if (!transformRotationAvailable(state)) return false;
      state.transformMode = CreativeEditorTransformMode::Rotate;
      changed = adjustRotation(state, 1);
      break;
    case CreativeEditorTransformControl::MirrorX:
      if (!state.preflight.capabilities.mirror) return false;
      state.transformMode = CreativeEditorTransformMode::Rotate;
      state.request.mirrorX = !state.request.mirrorX;
      break;
    case CreativeEditorTransformControl::CycleConstraint:
      if (state.transformMode == CreativeEditorTransformMode::Rotate) {
        if (planarSourceRoute(state)) {
          return false;
        }
        state.rotationAxis = nextRotationAxis(state.rotationAxis);
        syncRotation(state);
      } else {
        state.constraint =
            planarSourceRoute(state)
                ? stepPlanarConstraint(state.constraint, 1)
                : nextConstraint(state.constraint);
      }
      state.lastNudge = {};
      targetChanged = state.transformMode == CreativeEditorTransformMode::Move;
      break;
    case CreativeEditorTransformControl::ToggleMode:
      if (patternRecipeRoute(state) || terrainOperationRoute(state)) {
        return false;
      }
      if (state.mode == cr::CreativeSelectionPlacementMode::Move) {
        state.mode = cr::CreativeSelectionPlacementMode::Copy;
        resetScaleFactor(state);
        if (state.transformMode == CreativeEditorTransformMode::Scale) {
          state.transformMode = CreativeEditorTransformMode::Move;
        }
      } else if (state.moveAvailable) {
        state.mode = cr::CreativeSelectionPlacementMode::Move;
      } else {
        changed = false;
      }
      break;
    case CreativeEditorTransformControl::Confirm:
      changed = requestCreativeEditorSelectionTransformCommit(state);
      state.controlsOpen = false;
      break;
    case CreativeEditorTransformControl::Cancel:
      return cancelCreativeEditorSelectionTransformPreview(
          state, "transform_control_cancel");
    case CreativeEditorTransformControl::MirrorZ:
      if (!state.preflight.capabilities.mirror) return false;
      state.transformMode = CreativeEditorTransformMode::Rotate;
      state.request.mirrorZ = !state.request.mirrorZ;
      break;
    case CreativeEditorTransformControl::RotateNegative:
      if (!transformRotationAvailable(state)) return false;
      state.transformMode = CreativeEditorTransformMode::Rotate;
      changed = adjustRotation(state, -1);
      break;
    case CreativeEditorTransformControl::Reset:
      changed = state.rotationQuarterSteps != 0 || state.request.mirrorX ||
                state.request.mirrorZ ||
                state.constraint != cr::CreativeSelectionPlacementAxis::Free ||
                state.rotationAxis != cr::CreativeAxis3::Y ||
                !cr::creativeVec3ExactlyEqual(state.nudgeOffset, {}) ||
                std::any_of(state.scaleFactorIndices.begin(),
                            state.scaleFactorIndices.end(),
                            [](std::size_t index) {
                              return index != kCreativeEditorDefaultScaleIndex;
                            }) ||
                state.transformMode != CreativeEditorTransformMode::Move;
      state.rotationQuarterSteps = 0;
      state.rotationDegrees = 0.0;
      state.constraint = cr::CreativeSelectionPlacementAxis::Free;
      state.rotationAxis = cr::CreativeAxis3::Y;
      syncRotation(state);
      state.request.mirrorX = false;
      state.request.mirrorZ = false;
      resetScaleFactor(state);
      state.transformMode = CreativeEditorTransformMode::Move;
      state.nudgeOffset = {};
      state.lastNudge = {};
      targetChanged = true;
      break;
    case CreativeEditorTransformControl::Count:
      return false;
  }
  if (changed && state.active) {
    state.lastCommit = {};
    if (targetChanged) {
      refreshResolvedTarget(appState, state);
    } else {
      refreshTransformPlan(appState, state);
    }
  }
  return changed;
}

CreativeEditorTransformCommitReceipt
processCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    bool secondaryPressed,
    std::string_view source,
    double snapStepMeters,
    CreativeEditorWorldLayoutState* worldLayout,
    const CreativePlacementClearanceCache* clearanceCache) {
  if (!state.active) {
    return {};
  }

  const bool previousPositionable = state.targetPositionable;
  const cr::CreativeVec3 previousTarget = state.request.targetAnchor;
  if (state.anchorPolicy == CreativeEditorTransformAnchorPolicy::FixedSource) {
    state.aimTargetPositionable = true;
    state.aimTargetAnchor = state.request.sourceAnchor;
  } else if (state.anchorPolicy ==
             CreativeEditorTransformAnchorPolicy::FollowAim) {
    state.aimTargetPositionable =
        targetPositionable && cr::isFiniteCreativeVec3(targetAnchor);
    if (state.aimTargetPositionable) {
      state.aimTargetAnchor = targetAnchor;
    }
  }
  state.snapStepMeters = snapStepMeters;
  if (planarSourceRoute(state)) {
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    if (std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0) {
      state.snapStepMeters = grid.cellSizeMeters;
    }
  }
  refreshResolvedTarget(appState, state);
  refreshTransformClearance(appState, state, clearanceCache);
  const bool targetChanged =
      previousPositionable != state.targetPositionable ||
      (state.targetPositionable &&
       !cr::creativeVec3ExactlyEqual(previousTarget,
                                     state.request.targetAnchor));
  if (targetChanged) {
    state.lastCommit = {};
  }
  if (secondaryPressed) {
    static_cast<void>(requestCreativeEditorSelectionTransformCommit(state));
  }
  if (!state.commitRequested) {
    return {};
  }
  state.commitRequested = false;

  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  if (!state.targetPositionable || !state.plan.accepted) {
    receipt.reasonCode = state.targetPositionable
                             ? state.plan.reasonCode
                             : "editor_transform_target_unavailable";
    state.lastCommit = receipt;
    return receipt;
  }

  if (worldLayoutBuildingRoute(state)) {
    receipt = commitWorldLayoutBuildingTransform(
        appState, worldLayout, state, source);
  } else if (patternRecipeRoute(state)) {
    receipt = commitPatternRecipeTransform(appState, state, source);
  } else if (terrainOperationRoute(state)) {
    receipt = commitTerrainOperationTransform(appState, state, source);
  } else if (state.mode == cr::CreativeSelectionPlacementMode::Copy) {
    receipt.copyReceipt = pasteClipboardWithHistory(
        appState, state.sourceClipboard, clipboardPasteRequest(state), source);
    receipt.accepted = receipt.copyReceipt.accepted;
    receipt.changed = receipt.copyReceipt.changed;
    receipt.reasonCode = receipt.copyReceipt.reasonCode;
  } else {
    receipt.moveReceipt =
        placeObjectsWithHistory(appState, state.sourceObjectIds, state.request,
                                source);
    receipt.accepted = receipt.moveReceipt.accepted;
    receipt.changed = receipt.moveReceipt.changed;
    receipt.reasonCode = receipt.moveReceipt.reasonCode;
  }
  state.lastCommit = receipt;
  if (receipt.accepted && receipt.changed) {
    state.active = false;
    state.controlsOpen = false;
    state.targetPositionable = false;
  }
  return receipt;
}

}  // namespace iggy3d_creative_app
