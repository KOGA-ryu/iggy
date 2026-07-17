#include "EditorAssetScatter.hpp"
#include "EditorInteraction.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"
#include "app/iggy3d/creative/tools/AssetScatter.hpp"
#include "core/math/Mat4.hpp"

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

[[nodiscard]] bool sameCandidate(
    const cr::CreativeAssetScatterCandidate& lhs,
    const cr::CreativeAssetScatterCandidate& rhs) noexcept {
  return cr::creativeVec3ExactlyEqual(lhs.position, rhs.position) &&
         lhs.yawOffsetRadians == rhs.yawOffsetRadians &&
         lhs.uniformScale == rhs.uniformScale;
}

bool plannerIsDeterministicBoundedAndSpaced() {
  cr::CreativeAssetScatterRequest request;
  request.center = {10.0, 2.0, -5.0};
  request.radiusMeters = 8.0;
  request.spacingMeters = 1.0;
  request.densityFraction = 1.0;
  request.yaw = cr::CreativeAssetScatterYaw::Full;
  request.scaleVariation = 0.25;
  request.seed = 42U;
  const cr::CreativeAssetScatterPlan first =
      cr::planCreativeAssetScatter(request);
  const cr::CreativeAssetScatterPlan second =
      cr::planCreativeAssetScatter(request);
  bool same = first.candidateCount == second.candidateCount;
  for (std::size_t index = 0U; same && index < first.candidateCount; ++index) {
    same = sameCandidate(first.candidates[index], second.candidates[index]);
  }

  bool validCandidates = first.accepted && first.candidateCount > 1U &&
                         first.candidateCount <=
                             cr::kCreativeAssetScatterCandidateCapacity;
  for (std::size_t index = 0U;
       validCandidates && index < first.candidateCount; ++index) {
    const cr::CreativeAssetScatterCandidate& candidate =
        first.candidates[index];
    const double dx = candidate.position.x - request.center.x;
    const double dz = candidate.position.z - request.center.z;
    validCandidates = cr::isFiniteCreativeVec3(candidate.position) &&
                      dx * dx + dz * dz <= 64.0 + 1.0e-9 &&
                      candidate.yawOffsetRadians >= 0.0 &&
                      candidate.yawOffsetRadians < std::numbers::pi * 2.0 &&
                      candidate.uniformScale >= 0.75 &&
                      candidate.uniformScale <= 1.25;
    for (std::size_t other = 0U;
         validCandidates && other < index; ++other) {
      const double sx =
          candidate.position.x - first.candidates[other].position.x;
      const double sz =
          candidate.position.z - first.candidates[other].position.z;
      validCandidates = sx * sx + sz * sz >= 1.0 - 1.0e-9;
    }
  }

  cr::CreativeAssetScatterRequest sparseRequest = request;
  sparseRequest.radiusMeters = 4.0;
  sparseRequest.densityFraction = 0.35;
  const cr::CreativeAssetScatterPlan sparse =
      cr::planCreativeAssetScatter(sparseRequest);
  sparseRequest.densityFraction = 1.0;
  const cr::CreativeAssetScatterPlan dense =
      cr::planCreativeAssetScatter(sparseRequest);
  sparseRequest.seed = 43U;
  const cr::CreativeAssetScatterPlan otherSeed =
      cr::planCreativeAssetScatter(sparseRequest);

  return expect(same, "same scatter seed produces byte-stable candidates") &&
         expect(validCandidates,
                "scatter candidates stay finite, bounded, spaced, and tuned") &&
         expect(dense.candidateCount >= sparse.candidateCount,
                "density monotonically increases candidate count") &&
         expect(otherSeed.candidateCount > 1U &&
                    !sameCandidate(dense.candidates[1],
                                   otherSeed.candidates[1]),
                "different seed changes deterministic layout");
}

bool plannerRejectsInvalidAndOversizedRequests() {
  cr::CreativeAssetScatterRequest invalid;
  invalid.center.x = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeAssetScatterPlan nonFinite =
      cr::planCreativeAssetScatter(invalid);
  invalid = {};
  invalid.radiusMeters = 64.0;
  invalid.spacingMeters = 1.0;
  const cr::CreativeAssetScatterPlan oversized =
      cr::planCreativeAssetScatter(invalid);
  invalid = {};
  invalid.maxCandidateCount =
      cr::kCreativeAssetScatterCandidateCapacity + 1U;
  const cr::CreativeAssetScatterPlan badCapacity =
      cr::planCreativeAssetScatter(invalid);
  return expect(nonFinite.status ==
                    cr::CreativeAssetScatterStatus::InvalidRequest,
                "non-finite scatter request rejected") &&
         expect(oversized.status ==
                    cr::CreativeAssetScatterStatus::CapacityExceeded,
                "oversized scatter grid rejected before iteration") &&
         expect(badCapacity.status ==
                    cr::CreativeAssetScatterStatus::InvalidRequest,
                "caller cannot exceed fixed output capacity");
}

bool terrainSurfacePoseMatchesFlatRenderedPatch() {
  cr::CreativeTerrainField field;
  const cr::CreativeTerrainControlEdit edit{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 4U}};
  const cr::CreativeTerrainMutationReceipt applied =
      field.apply(std::span{&edit, 1U});
  const cr::CreativeTerrainSurfacePose pose =
      cr::sampleCreativeTerrainSurfacePose(
          {&field, {0.5, 0.0, 0.5}, {}, 1.0});
  const cr::CreativeTerrainSurfacePose missing =
      cr::sampleCreativeTerrainSurfacePose(
          {&field, {100.5, 0.0, 100.5}, {}, 1.0});
  const cr::CreativeTerrainSurfacePose invalid =
      cr::sampleCreativeTerrainSurfacePose(
          {&field, {0.5, 0.0, 0.5}, {}, 0.0});
  return expect(applied.accepted && applied.changed,
                "terrain control installed") &&
         expect(pose.accepted && pose.present &&
                    pose.status == cr::CreativeTerrainSurfacePoseStatus::Ready &&
                    std::fabs(pose.position.y - 4.0) < 1.0e-12 &&
                    std::fabs(pose.normal.y - 1.0) < 1.0e-12 &&
                    std::fabs(pose.slopeRadians) < 1.0e-12,
                "surface pose matches flat four-corner terrain patch") &&
         expect(missing.accepted && !missing.present,
                "missing terrain is distinguished from invalid query") &&
         expect(!invalid.accepted &&
                    invalid.status ==
                        cr::CreativeTerrainSurfacePoseStatus::InvalidRequest,
                "invalid surface request fails closed");
}

[[nodiscard]] bool optionsContain(const cr::CreativeToolOptionList& options,
                                  cr::CreativeToolOptionId id) noexcept {
  for (const cr::CreativeToolOptionId option : options.items()) {
    if (option == id) {
      return true;
    }
  }
  return false;
}

app::CreativeEditorState scatterEditor() {
  app::CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  cr::CreativeHotbarEntry& held = editor.interaction.hotbar.entries[0];
  held = {cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Prop};
  static_cast<void>(cr::setCreativeHotbarAsset(
      held, "scatter_tree", {{-0.5, -0.5, -0.5}, {0.5, 1.5, 0.5}}));
  editor.toolSettings.assetPlacementMode =
      cr::CreativeAssetPlacementMode::Scatter;
  editor.toolSettings.assetScatterRadius =
      cr::CreativeAssetScatterRadius::FourCells;
  editor.toolSettings.assetScatterSpacing =
      cr::CreativeAssetScatterSpacing::TwoCells;
  editor.toolSettings.assetScatterDensity =
      cr::CreativeAssetScatterDensity::Dense;
  editor.toolSettings.assetScatterYaw = cr::CreativeAssetScatterYaw::Full;
  editor.placeCellSize = 1.0;
  editor.interaction.target.valid = true;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.faceNormal = {0.0, 1.0, 0.0};
  editor.interaction.target.grid.placerForward = {0.0, 0.0, -1.0};
  editor.interaction.target.grid.placementAnchor = {0.5, 0.0, 0.5};
  editor.interaction.target.grid.adjacentCellBounds =
      {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  return editor;
}

bool assetOnlyToolOptionsAreContextual() {
  cr::CreativeToolSettings settings;
  const cr::CreativeHotbarEntry ordinary{
      cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Prop};
  cr::CreativeHotbarEntry asset = ordinary;
  const bool assetSet = cr::setCreativeHotbarAsset(
      asset, "tree", {{-0.5, 0.0, -0.5}, {0.5, 2.0, 0.5}});
  const cr::CreativeToolOptionList ordinaryOptions =
      app::creativeEditorToolOptionsForEntry(ordinary, settings);
  const cr::CreativeToolOptionList singleOptions =
      app::creativeEditorToolOptionsForEntry(asset, settings);
  settings.assetPlacementMode = cr::CreativeAssetPlacementMode::Scatter;
  settings.placementAnchor = cr::CreativePlacementAnchor::Corner;
  const cr::CreativeToolOptionList scatterOptions =
      app::creativeEditorToolOptionsForEntry(asset, settings);
  cr::CreativeAppState appState;
  app::CreativeEditorState scatterState = scatterEditor();
  scatterState.toolSettings.placementAnchor =
      cr::CreativePlacementAnchor::Corner;
  const cr::CreativePlacementGridFrame scatterGrid =
      app::creativeEditorPlacementGridFrame(appState.facade.document(),
                                            scatterState);
  app::CreativeEditorState editor;
  editor.interaction.hotbar.entries[0] = ordinary;
  app::syncCreativeEditorQuickEdit(editor);
  editor.quickEdit.selectedIndex = 1U;
  editor.interaction.hotbar.entries[0] = asset;
  app::syncCreativeEditorQuickEdit(editor);

  return expect(assetSet, "test imported asset is valid") &&
         expect(!optionsContain(ordinaryOptions,
                                cr::CreativeToolOptionId::AssetPlacementMode),
                "ordinary materials do not expose asset controls") &&
         expect(optionsContain(singleOptions,
                               cr::CreativeToolOptionId::AssetPlacementMode) &&
                    optionsContain(singleOptions,
                                   cr::CreativeToolOptionId::PlacementYaw) &&
                    optionsContain(singleOptions,
                                   cr::CreativeToolOptionId::PlacementAnchor) &&
                    !optionsContain(singleOptions,
                                    cr::CreativeToolOptionId::AssetScatterRadius),
                "single asset mode keeps normal placement controls") &&
         expect(scatterOptions.count == 7U &&
                    optionsContain(scatterOptions,
                                   cr::CreativeToolOptionId::AssetScatterSlope) &&
                    !optionsContain(scatterOptions,
                                    cr::CreativeToolOptionId::PlacementYaw) &&
                    !optionsContain(scatterOptions,
                                    cr::CreativeToolOptionId::PlacementAnchor) &&
                    !scatterOptions.capacityExceeded,
                "scatter mode exposes seven bounded quick-edit settings") &&
         expect(scatterGrid.valid &&
                    scatterGrid.anchorKind ==
                        cr::CreativePlacementAnchorKind::BaseCenter,
                "scatter remains center-based when single placement retains another anchor mode") &&
         expect(editor.quickEdit.selectedIndex == 0U &&
                    editor.quickEdit.options.ids[0] ==
                        cr::CreativeToolOptionId::AssetPlacementMode,
                "asset identity change resets quick edit to mode row");
}

bool terrainScatterRejectsSteepAndMissingSurface() {
  cr::CreativeAppState appState;
  const std::array edits{
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 2U, 1U}},
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{1, 0}, 6U, 1U}},
  };
  const cr::CreativeTerrainMutationReceipt applied =
      appState.facade.applyTerrainControlEdits(edits);
  app::CreativeEditorState editor = scatterEditor();
  editor.interaction.target.terrainHit = true;
  editor.toolSettings.assetScatterSlope =
      cr::CreativeAssetScatterSlope::Degrees15;
  const cr::CreativeTerrainSurfacePose centerPose =
      cr::sampleCreativeTerrainSurfacePose(
          {&appState.facade.document().terrainField(), {0.5, 0.0, 0.5},
           appState.facade.document().gridSettings().origin,
           appState.facade.document().gridSettings().cellSizeMeters});
  editor.interaction.target.grid.placementAnchor = centerPose.position;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const app::CreativeEditorAssetScatterPlan plan =
      app::buildCreativeEditorAssetScatterPlan(
          appState.facade.document(), editor, held);
  bool rejectedSurface = false;
  for (const app::CreativeEditorAssetScatterCandidate& candidate :
       plan.items()) {
    rejectedSurface =
        rejectedSurface ||
        candidate.status ==
            app::CreativeEditorAssetScatterCandidateStatus::SlopeRejected ||
        candidate.status ==
            app::CreativeEditorAssetScatterCandidateStatus::MissingSurface;
  }
  return expect(applied.accepted && centerPose.present &&
                    centerPose.slopeRadians >
                        cr::creativeAssetScatterMaximumSlopeRadians(
                            cr::CreativeAssetScatterSlope::Degrees15),
                "test terrain contains a slope above the configured limit") &&
         expect(plan.accepted && rejectedSurface,
                "terrain scatter marks steep or absent candidates invalid");
}

bool previewIsTransientAndSolidGhostIsSuppressed() {
  cr::CreativeAppState appState;
  app::CreativeEditorState editor = scatterEditor();
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const app::CreativeEditorAssetScatterPlan plan =
      app::buildCreativeEditorAssetScatterPlan(
          appState.facade.document(), editor, held);
  editor.interaction.assetScatter.preview = plan;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t edges = app::appendCreativeEditorAssetScatterWireframes(
      editor, 0.05F, lines);
  bool grounded = true;
  for (const app::CreativeEditorAssetScatterCandidate& candidate :
       plan.items()) {
    const cr::CreativeTransformedBounds bounds =
        cr::resolveCreativeTransformedBounds(candidate.placement.authoredBounds,
                                             candidate.placement.transform);
    grounded = grounded && bounds.valid &&
               std::fabs(bounds.worldBounds.min.y -
                         candidate.surfacePosition.y) < 1.0e-9;
  }
  iggy3d::FrameInput frame;
  frame.camera.clipFromWorld = iggy3d::identityMat4();
  frame.camera.clipFromView = iggy3d::identityMat4();
  app::attachCreativeEditorPlacementPreviews(
      editor, false, frame, &appState.facade.document());

  return expect(plan.accepted && plan.placeableCount > 0U,
                "asset scatter preview plan is admitted") &&
         expect(edges == plan.candidateCount * 12U && lines.size() == edges,
                "every scatter candidate has one exact bounds wireframe") &&
         expect(grounded,
                "random scale preserves imported asset ground alignment") &&
         expect(frame.creativePreview.itemCount == 1U &&
                    frame.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::Held,
                "scatter keeps held asset and suppresses single target ghost") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    appState.facade.document().objectCount() == 0U,
                "scatter aiming is transient");
}

void setSecondary(cr::CreativeWorldActionFrame& actions,
                  bool down,
                  bool pressed,
                  bool released) {
  const std::size_t index =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Secondary);
  actions.down[index] = down;
  actions.pressed[index] = pressed;
  actions.released[index] = released;
}

bool gestureIsAtomicDeduplicatedAndOneUndoStep() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Scatter");
  static_cast<void>(document.assignId(9001U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "scatter document installed")) {
    return false;
  }
  app::CreativeEditorState editor = scatterEditor();
  cr::CreativeWorldActionFrame press;
  setSecondary(press, true, true, false);
  app::processCreativeAssetScatterFrame(appState, editor, press, 0U);
  const std::size_t placed = appState.facade.document().objectCount();
  const std::uint64_t revisionAfterPress =
      appState.facade.document().revision();

  cr::CreativeWorldActionFrame beforeRepeat;
  setSecondary(beforeRepeat, true, false, false);
  app::processCreativeAssetScatterFrame(
      appState, editor, beforeRepeat, 199'000'000ULL);
  const bool beforeStable =
      appState.facade.document().revision() == revisionAfterPress;
  app::processCreativeAssetScatterFrame(
      appState, editor, beforeRepeat, 200'000'000ULL);
  const bool stationaryStable =
      appState.facade.document().objectCount() == placed;

  cr::CreativeWorldActionFrame release;
  setSecondary(release, false, false, true);
  app::processCreativeAssetScatterFrame(
      appState, editor, release, 201'000'000ULL);
  const std::size_t undoDepthBefore = cr::creativeUndoDepth(appState.history);
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(placed > 1U && editor.placedCount == placed,
                "first press atomically places the scatter patch") &&
         expect(beforeStable,
                "repeat kernel does not mutate before 200 milliseconds") &&
         expect(stationaryStable,
                "stationary repeat cannot stack duplicate assets") &&
         expect(undoDepthBefore == 1U &&
                    undo.accepted && undo.objectCountAfter == 0U,
                "release records exactly one undo step for the gesture");
}

bool sharedVisitedKernelIsBounded() {
  cr::CreativeWorldGestureVisitedKeys visited;
  bool inserted = true;
  for (std::uint64_t key = 1U;
       key <= cr::kCreativeWorldGestureVisitedKeyCapacity; ++key) {
    inserted = inserted &&
               cr::rememberCreativeWorldGestureKey(visited, key) ==
                   cr::CreativeWorldGestureVisitStatus::Inserted;
  }
  const cr::CreativeWorldGestureVisitStatus duplicate =
      cr::rememberCreativeWorldGestureKey(visited, 1U);
  const cr::CreativeWorldGestureVisitStatus overflow =
      cr::rememberCreativeWorldGestureKey(visited, 999U);
  return expect(inserted && visited.count == visited.keys.size(),
                "visited kernel accepts its fixed capacity") &&
         expect(duplicate ==
                    cr::CreativeWorldGestureVisitStatus::AlreadyPresent &&
                    overflow ==
                        cr::CreativeWorldGestureVisitStatus::CapacityExceeded,
                "visited kernel distinguishes duplicate from capacity");
}

}  // namespace

int main() {
  const bool ok = plannerIsDeterministicBoundedAndSpaced() &&
                  plannerRejectsInvalidAndOversizedRequests() &&
                  terrainSurfacePoseMatchesFlatRenderedPatch() &&
                  assetOnlyToolOptionsAreContextual() &&
                  terrainScatterRejectsSteepAndMissingSurface() &&
                  previewIsTransientAndSolidGhostIsSuppressed() &&
                  gestureIsAtomicDeduplicatedAndOneUndoStep() &&
                  sharedVisitedKernelIsBounded();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
