#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutTopography.hpp"

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

}  // namespace

int main() {
  bool ok = true;
  ok = planIsBoundedCanonicalAndCached() && ok;
  ok = samplesHeightAndLocalSlope() && ok;
  ok = renderDocumentSwitchesCommittedAndPreviewTerrain() && ok;
  ok = invalidRequestsFailClosed() && ok;
  ok = capacityLimitsRemainAtomicAndUseful() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
