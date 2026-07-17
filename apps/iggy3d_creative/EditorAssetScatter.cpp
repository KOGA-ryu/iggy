#include "EditorAssetScatter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string_view>
#include <utility>
#include <vector>

#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"

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

[[nodiscard]] cr::CreativeGridTarget syntheticScatterTarget(
    const CreativeEditorState& editor,
    cr::CreativeVec3 position) noexcept {
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
      editor.interaction.target.terrainHit
          ? cr::CreativePlacementTargetSource::Terrain
          : cr::CreativePlacementTargetSource::EmptyPlane,
      editor.interaction.target.terrainHit
          ? cr::CreativeObjectKind::TerrainPatch
          : cr::CreativeObjectKind::Unknown);
  target.valid = true;
  return target;
}

[[nodiscard]] bool existingAssetWithinSpacing(
    const cr::CreativeDocument& document,
    std::string_view assetId,
    const CreativeBrushPlacementPlan& placement,
    double spacingMeters) noexcept {
  const cr::CreativeTransformedBounds candidate =
      cr::resolveCreativeTransformedBounds(placement.previewBounds,
                                           placement.transform);
  if (!candidate.valid) {
    return true;
  }
  const double spacingSquared = spacingMeters * spacingMeters;
  for (const cr::CreativeObject& object : document.objects()) {
    if (object.assetId != assetId) {
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

[[nodiscard]] CreativeEditorAssetScatterCandidate buildEditorCandidate(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeHotbarEntry& held,
    const cr::CreativeAssetScatterCandidate& kernelCandidate,
    double spacingMeters,
    double maximumSlopeRadians) noexcept {
  CreativeEditorAssetScatterCandidate candidate;
  candidate.surfacePosition = kernelCandidate.position;
  bool surfaceAllowed = true;
  if (editor.interaction.target.terrainHit) {
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
      if (surface.slopeRadians > maximumSlopeRadians + 1.0e-10) {
        candidate.status =
            CreativeEditorAssetScatterCandidateStatus::SlopeRejected;
        surfaceAllowed = false;
      }
    }
  }

  const cr::CreativeGridTarget target =
      syntheticScatterTarget(editor, candidate.surfacePosition);
  CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      held.objectKind, target, editor.toolSettings.placementYaw);
  if (!admission.allowed) {
    candidate.status =
        CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
    return candidate;
  }
  admission.plan.transform.rotationEulerRadians.y +=
      kernelCandidate.yawOffsetRadians;
  admission.plan.transform.scale = {kernelCandidate.uniformScale,
                                    kernelCandidate.uniformScale,
                                    kernelCandidate.uniformScale};
  if (!held.hasAssetBounds ||
      !applyCreativeAssetPlacementBounds(admission.plan,
                                         held.assetSourceBounds)) {
    candidate.status =
        CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
    return candidate;
  }
  candidate.placement = admission.plan;
  candidate.visitedKey = cr::creativeAssetScatterSpatialKey(
      candidate.surfacePosition, std::max(0.01, spacingMeters * 0.5));
  if (!surfaceAllowed) {
    return candidate;
  }
  const std::string_view assetId = cr::creativeHotbarAssetId(held);
  if (creativeBrushPlacementTargetOccupied(document, candidate.placement,
                                            assetId) ||
      existingAssetWithinSpacing(document, assetId, candidate.placement,
                                 spacingMeters)) {
    candidate.status = CreativeEditorAssetScatterCandidateStatus::Occupied;
    return candidate;
  }
  candidate.status = CreativeEditorAssetScatterCandidateStatus::Ready;
  candidate.placeable = true;
  return candidate;
}

void rejectScatter(CreativeEditorState& editor,
                   cr::CreativeObjectKind objectKind) {
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex, objectKind);
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

void applyScatterRemoval(cr::CreativeAppState& appState,
                         CreativeEditorState& editor,
                         const cr::CreativeHotbarEntry& held) {
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  const std::uint64_t key = removalVisitedKey(target);
  if (stroke.capacityReached || key == 0U ||
      cr::creativeWorldGestureVisited(stroke.visited, key)) {
    if (key == 0U || stroke.capacityReached) {
      rejectScatter(editor, target.objectKind);
    }
    return;
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
    const cr::CreativeDocumentRemoveReceipt receipt =
        appState.facade.removeDocumentObject(target.objectId);
    changed = receipt.accepted && receipt.objectRemoved && receipt.changed;
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

void applyScatterPlacement(cr::CreativeAppState& appState,
                           CreativeEditorState& editor,
                           const cr::CreativeHotbarEntry& held) {
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  if (stroke.capacityReached) {
    rejectScatter(editor, held.objectKind);
    return;
  }
  const std::size_t remaining = stroke.visited.keys.size() -
                                static_cast<std::size_t>(stroke.visited.count);
  std::vector<cr::CreativeDocumentCreateRequest> requests;
  std::vector<std::uint64_t> visitedKeys;
  requests.reserve(std::min(remaining, stroke.preview.placeableCount));
  visitedKeys.reserve(requests.capacity());
  const cr::CreativeObjectId parentId =
      activeCreativeEditorGroupFocusId(editor.groupFocus);
  for (const CreativeEditorAssetScatterCandidate& candidate :
       stroke.preview.items()) {
    if (requests.size() >= remaining) {
      stroke.capacityReached = true;
      break;
    }
    if (!candidate.placeable ||
        cr::creativeWorldGestureVisited(stroke.visited,
                                        candidate.visitedKey)) {
      continue;
    }
    cr::CreativeDocumentCreateRequest request = buildBrushCreateRequest(
        candidate.placement, editor.placedCount + requests.size() + 1U,
        cr::creativeHotbarAssetId(held));
    if (parentId != cr::kInvalidObjectId) {
      request.parentId = parentId;
    }
    requests.push_back(std::move(request));
    visitedKeys.push_back(candidate.visitedKey);
  }
  if (requests.empty()) {
    rejectScatter(editor, held.objectKind);
    return;
  }
  if (!ensureScatterTransaction(appState, stroke,
                                cr::CreativeWorldGestureKind::Place)) {
    rejectScatter(editor, held.objectKind);
    return;
  }
  const cr::CreativeFacadeDocumentBatchCreateReceipt receipt =
      appState.facade.createDocumentObjectsAtomically(requests);
  if (!receipt.accepted || !receipt.changed ||
      receipt.appliedCreateCount != requests.size()) {
    rejectScatter(editor, held.objectKind);
    return;
  }
  for (const std::uint64_t key : visitedKeys) {
    static_cast<void>(cr::rememberCreativeWorldGestureKey(stroke.visited,
                                                          key));
  }
  editor.placedCount += requests.size();
  stroke.acceptedMutationCount = static_cast<std::uint16_t>(
      stroke.acceptedMutationCount + requests.size());
  stroke.capacityReached = stroke.visited.count >= stroke.visited.keys.size();
  const auto objects = appState.facade.document().objects();
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Placed,
      editor.frameIndex, held.objectKind,
      objects.empty() ? cr::kInvalidObjectId : objects.back().id);
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
    const cr::CreativeHotbarEntry& held) noexcept {
  CreativeEditorAssetScatterPlan plan;
  if (!creativeEditorUsesAssetScatter(held, editor.toolSettings) ||
      !editor.interaction.target.grid.valid ||
      !std::isfinite(editor.placeCellSize) || editor.placeCellSize <= 0.0) {
    return plan;
  }
  plan.requested = true;
  const double radiusMeters =
      static_cast<double>(cr::creativeAssetScatterRadiusCells(
          editor.toolSettings.assetScatterRadius)) *
      editor.placeCellSize;
  const double spacingMeters =
      static_cast<double>(cr::creativeAssetScatterSpacingCells(
          editor.toolSettings.assetScatterSpacing)) *
      editor.placeCellSize;
  cr::CreativeAssetScatterRequest request;
  request.center = editor.interaction.target.grid.placementAnchor;
  request.radiusMeters = radiusMeters;
  request.spacingMeters = spacingMeters;
  request.densityFraction = cr::creativeAssetScatterDensityFraction(
      editor.toolSettings.assetScatterDensity);
  request.yaw = editor.toolSettings.assetScatterYaw;
  request.scaleVariation = cr::creativeAssetScatterScaleVariation(
      editor.toolSettings.assetScatterScale);
  request.seed = scatterSeed(held, request.center, editor.placeCellSize);
  const cr::CreativeAssetScatterPlan kernel =
      cr::planCreativeAssetScatter(request);
  plan.kernelStatus = kernel.status;
  plan.truncated = kernel.truncated;
  if (!kernel.accepted) {
    return plan;
  }
  const double maximumSlope = cr::creativeAssetScatterMaximumSlopeRadians(
      editor.toolSettings.assetScatterSlope);
  for (const cr::CreativeAssetScatterCandidate& kernelCandidate :
       kernel.items()) {
    CreativeEditorAssetScatterCandidate candidate = buildEditorCandidate(
        document, editor, held, kernelCandidate, spacingMeters, maximumSlope);
    if (candidate.placeable) {
      ++plan.placeableCount;
    }
    plan.candidates[plan.candidateCount++] = std::move(candidate);
  }
  plan.accepted = plan.candidateCount > 0U;
  return plan;
}

void processCreativeAssetScatterFrame(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (!creativeEditorUsesAssetScatter(held, editor.toolSettings)) {
    finalizeCreativeAssetScatterStroke(
        appState, editor, "creative_asset_scatter_non_scatter_tool");
    return;
  }
  CreativeAssetScatterStrokeState& stroke = editor.interaction.assetScatter;
  stroke.preview = buildCreativeEditorAssetScatterPlan(
      appState.facade.document(), editor, held);
  const cr::CreativeWorldGestureRepeatRequest repeatRequest =
      cr::makeCreativeWorldStrokeRepeatRequest(actions,
                                               monotonicTimeNanoseconds);
  const cr::CreativeWorldGestureRepeatResult repeat =
      cr::stepCreativeWorldGestureRepeat(stroke.repeat, repeatRequest);
  stroke.repeat = repeat.next;
  if (repeat.finalized) {
    finalizeCreativeAssetScatterStroke(
        appState, editor, "creative_asset_scatter_released");
    editor.interaction.assetScatter.preview =
        buildCreativeEditorAssetScatterPlan(appState.facade.document(),
                                            editor, held);
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
  stroke.preview = buildCreativeEditorAssetScatterPlan(
      appState.facade.document(), editor, held);
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
  return wireLines.size() - before;
}

}  // namespace iggy3d_creative_app
