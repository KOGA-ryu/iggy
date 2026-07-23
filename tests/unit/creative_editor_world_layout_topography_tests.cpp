#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutPanel.hpp"
#include "EditorWorldLayoutTopography.hpp"
#include "EditorDesktopCommands.hpp"
#include "EditorEdits.hpp"
#include "EditorState.hpp"
#include "EditorTerrain.hpp"

#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

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
                    state.plan.analysis.cells.size() == heights.size(),
                "topography retains the bounded canonical terrain cells") &&
         expect(state.plan.minimumHeightCells == 1U &&
                    state.plan.maximumHeightCells == 9U,
                "topography reports the exact elevation range") &&
         expect(state.plan.analysis.contours.accepted &&
                    !state.plan.analysis.contours.segments.empty(),
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

bool previewAnalysisReportsCutFillAndSlopeBands() {
  constexpr std::array<std::uint16_t, 9U> liveHeights{
      4U, 4U, 4U,
      4U, 4U, 4U,
      4U, 4U, 4U,
  };
  constexpr std::array<std::uint16_t, 9U> previewHeights{
      2U, 4U, 6U,
      2U, 4U, 6U,
      2U, 4U, 6U,
  };
  const cr::CreativeDocument document =
      topographyDocument(2114U, liveHeights);
  cr::CreativeTerrainHeightField candidate;
  const cr::CreativeTerrainHeightFieldReplaceReceipt replaced =
      candidate.replace({{-1, -1}, 3U, 3U}, previewHeights);
  const auto preview = app::buildCreativeEditorWorldLayoutTopography(
      document, 1U, 2U, &candidate);
  const auto live = app::buildCreativeEditorWorldLayoutTopography(
      document, 1U, 2U);
  const auto cut = app::sampleCreativeEditorWorldLayoutTopography(
      preview, -0.5, -0.5);
  const auto fill = app::sampleCreativeEditorWorldLayoutTopography(
      preview, 1.5, -0.5);

  return expect(replaced.accepted && preview.accepted &&
                    preview.analysis.hasReference &&
                    preview.analysis.cutCellCount == 3U &&
                    preview.analysis.fillCellCount == 3U,
                "preview topography compares exact candidate and live cells") &&
         expect(cut.present && cut.deltaCells == -2 &&
                    cut.cutFill == cr::CreativeTerrainCutFillKind::Cut &&
                    fill.present && fill.deltaCells == 2 &&
                    fill.cutFill == cr::CreativeTerrainCutFillKind::Fill,
                "hover facts expose signed cut and fill deltas") &&
         expect(cut.slopeBand == cr::CreativeTerrainSlopeBand::Extreme &&
                    cut.slopeDegrees > 63.4 && cut.slopeDegrees < 63.5,
                "plan slope bands use the shared analysis gradient") &&
         expect(live.accepted && !live.analysis.hasReference &&
                    live.analysis.cutCellCount == 0U &&
                    live.analysis.fillCellCount == 0U,
                "committed terrain never fabricates cut fill feedback");
}

bool contourAndHeightTargetsProduceSharedRegionRecipes() {
  constexpr std::array<std::uint16_t, 9U> slopeHeights{
      1U, 3U, 3U,
      1U, 3U, 3U,
      1U, 3U, 3U,
  };
  const auto slopePlan = app::buildCreativeEditorWorldLayoutTopography(
      topographyDocument(2115U, slopeHeights), 1U, 2U);
  const cr::CreativeTerrainContourSegment& segment =
      slopePlan.analysis.contours.segments.front();
  const double contourX = (segment.start.x + segment.end.x) * 0.5;
  const double contourZ = (segment.start.z + segment.end.z) * 0.5;
  const auto contour =
      app::planCreativeEditorWorldLayoutTerrainAnalysisEdit(
          slopePlan, contourX, contourZ, 0.05,
          cr::CreativeTerrainAnalysisHitMode::ContourOnly);

  constexpr std::array<std::uint16_t, 9U> flatHeights{
      7U, 7U, 7U,
      7U, 7U, 7U,
      7U, 7U, 7U,
  };
  const auto flatPlan = app::buildCreativeEditorWorldLayoutTopography(
      topographyDocument(2116U, flatHeights), 1U, 2U);
  const auto handle = app::planCreativeEditorWorldLayoutTerrainAnalysisEdit(
      flatPlan, 0.25, 0.25, 0.05,
      cr::CreativeTerrainAnalysisHitMode::HeightHandleOnly);
  const auto noFallback =
      app::planCreativeEditorWorldLayoutTerrainAnalysisEdit(
          flatPlan, 0.25, 0.25, 0.05,
          cr::CreativeTerrainAnalysisHitMode::ContourOnly);
  app::CreativeEditorWorldLayoutTerrainRegionState region;
  region.editingEnabled = true;
  region.editingOperationId = 77U;
  const bool selected =
      app::selectCreativeEditorWorldLayoutTerrainAnalysisEdit(region,
                                                               contour);
  const app::CreativeEditorWorldLayoutTerrainRegionState selectedRegion =
      region;
  const bool rejected =
      app::selectCreativeEditorWorldLayoutTerrainAnalysisEdit(region,
                                                               noFallback);

  return expect(contour.accepted &&
                    contour.hit.kind ==
                        cr::CreativeTerrainAnalysisHitKind::Contour &&
                    contour.recipe.mode ==
                        cr::CreativeTerrainRegionMode::Flatten &&
                    contour.recipe.targetHeightCells ==
                        segment.levelCells &&
                    contour.recipe.bounds.widthCells == 5U &&
                    contour.recipe.bounds.depthCells == 5U &&
                    contour.recipe.mask ==
                        cr::CreativeTerrainCompositionMask::Ellipse &&
                    contour.recipe.featherCells == 1U,
                "contour selection becomes a feathered flatten recipe") &&
         expect(handle.accepted &&
                    handle.hit.kind ==
                        cr::CreativeTerrainAnalysisHitKind::HeightHandle &&
                    handle.recipe.bounds ==
                        cr::CreativeTerrainHeightFieldBounds{{0, 0}, 1U, 1U} &&
                    handle.recipe.targetHeightCells == 7U &&
                    handle.recipe.featherCells == 0U,
                "height handle selection becomes an exact one-cell recipe") &&
         expect(!noFallback.accepted,
                "height handles remain explicit when no contour is nearby") &&
         expect(selected && selectedRegion.regionValid &&
                    !selectedRegion.selecting &&
                    selectedRegion.recipe == contour.recipe &&
                    selectedRegion.anchor == contour.recipe.bounds.minimum &&
                    selectedRegion.cursor ==
                        cr::CreativeTerrainCoord2{
                            contour.recipe.bounds.minimum.x + 4,
                            contour.recipe.bounds.minimum.z + 4} &&
                    selectedRegion.editingOperationId ==
                        cr::kInvalidCreativeTerrainOperationId &&
                    selectedRegion.statusMessage ==
                        "Contour region selected",
                "analysis selection becomes one new editable region draft") &&
         expect(!rejected && region.recipe == selectedRegion.recipe &&
                    region.statusMessage ==
                        "Terrain analysis target unavailable",
                "rejected analysis selection preserves the prior draft");
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
                 !rejected.accepted && rejected.analysis.cells.empty(),
             "cell capacity rejects atomically before retaining draw data") &&
         expect(degraded.accepted &&
                    degraded.status ==
                        app::CreativeEditorWorldLayoutTopographyStatus::Ready &&
                    degraded.analysis.cells.size() == checkerboard.size() &&
                    !degraded.analysis.contours.accepted &&
                    degraded.analysis.contours.status ==
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
      cr::CreativeTerrainRegionMode::Flatten,
      cr::CreativeTerrainRegionMode::Raise,
      cr::CreativeTerrainRegionMode::Lower,
      cr::CreativeTerrainRegionMode::Smooth,
      cr::CreativeTerrainRegionMode::Noise,
      cr::CreativeTerrainRegionMode::Erase,
  };
  bool mappingsMatch = true;
  for (std::size_t index = 0U; index < expectedModes.size(); ++index) {
    state.recipe.mode = expectedModes[index];
    const auto plan =
        app::planCreativeEditorWorldLayoutTerrainRegion(state);
    mappingsMatch = mappingsMatch && plan.accepted &&
                    plan.recipe == state.recipe &&
                    plan.recipe.mode == expectedModes[index] &&
                    plan.recipe.amountCells == state.recipe.amountCells &&
                    plan.recipe.noiseReliefCells ==
                        state.recipe.noiseReliefCells;
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
                    state.recipe.bounds ==
                        cr::CreativeTerrainHeightFieldBounds{{-1, -2}, 4U,
                                                             5U},
                "drag selection floors cells and includes both endpoints") &&
         expect(mappingsMatch,
                "region modes map to the shared durable composition recipes") &&
         expect(oversizedBegan && !oversizedUpdated &&
                    !oversized.regionValid && !nonFinite,
                "capacity and non-finite selection fail closed");
}

bool regionBoundsMoveAndResizeStayBounded() {
  using Handle = app::CreativeEditorWorldLayoutTerrainRegionHandle;
  app::CreativeEditorWorldLayoutTerrainRegionState state;
  state.editingEnabled = true;
  const bool selected =
      app::setCreativeEditorWorldLayoutTerrainRegionBounds(
          state, {{2, 3}, 4U, 5U});
  const bool handlesExact =
      app::hitCreativeEditorWorldLayoutTerrainRegionHandle(
          state, 2.0, 3.0, 0.2) == Handle::MinimumXMinimumZ &&
      app::hitCreativeEditorWorldLayoutTerrainRegionHandle(
          state, 6.0, 5.0, 0.2) == Handle::MaximumX &&
      app::hitCreativeEditorWorldLayoutTerrainRegionHandle(
          state, 4.0, 5.0, 0.2) == Handle::Body &&
      app::hitCreativeEditorWorldLayoutTerrainRegionHandle(
          state, 20.0, 20.0, 0.2) == Handle::None;

  const bool beganMove =
      app::beginCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, Handle::Body, 3.0, 4.0);
  const bool moved =
      app::updateCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, 5.0, 1.0);
  const bool finishedMove =
      app::finishCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, 5.0, 1.0);
  const bool moveExact =
      state.recipe.bounds ==
      cr::CreativeTerrainHeightFieldBounds{{4, 0}, 4U, 5U};

  const bool beganResize =
      app::beginCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, Handle::MinimumXMinimumZ, 4.0, 0.0);
  const bool resized =
      app::updateCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, 2.0, -1.0);
  const bool finishedResize =
      app::finishCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, 2.0, -1.0);
  const bool resizeExact =
      state.recipe.bounds ==
      cr::CreativeTerrainHeightFieldBounds{{2, -1}, 6U, 6U};

  const cr::CreativeTerrainHeightFieldBounds beforeRejected =
      state.recipe.bounds;
  const bool beganInvalid =
      app::beginCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, Handle::MaximumX, 8.0, 0.0);
  const bool rejectedCollapse =
      !app::updateCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, 1.0, 0.0);
  static_cast<void>(
      app::cancelCreativeEditorWorldLayoutTerrainRegionManipulation(state));

  const bool beganCanceled =
      app::beginCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, Handle::Body, 3.0, 0.0);
  const bool changedBeforeCancel =
      app::updateCreativeEditorWorldLayoutTerrainRegionManipulation(
          state, 8.0, 8.0);
  const bool canceled =
      app::cancelCreativeEditorWorldLayoutTerrainRegionManipulation(state);
  const bool invalidNumericRejected =
      !app::setCreativeEditorWorldLayoutTerrainRegionBounds(
          state, {{0, 0}, std::numeric_limits<std::uint16_t>::max(),
                  std::numeric_limits<std::uint16_t>::max()});

  return expect(selected && handlesExact,
                "region edges corners and body resolve to stable handles") &&
         expect(beganMove && moved && finishedMove && moveExact,
                "body movement preserves dimensions and applies cell deltas") &&
         expect(beganResize && resized && finishedResize && resizeExact,
                "corner resize keeps the opposite corner fixed") &&
         expect(beganInvalid && rejectedCollapse &&
                    state.recipe.bounds == beforeRejected,
                "crossed edges are rejected without corrupting bounds") &&
         expect(beganCanceled && changedBeforeCancel && canceled &&
                    state.recipe.bounds == beforeRejected,
                "cancel restores the exact pre-drag bounds") &&
         expect(invalidNumericRejected &&
                    state.recipe.bounds == beforeRejected,
                "numeric bounds reject capacity overflow atomically");
}

bool regionRecipeIsIdenticalAcross2dAnd3dFrontends() {
  app::CreativeEditorWorldLayoutTerrainRegionState drafting;
  drafting.editingEnabled = true;
  static_cast<void>(app::beginCreativeEditorWorldLayoutTerrainRegion(
      drafting, -0.8, -0.8));
  static_cast<void>(app::finishCreativeEditorWorldLayoutTerrainRegion(
      drafting, 1.8, 1.8));
  drafting.recipe.mode = cr::CreativeTerrainRegionMode::Noise;
  drafting.recipe.mask = cr::CreativeTerrainCompositionMask::Ellipse;
  drafting.recipe.amountCells = 3U;
  drafting.recipe.targetHeightCells = 7U;
  drafting.recipe.noiseReliefCells = 2U;
  drafting.recipe.noiseScaleCells = 5.5;
  drafting.recipe.featherCells = 1U;
  drafting.recipe.seed = 991U;
  const auto draftingPlan =
      app::planCreativeEditorWorldLayoutTerrainRegion(drafting);

  app::CreativeEditorState editor;
  editor.toolSettings.terrainRegionRecipe = drafting.recipe;
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::First,
      {-1, 0, -1}));
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::Second,
      {1, 0, 1}));
  cr::CreativeTerrainRegionRecipe viewportRecipe;
  const bool viewportBuilt = app::buildCreativeEditorTerrainRegionRecipe(
      editor, editor.volume.selection, viewportRecipe);

  constexpr std::array<std::uint16_t, 9U> heights{
      4U, 4U, 4U,
      4U, 4U, 4U,
      4U, 4U, 4U,
  };
  const cr::CreativeDocument document = topographyDocument(2111U, heights);
  const cr::CreativeTerrainOperationMutationPlan viewportPlan =
      app::planCreativeEditorTerrainRegion(document, editor,
                                           editor.volume.selection);
  cr::CreativeTerrainOperationMutationRequest draftingRequest;
  draftingRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  draftingRequest.operationKind = cr::CreativeTerrainOperationKind::Region;
  draftingRequest.region = draftingPlan.recipe;
  const cr::CreativeTerrainOperationMutationPlan directDraftingPlan =
      cr::planCreativeTerrainOperationMutation(
          document.terrainField(), document.terrainHeightField(),
          document.terrainMaterialField(), document.terrainOperationStack(),
          draftingRequest);

  return expect(draftingPlan.accepted && viewportBuilt &&
                    draftingPlan.recipe == viewportRecipe,
                "2D and 3D frontends produce one identical region recipe") &&
         expect(viewportPlan.receipt.accepted &&
                    directDraftingPlan.receipt.accepted &&
                    cr::creativeTerrainHeightFieldsEqual(
                        viewportPlan.heightField,
                        directDraftingPlan.heightField) &&
                    viewportPlan.receipt.replay.heightHash ==
                        directDraftingPlan.receipt.replay.heightHash,
                "both frontends produce the same exact terrain candidate");
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
  region.recipe.targetHeightCells = 9U;
  region.recipe.featherCells = 0U;
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
  const std::uint64_t revisionAfterPreview =
      appState.facade.document().revision();
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
  const bool regionOperationAfterApply =
      operationCountAfterApply == 1U &&
      appState.facade.document()
              .terrainOperationStack()
              .operations.front()
              .kind == cr::CreativeTerrainOperationKind::Region;
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
                    revisionAfterPreview == revisionBefore,
                "preview computes exact candidate without document mutation") &&
         expect(changedSample.present && changedSample.heightCells == 9U &&
                    preservedSample.present &&
                    preservedSample.heightCells == 4U &&
                    candidateCacheBuilt && candidateCacheReused &&
                    previewCache.buildCount == 1U,
                "2D topography consumes the same bounded candidate as 3D") &&
         expect(applied.accepted && applied.changed && applied.sceneChanged &&
                    operationCountAfterApply == 1U &&
                    regionOperationAfterApply &&
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

bool analysisEditPreviewAndCollisionStaySynchronized() {
  constexpr std::array<std::uint16_t, 9U> heights{
      2U, 6U, 6U,
      2U, 6U, 6U,
      2U, 6U, 6U,
  };
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(
      topographyDocument(2117U, heights)));
  const cr::CreativeDocument& documentBefore = appState.facade.document();
  const app::CreativeEditorWorldLayoutTopographyPlan analysis =
      app::buildCreativeEditorWorldLayoutTopography(documentBefore, 1U, 2U);
  const cr::CreativeTerrainContourSegment& segment =
      analysis.analysis.contours.segments.front();
  const double contourX = (segment.start.x + segment.end.x) * 0.5;
  const double contourZ = (segment.start.z + segment.end.z) * 0.5;
  const auto edit = app::planCreativeEditorWorldLayoutTerrainAnalysisEdit(
      analysis, contourX, contourZ, 0.05,
      cr::CreativeTerrainAnalysisHitMode::ContourOnly);

  app::CreativeEditorState editor;
  app::CreativeEditorWorldLayoutTerrainRegionState& region =
      editor.worldLayoutTopography.region;
  region.editingEnabled = true;
  const bool selected =
      app::selectCreativeEditorWorldLayoutTerrainAnalysisEdit(region, edit);
  const app::CreativeEditorTerrainGenerationPreviewReceipt preview =
      selected ? app::previewCreativeEditorWorldLayoutTerrainRegion(
                     region, editor.terrainGeneration, documentBefore)
               : app::CreativeEditorTerrainGenerationPreviewReceipt{};
  const cr::CreativeTerrainHeightField candidate =
      editor.terrainGeneration.operationPreview.heightField;

  cr::CreativeTerrainCoord2 changedCoord{};
  std::uint16_t changedHeightBefore = 0U;
  std::uint16_t changedHeightAfter = 0U;
  bool foundChanged = false;
  for (std::int32_t z = -1; z <= 1 && !foundChanged; ++z) {
    for (std::int32_t x = -1; x <= 1; ++x) {
      const cr::CreativeTerrainCoord2 coord{x, z};
      const auto candidateHeight = candidate.heightAt(coord);
      const auto sourceHeight =
          documentBefore.terrainHeightField().heightAt(coord);
      if (candidateHeight.has_value() && sourceHeight.has_value() &&
          *candidateHeight != *sourceHeight) {
        changedCoord = coord;
        changedHeightBefore = *sourceHeight;
        changedHeightAfter = *candidateHeight;
        foundChanged = true;
        break;
      }
    }
  }

  const cr::CreativeTerrainSurfacePlan baselineSurface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          documentBefore.terrainField(),
          documentBefore.terrainHeightField());
  const cr::CreativeTerrainSurfacePlan candidateSurface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          documentBefore.terrainField(), candidate);
  const cr::CreativeTerrainContourPlan expectedCandidateContours =
      cr::buildCreativeTerrainContourPlan(
          candidateSurface,
          {1U, 2U, cr::kCreativeTerrainContourSegmentCapacity});
  app::CreativeTerrainContourDisplayState contourDisplay;
  contourDisplay.visible = true;
  contourDisplay.intervalCells = 1U;
  contourDisplay.majorEvery = 2U;
  const bool previewContoursBuilt =
      app::refreshCreativeEditorTerrainContours(
          contourDisplay, documentBefore, &candidateSurface,
          editor.terrainGeneration.operationPreview.receipt.replay.heightHash);
  const cr::CreativeTerrainContourPlan previewContours = contourDisplay.plan;

  cr::CreativeRoomBakeRequest beforeBakeRequest;
  beforeBakeRequest.document = &documentBefore;
  beforeBakeRequest.roomId = "terrain_analysis_before";
  beforeBakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult beforeBake =
      cr::buildRoomAssetFromCreativeDocument(beforeBakeRequest);
  const iggy3d::SpatialSurfaceSet beforeSurfaces =
      iggy3d::buildSpatialSurfaceSet(beforeBake.room);
  const cr::CreativeGridSettings grid = documentBefore.gridSettings();
  const cr::CreativeTerrainRenderPlan baselineRender =
      cr::buildCreativeTerrainRenderPlan(
          baselineSurface, documentBefore.terrainMaterialField(), grid.origin,
          grid.cellSizeMeters);
  const cr::CreativeTerrainRenderPlan candidateRender =
      cr::buildCreativeTerrainRenderPlan(
          candidateSurface, documentBefore.terrainMaterialField(), grid.origin,
          grid.cellSizeMeters);
  const cr::CreativeTerrainSurfacePatch* baselinePatch = nullptr;
  const cr::CreativeTerrainSurfacePatch* candidatePatch = nullptr;
  for (const cr::CreativeTerrainSurfacePatch& patch : baselineRender.patches) {
    if (patch.coord == changedCoord) {
      baselinePatch = &patch;
      break;
    }
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : candidateRender.patches) {
    if (patch.coord == changedCoord) {
      candidatePatch = &patch;
      break;
    }
  }
  const float sampleX = static_cast<float>(
      grid.origin.x + (static_cast<double>(changedCoord.x) + 0.5) *
                          grid.cellSizeMeters);
  const float sampleZ = static_cast<float>(
      grid.origin.z + (static_cast<double>(changedCoord.z) + 0.5) *
                          grid.cellSizeMeters);
  const iggy3d::CollisionQueryResult beforeGround =
      iggy3d::sampleSurfaceHeightAtOrBelow(
          beforeSurfaces, {sampleX, 64.0F, sampleZ}, 64.0F, 0.2F);

  const app::CreativeEditorTerrainGenerationApplyReceipt applied =
      app::applyCreativeEditorWorldLayoutTerrainRegion(
          region, editor.terrainGeneration, appState);
  const cr::CreativeDocument& documentAfter = appState.facade.document();
  const bool appliedExact = cr::creativeTerrainHeightFieldsEqual(
      candidate, documentAfter.terrainHeightField());
  const bool appliedContoursBuilt =
      app::refreshCreativeEditorTerrainContours(contourDisplay,
                                                documentAfter);

  cr::CreativeRoomBakeRequest afterBakeRequest;
  afterBakeRequest.document = &documentAfter;
  afterBakeRequest.roomId = "terrain_analysis_after";
  afterBakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult afterBake =
      cr::buildRoomAssetFromCreativeDocument(afterBakeRequest);
  const iggy3d::SpatialSurfaceSet afterSurfaces =
      iggy3d::buildSpatialSurfaceSet(afterBake.room);
  const iggy3d::CollisionQueryResult afterGround =
      iggy3d::sampleSurfaceHeightAtOrBelow(
          afterSurfaces, {sampleX, 64.0F, sampleZ}, 64.0F, 0.2F);
  const bool collisionSynchronized =
      baselineRender.accepted && candidateRender.accepted &&
      baselinePatch != nullptr && candidatePatch != nullptr &&
      beforeBake.receipt.accepted && afterBake.receipt.accepted &&
      beforeGround.status == iggy3d::CollisionQueryStatus::Hit &&
      afterGround.status == iggy3d::CollisionQueryStatus::Hit &&
      beforeGround.role == iggy3d::CollisionSurfaceRole::Walkable &&
      afterGround.role == iggy3d::CollisionSurfaceRole::Walkable &&
      near(beforeGround.heightMeters, baselinePatch->center.y, 0.001) &&
      near(afterGround.heightMeters, candidatePatch->center.y, 0.001) &&
      near(afterGround.heightMeters - beforeGround.heightMeters,
           candidatePatch->center.y - baselinePatch->center.y, 0.001);
  if (!collisionSynchronized) {
    std::cerr << "terrain analysis collision sync: coord "
              << changedCoord.x << ',' << changedCoord.z << " heights "
              << changedHeightBefore << " -> " << changedHeightAfter
              << " ground " << beforeGround.heightMeters << " -> "
              << afterGround.heightMeters << " expected delta "
              << (candidatePatch == nullptr || baselinePatch == nullptr
                      ? 0.0
                      : candidatePatch->center.y - baselinePatch->center.y)
              << " statuses "
              << static_cast<int>(beforeGround.status) << ','
              << static_cast<int>(afterGround.status) << " roles "
              << static_cast<int>(beforeGround.role) << ','
              << static_cast<int>(afterGround.role) << '\n';
  }

  return expect(analysis.accepted && edit.accepted && selected &&
                    preview.accepted && foundChanged &&
                    candidateSurface.accepted,
                "contour selection produces one exact preview candidate") &&
         expect(expectedCandidateContours.accepted && previewContoursBuilt &&
                    previewContours.accepted &&
                    previewContours.segments ==
                        expectedCandidateContours.segments,
                "3D contour overlay consumes the preview terrain surface") &&
         expect(applied.accepted && applied.changed && appliedExact &&
                    appliedContoursBuilt && contourDisplay.plan.accepted &&
                    contourDisplay.plan.segments == previewContours.segments,
                "apply preserves preview terrain and contour geometry exactly") &&
         expect(collisionSynchronized,
                "applied contour edit moves walkable collision by the exact terrain delta");
}

bool regionOperationReopensAndUpdatesInPlace() {
  constexpr std::array<std::uint16_t, 9U> heights{
      4U, 4U, 4U,
      4U, 4U, 4U,
      4U, 4U, 4U,
  };
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(
      topographyDocument(2110U, heights)));
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
  request.sourceKey = "world_layout.region.2110";
  request.operationKind = cr::CreativeTerrainOperationKind::Region;
  request.region.bounds = {{-1, -1}, 2U, 2U};
  request.region.mode = cr::CreativeTerrainRegionMode::Flatten;
  request.region.targetHeightCells = 8U;
  const cr::CreativeTerrainOperationMutationReceipt seeded =
      appState.facade.applyTerrainOperationMutation(request);

  app::CreativeEditorState editor;
  app::CreativeEditorWorldLayoutTerrainRegionState& region =
      editor.worldLayoutTopography.region;
  const bool selected =
      app::selectCreativeEditorWorldLayoutTerrainRegionOperation(
          region, editor.terrainGeneration, appState.facade.document(),
          seeded.operationId);
  const bool loadedExactDraft =
      selected && region.recipe == request.region &&
      region.editingOperationId == seeded.operationId;
  region.recipe.targetHeightCells = 10U;
  const app::CreativeEditorTerrainGenerationPreviewReceipt preview =
      app::previewCreativeEditorWorldLayoutTerrainRegion(
          region, editor.terrainGeneration, appState.facade.document());
  const app::CreativeEditorTerrainGenerationApplyReceipt applied =
      app::applyCreativeEditorWorldLayoutTerrainRegion(
          region, editor.terrainGeneration, appState);
  const cr::CreativeTerrainOperationStack& stack =
      appState.facade.document().terrainOperationStack();

  return expect(loadedExactDraft &&
                    region.editingOperationId ==
                        cr::kInvalidCreativeTerrainOperationId,
                "saved region reloads its exact draft and closes after apply") &&
         expect(preview.accepted && applied.accepted && applied.changed &&
                    stack.operations.size() == 1U &&
                    stack.operations.front().id == seeded.operationId &&
                    stack.operations.front().owner ==
                        cr::CreativeTerrainOperationOwner::WorldLayout &&
                    stack.operations.front().sourceKey == request.sourceKey &&
                    stack.operations.front().region.targetHeightCells == 10U &&
                    appState.facade.document().terrainHeightField().heightAt(
                        {-1, -1}) == 10U,
                "reopened region updates the same durable operation") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "reopened region update records exactly one history entry");
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
  region.recipe.noiseScaleCells = std::numeric_limits<double>::infinity();
  app::CreativeDesktopCommandFrame rejectedFrame;
  rejectedFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  const app::CreativeDesktopCommandResult rejected =
      app::dispatchCreativeDesktopCommands(
          rejectedFrame, {appState, editor, {}, nullptr, nullptr, nullptr});
  const Phase rejectedPhase =
      app::classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);

  region.recipe.noiseScaleCells = 12.0;
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
  ok = previewAnalysisReportsCutFillAndSlopeBands() && ok;
  ok = contourAndHeightTargetsProduceSharedRegionRecipes() && ok;
  ok = capacityLimitsRemainAtomicAndUseful() && ok;
  ok = regionSelectionBuildsSharedOperationRecipes() && ok;
  ok = regionBoundsMoveAndResizeStayBounded() && ok;
  ok = regionRecipeIsIdenticalAcross2dAnd3dFrontends() && ok;
  ok = regionPreviewAndApplyAreExactAtomicAndUndoable() && ok;
  ok = analysisEditPreviewAndCollisionStaySynchronized() && ok;
  ok = regionOperationReopensAndUpdatesInPlace() && ok;
  ok = regionPhaseAndMetricsMirrorTheWorkflow() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
