#include "EditorTransform.hpp"
#include "EditorTransformInternal.hpp"

#include <SDL3/SDL_log.h>

#include <cmath>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorWorldLayoutBuildings.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutInternal.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d_creative_app {
namespace {

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
  request.offset = detail::subtract(state.request.targetAnchor,
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
  if (detail::planarSourceRoute(state)) {
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    if (std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0) {
      state.snapStepMeters = grid.cellSizeMeters;
    }
  }
  detail::refreshResolvedTarget(appState, state);
  detail::refreshTransformClearance(appState, state, clearanceCache);
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

  if (detail::worldLayoutBuildingRoute(state)) {
    receipt = commitWorldLayoutBuildingTransform(
        appState, worldLayout, state, source);
  } else if (detail::patternRecipeRoute(state)) {
    receipt = commitPatternRecipeTransform(appState, state, source);
  } else if (detail::terrainOperationRoute(state)) {
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
