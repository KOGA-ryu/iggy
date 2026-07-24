#include "EditorTransformInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>

#include "EditorPlacementClearance.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/RecipeTransform.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

namespace iggy3d_creative_app::detail {

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

namespace {

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

}  // namespace

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

}  // namespace iggy3d_creative_app::detail
