#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorTerrain.hpp"

#include "app/iggy3d/creative/document/Document.hpp"

#include <array>
#include <cmath>
#include <iostream>
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

bool near(float actual, float expected, float epsilon = 1.0e-4F) {
  return std::fabs(actual - expected) <= epsilon;
}

cr::CreativeDocument contourDocument() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Contours");
  static_cast<void>(document.assignId(1301U));
  static_cast<void>(document.setGridSettings(
      {{10.0, 2.0, -4.0}, 2.0, {16, 16, 16}}));
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 2U, 2U};
  constexpr std::array<std::uint16_t, 4U> heights{1U, 3U, 1U, 3U};
  static_cast<void>(document.replaceTerrainHeightField(bounds, heights));
  return document;
}

bool contourDisplayCachesAndConvertsToWorldLines() {
  const cr::CreativeDocument document = contourDocument();
  app::CreativeEditorState editor;
  app::CreativeTerrainContourDisplayState& contours = editor.terrain.contours;

  const bool hiddenBuild = app::refreshCreativeEditorTerrainContours(
      contours, document);
  contours.visible = true;
  const bool firstBuild = app::refreshCreativeEditorTerrainContours(
      contours, document);
  bool idleReused = true;
  for (std::uint32_t frame = 0U; frame < 300U; ++frame) {
    idleReused = !app::refreshCreativeEditorTerrainContours(
                     contours, document) &&
                 idleReused;
  }

  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t appended = app::appendCreativeEditorTerrainContours(
      document, editor, 0.04F, lines);
  const std::size_t captureAppended =
      app::appendCreativeEditorTerrainContours(document, editor, 0.04F,
                                               lines, true);

  return expect(!hiddenBuild && contours.buildCount == 1U && firstBuild &&
                    idleReused,
                "hidden contours do no work and 300 idle frames reuse cache") &&
         expect(contours.plan.accepted && contours.plan.segments.size() == 1U,
                "default two-cell cadence produces one line") &&
         expect(appended == 1U && captureAppended == 0U && lines.size() == 1U,
                "visible contours append once and capture mode hides them") &&
         expect(near(lines[0].start.x, 11.5F) &&
                    near(lines[0].start.z, -3.0F) &&
                    near(lines[0].end.x, 11.5F) &&
                    near(lines[0].end.z, -1.0F) &&
                    near(lines[0].start.y, 5.04F),
                "grid-space contour converts to the lifted world surface") &&
         expect(near(lines[0].color.r, 0.18F) &&
                    near(lines[0].color.g, 0.70F) &&
                    near(lines[0].color.b, 0.78F),
                "minor contour uses the restrained minor role");
}

bool settingsAndPreviewSurfaceInvalidateExactlyOnce() {
  const cr::CreativeDocument document = contourDocument();
  app::CreativeEditorState editor;
  app::CreativeTerrainContourDisplayState& contours = editor.terrain.contours;
  contours.visible = true;
  static_cast<void>(app::refreshCreativeEditorTerrainContours(
      contours, document));

  contours.intervalCells = 1U;
  const bool settingsBuild = app::refreshCreativeEditorTerrainContours(
      contours, document);
  const bool settingsReuse = app::refreshCreativeEditorTerrainContours(
      contours, document);

  app::CreativeEditorSceneCache sceneCache;
  const bool sceneBuilt =
      app::refreshCreativeEditorSceneCache(sceneCache, document);
  const bool overrideBuild = app::refreshCreativeEditorTerrainContours(
      contours, document, &sceneCache.composedTerrainSurface,
      sceneCache.terrainSurfaceBuildCount);
  const bool overrideReuse = app::refreshCreativeEditorTerrainContours(
      contours, document, &sceneCache.composedTerrainSurface,
      sceneCache.terrainSurfaceBuildCount);
  const bool previewKeyBuild = app::refreshCreativeEditorTerrainContours(
      contours, document, &sceneCache.composedTerrainSurface, 0xA55AU);

  return expect(settingsBuild && !settingsReuse && contours.buildCount == 4U,
                "cadence and preview keys each rebuild exactly once") &&
         expect(contours.plan.accepted && contours.plan.segments.size() == 2U,
                "one-cell cadence exposes both terrain levels") &&
         expect(sceneBuilt && sceneCache.composedTerrainSurface.accepted &&
                    overrideBuild && !overrideReuse && previewKeyBuild,
                "scene cache surface is the contour source and preview key") &&
         expect(sceneCache.terrainSurfaceBuildCount == 1U,
                "contour refresh never rebuilds the terrain surface");
}

}  // namespace

int main() {
  bool ok = true;
  ok = contourDisplayCachesAndConvertsToWorldLines() && ok;
  ok = settingsAndPreviewSurfaceInvalidateExactlyOnce() && ok;
  return ok ? 0 : 1;
}
