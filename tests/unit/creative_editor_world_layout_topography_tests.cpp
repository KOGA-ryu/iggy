#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutPanel.hpp"
#include "EditorWorldLayoutTopography.hpp"
#include "EditorDesktopCommands.hpp"
#include "EditorEdits.hpp"
#include "EditorState.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double actual, double expected, double epsilon = 1.0e-6) {
  return std::fabs(actual - expected) <= epsilon;
}

cr::CreativeDocument topographyDocument(
    cr::CreativeDocumentId id,
    std::span<const std::uint16_t> heights) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Topography");
  static_cast<void>(document.assignId(id));
  static_cast<void>(document.setGridSettings(
      {{10.0, 2.0, -4.0}, 2.0, {32, 32, 32}}));
  static_cast<void>(document.replaceTerrainHeightField(
      {{-1, -1}, 3U, 3U}, heights));
  return document;
}

bool planIsBoundedCanonicalAndCached() {
  constexpr std::array<std::uint16_t, 9U> heights{
      1U, 2U, 3U,
      2U, 4U, 6U,
      3U, 6U, 9U,
  };
  cr::CreativeDocument document = topographyDocument(2101U, heights);
  app::CreativeEditorWorldLayoutTopographyState state;

  const bool first =
      app::refreshCreativeEditorWorldLayoutTopography(state, document);
  bool idleReused = true;
  for (std::uint32_t frame = 0U; frame < 300U; ++frame) {
    idleReused =
        !app::refreshCreativeEditorWorldLayoutTopography(state, document) &&
        idleReused;
  }
  state.intervalCells = 1U;
  const bool settingsRebuilt =
      app::refreshCreativeEditorWorldLayoutTopography(state, document);
  const bool settingsReused =
      app::refreshCreativeEditorWorldLayoutTopography(state, document);

  return expect(first && idleReused && settingsRebuilt && !settingsReused &&
                    state.buildCount == 2U,
                "first access and settings changes rebuild exactly once") &&
         expect(state.plan.accepted &&
                    state.plan.status ==
                        app::CreativeEditorWorldLayoutTopographyStatus::Ready &&
                    state.plan.columns.size() == heights.size(),
                "topography retains the bounded canonical terrain cells") &&
         expect(state.plan.minimumHeightCells == 1U &&
                    state.plan.maximumHeightCells == 9U,
                "topography reports the exact elevation range") &&
         expect(state.plan.contours.accepted &&
                    !state.plan.contours.segments.empty(),
                "topography consumes the shared contour kernel");
}

bool samplesHeightAndLocalSlope() {
  constexpr std::array<std::uint16_t, 9U> heights{
      1U, 2U, 3U,
      2U, 4U, 6U,
      3U, 6U, 9U,
  };
  const cr::CreativeDocument document = topographyDocument(2102U, heights);
  const app::CreativeEditorWorldLayoutTopographyPlan plan =
      app::buildCreativeEditorWorldLayoutTopography(document, 1U, 2U);
  const app::CreativeEditorWorldLayoutTopographySample center =
      app::sampleCreativeEditorWorldLayoutTopography(plan, 0.25, 0.75);
  const app::CreativeEditorWorldLayoutTopographySample edge =
      app::sampleCreativeEditorWorldLayoutTopography(plan, -0.25, -0.25);
  const app::CreativeEditorWorldLayoutTopographySample hole =
      app::sampleCreativeEditorWorldLayoutTopography(plan, 20.0, 20.0);
  const app::CreativeEditorWorldLayoutTopographySample invalid =
      app::sampleCreativeEditorWorldLayoutTopography(
          plan, std::numeric_limits<double>::quiet_NaN(), 0.0);

  return expect(center.present &&
                    center.coord == cr::CreativeTerrainCoord2{0, 0} &&
                    center.heightCells == 4U,
                "hover coordinates floor into the exact terrain cell") &&
         expect(near(center.slopeXCellsPerCell, 2.0) &&
                    near(center.slopeZCellsPerCell, 2.0) &&
                    near(center.slopeMagnitude, std::sqrt(8.0)) &&
                    center.neighborSampleCount == 4U,
                "interior slope uses centered differences on both axes") &&
         expect(edge.present && edge.heightCells == 1U &&
                    near(edge.slopeXCellsPerCell, 1.0) &&
                    near(edge.slopeZCellsPerCell, 1.0) &&
                    edge.neighborSampleCount == 2U,
                "edge slope falls back to one-sided samples") &&
         expect(!hole.present && !invalid.present,
                "holes and non-finite points produce no hover facts");
}

bool renderDocumentSwitchesCommittedAndPreviewTerrain() {
  constexpr std::array<std::uint16_t, 9U> liveHeights{
      1U, 1U, 1U,
      1U, 1U, 1U,
      1U, 1U, 1U,
  };
  constexpr std::array<std::uint16_t, 9U> previewHeights{
      7U, 7U, 7U,
      7U, 7U, 7U,
      7U, 7U, 7U,
  };
  const cr::CreativeDocument live = topographyDocument(2103U, liveHeights);
  app::CreativeEditorWorldLayoutState layout;
  app::resetCreativeEditorWorldLayout(layout);
  layout.preview.document = topographyDocument(2103U, previewHeights);
  layout.preview.accepted = true;
  layout.previewVisible = true;
  layout.previewLayoutRevision = layout.revision;

  app::CreativeEditorWorldLayoutTopographyState state;
  const cr::CreativeDocument& previewDocument =
      app::creativeEditorWorldLayoutRenderDocument(layout, live);
  const bool previewBuilt =
      app::refreshCreativeEditorWorldLayoutTopography(
          state, previewDocument, true, layout.previewLayoutRevision);
  const auto previewSample = app::sampleCreativeEditorWorldLayoutTopography(
      state.plan, 0.0, 0.0);

  layout.previewVisible = false;
  const cr::CreativeDocument& liveDocument =
      app::creativeEditorWorldLayoutRenderDocument(layout, live);
  const bool liveBuilt =
      app::refreshCreativeEditorWorldLayoutTopography(state, liveDocument);
  const auto liveSample = app::sampleCreativeEditorWorldLayoutTopography(
      state.plan, 0.0, 0.0);

  state.visible = false;
  const bool hiddenBuild =
      app::refreshCreativeEditorWorldLayoutTopography(
          state, previewDocument, true, layout.previewLayoutRevision);

  return expect(&previewDocument == &layout.preview.document && previewBuilt &&
                    previewSample.present && previewSample.heightCells == 7U,
                "active exact preview supplies the plan-view terrain") &&
         expect(&liveDocument == &live && liveBuilt && liveSample.present &&
                    liveSample.heightCells == 1U && state.buildCount == 2U,
                "committed mode returns to live terrain and rebuilds once") &&
         expect(!hiddenBuild && state.buildCount == 2U,
                "hidden topography performs no composition work");
}

bool invalidRequestsFailClosed() {
  cr::CreativeDocument invalid;
  const auto invalidDocument =
      app::buildCreativeEditorWorldLayoutTopography(invalid);
  constexpr std::array<std::uint16_t, 9U> heights{
      1U, 1U, 1U,
      1U, 1U, 1U,
      1U, 1U, 1U,
  };
  const cr::CreativeDocument document = topographyDocument(2104U, heights);
  const auto invalidInterval =
      app::buildCreativeEditorWorldLayoutTopography(document, 0U, 5U);
  const auto invalidMajor =
      app::buildCreativeEditorWorldLayoutTopography(document, 1U, 0U);
  return expect(
      !invalidDocument.accepted &&
          invalidDocument.status ==
              app::CreativeEditorWorldLayoutTopographyStatus::InvalidDocument,
      "invalid documents are rejected") &&
         expect(!invalidInterval.accepted && !invalidMajor.accepted &&
                    invalidInterval.status ==
                        app::CreativeEditorWorldLayoutTopographyStatus::
                            InvalidRequest &&
                    invalidMajor.status ==
                        app::CreativeEditorWorldLayoutTopographyStatus::
                            InvalidRequest,
                "invalid contour cadence is rejected");
}

bool capacityLimitsRemainAtomicAndUseful() {
  cr::CreativeDocument oversized =
      cr::CreativeDocument::create("Oversized topography");
  static_cast<void>(oversized.assignId(2105U));
  std::vector<cr::CreativeTerrainControlEdit> controls;
  controls.reserve(11U);
  for (std::int32_t index = 0; index < 11; ++index) {
    controls.push_back(
        {cr::CreativeTerrainEditKind::Upsert,
         {{index * 40, 0}, 8U, cr::kCreativeTerrainMaximumRadiusCells}});
  }
  const cr::CreativeTerrainMutationReceipt applied =
      oversized.applyTerrainControlEdits(controls);
  const auto rejected =
      app::buildCreativeEditorWorldLayoutTopography(oversized);

  cr::CreativeDocument dense =
      cr::CreativeDocument::create("Dense contours");
  static_cast<void>(dense.assignId(2106U));
  constexpr std::uint16_t side = 65U;
  std::vector<std::uint16_t> checkerboard(
      static_cast<std::size_t>(side) * side);
  for (std::uint16_t z = 0U; z < side; ++z) {
    for (std::uint16_t x = 0U; x < side; ++x) {
      checkerboard[static_cast<std::size_t>(z) * side + x] =
          (x + z) % 2U == 0U ? 1U : 64U;
    }
  }
  static_cast<void>(dense.replaceTerrainHeightField(
      {{0, 0}, side, side}, checkerboard));
  const auto degraded =
      app::buildCreativeEditorWorldLayoutTopography(dense, 1U, 5U);

  return expect(
             applied.accepted &&
                 rejected.status ==
                     app::CreativeEditorWorldLayoutTopographyStatus::
                         CapacityExceeded &&
                 !rejected.accepted && rejected.columns.empty(),
             "cell capacity rejects atomically before retaining draw data") &&
         expect(degraded.accepted &&
                    degraded.status ==
                        app::CreativeEditorWorldLayoutTopographyStatus::Ready &&
                    degraded.columns.size() == checkerboard.size() &&
                    !degraded.contours.accepted &&
                    degraded.contours.status ==
                        cr::CreativeTerrainContourPlanStatus::CapacityExceeded,
                "contour overflow retains bounded elevation bands");
}

bool regionSelectionBuildsSharedOperationRecipes() {
  app::CreativeEditorWorldLayoutTerrainRegionState state;
  state.editingEnabled = true;
  const bool began = app::beginCreativeEditorWorldLayoutTerrainRegion(
      state, 2.8, -1.2);
  const bool updated = app::updateCreativeEditorWorldLayoutTerrainRegion(
      state, -0.1, 2.9);
  const bool finished = app::finishCreativeEditorWorldLayoutTerrainRegion(
      state, -0.1, 2.9);

  constexpr std::array expectedModes{
      cr::CreativeTerrainCompositionMode::Replace,
      cr::CreativeTerrainCompositionMode::Raise,
      cr::CreativeTerrainCompositionMode::Lower,
      cr::CreativeTerrainCompositionMode::Smooth,
      cr::CreativeTerrainCompositionMode::Replace,
  };
  bool mappingsMatch = true;
  for (std::size_t index = 0U; index < expectedModes.size(); ++index) {
    state.operation =
        static_cast<app::CreativeEditorWorldLayoutTerrainRegionOperation>(
            index);
    const auto plan =
        app::planCreativeEditorWorldLayoutTerrainRegion(state);
    mappingsMatch = mappingsMatch && plan.accepted &&
                    plan.generation.bounds == state.bounds &&
                    plan.composition.mode == expectedModes[index] &&
                    plan.generation.reliefCells ==
                        (state.operation ==
                                 app::CreativeEditorWorldLayoutTerrainRegionOperation::
                                     Noise
                             ? state.noiseReliefCells
                             : 0U);
  }

  app::CreativeEditorWorldLayoutTerrainRegionState oversized;
  oversized.editingEnabled = true;
  const bool oversizedBegan =
      app::beginCreativeEditorWorldLayoutTerrainRegion(oversized, 0.0, 0.0);
  const bool oversizedUpdated =
      app::updateCreativeEditorWorldLayoutTerrainRegion(
          oversized, 100.0, 100.0);
  const bool nonFinite =
      app::beginCreativeEditorWorldLayoutTerrainRegion(
          oversized, std::numeric_limits<double>::quiet_NaN(), 0.0);

  return expect(began && updated && finished &&
                    state.bounds ==
                        cr::CreativeTerrainHeightFieldBounds{{-1, -2}, 4U,
                                                             5U},
                "drag selection floors cells and includes both endpoints") &&
         expect(mappingsMatch,
                "region modes map to the shared durable composition recipes") &&
         expect(oversizedBegan && !oversizedUpdated &&
                    !oversized.regionValid && !nonFinite,
                "capacity and non-finite selection fail closed");
}

bool regionPreviewAndApplyAreExactAtomicAndUndoable() {
  constexpr std::array<std::uint16_t, 9U> heights{
      4U, 4U, 4U,
      4U, 4U, 4U,
      4U, 4U, 4U,
  };
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(
      topographyDocument(2107U, heights)));
  app::CreativeEditorState editor;
  app::CreativeEditorWorldLayoutTerrainRegionState& region =
      editor.worldLayoutTopography.region;
  region.editingEnabled = true;
  region.targetHeightCells = 9U;
  region.featherCells = 0U;
  static_cast<void>(app::beginCreativeEditorWorldLayoutTerrainRegion(
      region, -0.8, -0.8));
  static_cast<void>(app::finishCreativeEditorWorldLayoutTerrainRegion(
      region, 0.8, 0.8));
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();

  app::CreativeDesktopCommandFrame previewFrame;
  previewFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  const app::CreativeDesktopCommandResult preview =
      app::dispatchCreativeDesktopCommands(
          previewFrame, {appState, editor, {}, nullptr, nullptr, nullptr});
  const bool previewOwned = region.ownsPreview;
  const cr::CreativeTerrainHeightField& candidate =
      editor.terrainGeneration.operationPreview.heightField;
  app::CreativeEditorWorldLayoutTopographyState previewCache;
  const std::uint64_t candidateHash =
      editor.terrainGeneration.operationPreview.receipt.replay.heightHash;
  const bool candidateCacheBuilt =
      app::refreshCreativeEditorWorldLayoutTopography(
          previewCache, appState.facade.document(), true, candidateHash,
          &candidate);
  const bool candidateCacheReused =
      !app::refreshCreativeEditorWorldLayoutTopography(
          previewCache, appState.facade.document(), true, candidateHash,
          &candidate);
  const auto previewPlan = app::buildCreativeEditorWorldLayoutTopography(
      appState.facade.document(), 1U, 2U, &candidate);
  const auto changedSample = app::sampleCreativeEditorWorldLayoutTopography(
      previewPlan, -0.5, -0.5);
  const auto preservedSample = app::sampleCreativeEditorWorldLayoutTopography(
      previewPlan, 1.5, 1.5);

  app::CreativeDesktopCommandFrame applyFrame;
  applyFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionApply);
  const app::CreativeDesktopCommandResult applied =
      app::dispatchCreativeDesktopCommands(
          applyFrame, {appState, editor, {}, nullptr, nullptr, nullptr});
  const std::size_t operationCountAfterApply =
      appState.facade.document().terrainOperationStack().operations.size();
  const std::uint64_t undoDepthAfterApply =
      cr::creativeUndoDepth(appState.history);
  const bool undone =
      app::undoLastEdit(appState, "world_layout_terrain_region_undo");
  region.editingEnabled = true;
  static_cast<void>(app::beginCreativeEditorWorldLayoutTerrainRegion(
      region, -0.8, -0.8));
  static_cast<void>(app::finishCreativeEditorWorldLayoutTerrainRegion(
      region, 0.8, 0.8));
  app::CreativeDesktopCommandFrame canceledPreviewFrame;
  canceledPreviewFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  const app::CreativeDesktopCommandResult canceledPreview =
      app::dispatchCreativeDesktopCommands(
          canceledPreviewFrame,
          {appState, editor, {}, nullptr, nullptr, nullptr});
  const std::uint64_t revisionBeforeCancel =
      appState.facade.document().revision();
  app::CreativeDesktopCommandFrame cancelFrame;
  cancelFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
  const app::CreativeDesktopCommandResult canceled =
      app::dispatchCreativeDesktopCommands(
          cancelFrame, {appState, editor, {}, nullptr, nullptr, nullptr});

  return expect(preview.accepted && preview.changed && previewOwned &&
                    appState.facade.document().revision() == revisionBefore,
                "preview computes exact candidate without document mutation") &&
         expect(changedSample.present && changedSample.heightCells == 9U &&
                    preservedSample.present &&
                    preservedSample.heightCells == 4U &&
                    candidateCacheBuilt && candidateCacheReused &&
                    previewCache.buildCount == 1U,
                "2D topography consumes the same bounded candidate as 3D") &&
         expect(applied.accepted && applied.changed && applied.sceneChanged &&
                    operationCountAfterApply == 1U &&
                    undoDepthAfterApply == 1U && undone &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "apply records one durable operation and one undo entry") &&
         expect(appState.facade.document().terrainOperationStack()
                        .operations.empty() &&
                    appState.facade.document().terrainHeightField().heightAt(
                        {-1, -1}) == 4U,
                "undo restores pre-region terrain truth") &&
         expect(canceledPreview.accepted && canceled.accepted &&
                    canceled.changed &&
                    appState.facade.document().revision() ==
                        revisionBeforeCancel &&
                    !editor.terrainGeneration.previewActive &&
                    !region.ownsPreview,
                "cancel clears transient region truth without mutation");
}

// The desktop terrain workflow presents the region lifecycle through a pure
// phase classifier and read-only metrics; this walks the real dispatcher
// workflow through every presented state.
bool regionPhaseAndMetricsMirrorTheWorkflow() {
  using Phase = app::CreativeEditorWorldLayoutTerrainRegionPhase;
  constexpr std::array<std::uint16_t, 9U> heights{
      4U, 4U, 4U,
      4U, 4U, 4U,
      4U, 4U, 4U,
  };
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(
      topographyDocument(2108U, heights)));
  app::CreativeEditorState editor;
  app::CreativeEditorWorldLayoutTerrainRegionState& region =
      editor.worldLayoutTopography.region;

  const bool labelsExact =
      app::toString(Phase::Idle) == "idle" &&
      app::toString(Phase::Selecting) == "selecting" &&
      app::toString(Phase::AwaitingPreview) == "awaiting preview" &&
      app::toString(Phase::Ready) == "ready" &&
      app::toString(Phase::Rejected) == "rejected" &&
      app::toString(Phase::Stale) == "stale";

  const Phase disabledPhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);
  region.editingEnabled = true;
  const Phase enabledIdlePhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);
  const app::CreativeEditorWorldLayoutTerrainRegionMetrics idleMetrics =
      app::measureCreativeEditorWorldLayoutTerrainRegion(region);

  static_cast<void>(app::beginCreativeEditorWorldLayoutTerrainRegion(
      region, -0.8, -0.8));
  const Phase selectingPhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);
  static_cast<void>(app::finishCreativeEditorWorldLayoutTerrainRegion(
      region, 0.8, 0.8));
  const Phase draftPhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);
  const app::CreativeEditorWorldLayoutTerrainRegionMetrics draftMetrics =
      app::measureCreativeEditorWorldLayoutTerrainRegion(region);

  app::CreativeDesktopCommandFrame previewFrame;
  previewFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  const app::CreativeDesktopCommandResult preview =
      app::dispatchCreativeDesktopCommands(
          previewFrame, {appState, editor, {}, nullptr, nullptr, nullptr});
  const Phase readyPhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);

  // A different document invalidates the owned preview: the workflow
  // presents the loss as the stale state with the terrain state's exact
  // message.
  const cr::CreativeDocument otherDocument =
      topographyDocument(2109U, heights);
  const bool staleSynchronized =
      app::synchronizeCreativeEditorWorldLayoutTerrainRegion(
          region, editor.terrainGeneration, otherDocument);
  const Phase stalePhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);
  const bool staleMessageExact =
      region.statusMessage ==
      "Terrain region preview canceled: document changed";

  // An invalid draft parameter turns the next exact preview into the
  // rejected state.
  static_cast<void>(app::beginCreativeEditorWorldLayoutTerrainRegion(
      region, -0.8, -0.8));
  static_cast<void>(app::finishCreativeEditorWorldLayoutTerrainRegion(
      region, 0.8, 0.8));
  region.noiseScaleCells = std::numeric_limits<double>::infinity();
  app::CreativeDesktopCommandFrame rejectedFrame;
  rejectedFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  const app::CreativeDesktopCommandResult rejected =
      app::dispatchCreativeDesktopCommands(
          rejectedFrame, {appState, editor, {}, nullptr, nullptr, nullptr});
  const Phase rejectedPhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);

  region.noiseScaleCells = 12.0;
  app::CreativeDesktopCommandFrame cancelFrame;
  cancelFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
  const app::CreativeDesktopCommandResult canceled =
      app::dispatchCreativeDesktopCommands(
          cancelFrame, {appState, editor, {}, nullptr, nullptr, nullptr});
  const Phase canceledPhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);

  return expect(labelsExact,
                "phase labels present the workflow vocabulary exactly") &&
         expect(disabledPhase == Phase::Idle &&
                    enabledIdlePhase == Phase::Idle && !idleMetrics.present,
                "the workflow rests idle with no region metrics") &&
         expect(selectingPhase == Phase::Selecting &&
                    draftPhase == Phase::AwaitingPreview,
                "selection presents selecting then awaiting preview") &&
         expect(draftMetrics.present && draftMetrics.minimumX == -1 &&
                    draftMetrics.minimumZ == -1 &&
                    draftMetrics.widthCells == 2U &&
                    draftMetrics.depthCells == 2U &&
                    draftMetrics.candidateCellCount == 4U,
                "metrics report bounds width depth and candidate cells") &&
         expect(preview.accepted && readyPhase == Phase::Ready,
                "an owned exact preview presents ready") &&
         expect(staleSynchronized && stalePhase == Phase::Stale &&
                    staleMessageExact,
                "a document change presents the stale state exactly") &&
         expect(!rejected.accepted && rejectedPhase == Phase::Rejected,
                "an invalid draft presents the rejected state") &&
         expect(canceled.accepted && canceledPhase == Phase::Idle,
                "cancel returns the workflow to idle");
}

}  // namespace

int main() {
  bool ok = true;
  ok = planIsBoundedCanonicalAndCached() && ok;
  ok = samplesHeightAndLocalSlope() && ok;
  ok = renderDocumentSwitchesCommittedAndPreviewTerrain() && ok;
  ok = invalidRequestsFailClosed() && ok;
  ok = capacityLimitsRemainAtomicAndUseful() && ok;
  ok = regionSelectionBuildsSharedOperationRecipes() && ok;
  ok = regionPreviewAndApplyAreExactAtomicAndUndoable() && ok;
  ok = regionPhaseAndMetricsMirrorTheWorkflow() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
