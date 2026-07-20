#include "EditorToolGlyphs.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using iggy3d_creative_app::CreativeEditorToolGlyph;
using iggy3d_creative_app::CreativeEditorToolGlyphOp;
using iggy3d_creative_app::CreativeEditorToolGlyphOpKind;
using iggy3d_creative_app::CreativeEditorToolGlyphPoint;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float actual, float expected) {
  return std::fabs(actual - expected) <= 1.0e-3F;
}

constexpr std::size_t kGlyphCount =
    static_cast<std::size_t>(CreativeEditorToolGlyph::Count);

bool everyGlyphCarriesInkAndAUniqueName() {
  bool allInked = true;
  bool allNamed = true;
  bool allUnique = true;
  for (std::size_t index = 0U; index < kGlyphCount; ++index) {
    const auto glyph = static_cast<CreativeEditorToolGlyph>(index);
    const auto ops = iggy3d_creative_app::creativeEditorToolGlyphOps(glyph);
    const std::string_view name = iggy3d_creative_app::toString(glyph);
    if (ops.empty()) {
      std::cerr << "glyph without ops: " << name << '\n';
      allInked = false;
    }
    if (name.empty() || name == "unknown") {
      std::cerr << "glyph without a name at index " << index << '\n';
      allNamed = false;
    }
    for (std::size_t other = index + 1U; other < kGlyphCount; ++other) {
      const std::string_view otherName = iggy3d_creative_app::toString(
          static_cast<CreativeEditorToolGlyph>(other));
      if (name == otherName) {
        std::cerr << "duplicate glyph name: " << name << '\n';
        allUnique = false;
      }
    }
  }
  const auto invalidOps = iggy3d_creative_app::creativeEditorToolGlyphOps(
      CreativeEditorToolGlyph::Count);
  return expect(allInked, "every glyph owns at least one draw op") &&
         expect(allNamed, "every glyph owns a real name") &&
         expect(allUnique, "glyph names never collide") &&
         expect(invalidOps.empty(), "out-of-range glyphs draw nothing") &&
         expect(iggy3d_creative_app::toString(CreativeEditorToolGlyph::Count) ==
                    "unknown",
                "out-of-range glyphs answer unknown");
}

bool glyphGeometryStaysOnTheDraftingGrid() {
  bool allOnGrid = true;
  bool allSubstantial = true;
  bool allOpsWellFormed = true;
  for (std::size_t index = 0U; index < kGlyphCount; ++index) {
    const auto glyph = static_cast<CreativeEditorToolGlyph>(index);
    const std::string_view name = iggy3d_creative_app::toString(glyph);
    const auto bounds =
        iggy3d_creative_app::measureCreativeEditorToolGlyphBounds(glyph);
    if (!bounds.present || bounds.minimumX < 0.0F || bounds.minimumY < 0.0F ||
        bounds.maximumX > 24.0F || bounds.maximumY > 24.0F) {
      std::cerr << "glyph escapes the 24-cell grid: " << name << '\n';
      allOnGrid = false;
    }
    const float widthSpan = bounds.maximumX - bounds.minimumX;
    const float heightSpan = bounds.maximumY - bounds.minimumY;
    if (bounds.present && widthSpan < 11.9F && heightSpan < 11.9F) {
      std::cerr << "glyph surrenders too much space: " << name << '\n';
      allSubstantial = false;
    }
    for (const CreativeEditorToolGlyphOp& op :
         iggy3d_creative_app::creativeEditorToolGlyphOps(glyph)) {
      const bool alphaSane = op.alpha > 0.0F && op.alpha <= 1.0F;
      const bool widthSane = op.widthCells > 0.0F && op.widthCells <= 1.25F;
      const bool dashSane =
          (op.dashCells <= 0.0F) == (op.gapCells <= 0.0F) &&
          op.dashCells >= 0.0F && op.gapCells >= 0.0F;
      bool pointsSane = true;
      switch (op.kind) {
        case CreativeEditorToolGlyphOpKind::Stroke:
          pointsSane = op.points.size() >= 2U;
          break;
        case CreativeEditorToolGlyphOpKind::Fill:
          pointsSane = op.points.size() >= 3U;
          break;
        case CreativeEditorToolGlyphOpKind::Ellipse:
        case CreativeEditorToolGlyphOpKind::EllipseFilled:
          pointsSane = op.points.size() == 2U && op.points[1].x > 0.0F &&
                       op.points[1].y > 0.0F;
          break;
        case CreativeEditorToolGlyphOpKind::Path:
          pointsSane = op.points.size() >= 4U &&
                       (op.points.size() - 1U) % 3U == 0U;
          break;
        case CreativeEditorToolGlyphOpKind::Count:
          pointsSane = false;
          break;
      }
      if (!alphaSane || !widthSane || !dashSane || !pointsSane) {
        std::cerr << "malformed op in glyph: " << name << '\n';
        allOpsWellFormed = false;
      }
    }
  }
  return expect(allOnGrid, "every glyph stays within the 24-cell grid") &&
         expect(allSubstantial,
                "every glyph spans at least half the grid on one axis") &&
         expect(allOpsWellFormed,
                "every op carries sane alpha, width, dash, and point counts");
}

bool toolSurfaceIsFullyIconified() {
  bool allToolsMapped = true;
  const auto toolCount =
      static_cast<std::size_t>(iggy3d_creative_app::CreativeEditorWorldLayoutTool::Count);
  for (std::size_t index = 0U; index < toolCount; ++index) {
    const auto tool =
        static_cast<iggy3d_creative_app::CreativeEditorWorldLayoutTool>(index);
    const auto glyph =
        iggy3d_creative_app::creativeEditorToolGlyphForWorldLayoutTool(tool);
    if (iggy3d_creative_app::creativeEditorToolGlyphOps(glyph).empty()) {
      std::cerr << "unmapped world layout tool at index " << index << '\n';
      allToolsMapped = false;
    }
  }
  const auto selectGlyph =
      iggy3d_creative_app::creativeEditorToolGlyphForWorldLayoutTool(
          iggy3d_creative_app::CreativeEditorWorldLayoutTool::Select);
  const auto wallGlyph =
      iggy3d_creative_app::creativeEditorToolGlyphForWorldLayoutTool(
          iggy3d_creative_app::CreativeEditorWorldLayoutTool::Wall);
  const auto catalogGlyph =
      iggy3d_creative_app::creativeEditorToolGlyphForWorldLayoutTool(
          iggy3d_creative_app::CreativeEditorWorldLayoutTool::CatalogAsset);
  const auto smoothGlyph =
      iggy3d_creative_app::creativeEditorToolGlyphForTerrainRegionOperation(
          iggy3d_creative_app::CreativeEditorWorldLayoutTerrainRegionOperation::
              Smooth);
  const auto ellipseGlyph =
      iggy3d_creative_app::creativeEditorToolGlyphForTerrainMask(
          iggy3d::creative::CreativeTerrainCompositionMask::Ellipse);
  const auto terrainParent =
      iggy3d_creative_app::creativeEditorToolGlyphForPaletteCategory(
          iggy3d_creative_app::CreativeEditorWorldLayoutPaletteCategory::
              Terrain);
  const auto coverGlyph =
      iggy3d_creative_app::creativeEditorToolGlyphForAssetCategory(
          iggy3d_creative_app::CreativeEditorWorldLayoutAssetCategory::Cover);
  const auto gameplayAssets =
      iggy3d_creative_app::creativeEditorToolGlyphForAssetCategory(
          iggy3d_creative_app::CreativeEditorWorldLayoutAssetCategory::
              Gameplay);
  return expect(allToolsMapped, "every world layout tool maps to a glyph") &&
         expect(selectGlyph == CreativeEditorToolGlyph::ParentSelect,
                "select keeps the cursor glyph") &&
         expect(wallGlyph == CreativeEditorToolGlyph::Partition,
                "wall maps to the partition glyph") &&
         expect(catalogGlyph == CreativeEditorToolGlyph::AssetBox,
                "catalog asset maps to the asset box glyph") &&
         expect(smoothGlyph == CreativeEditorToolGlyph::RegionSmooth,
                "smooth maps to the region smooth glyph") &&
         expect(ellipseGlyph == CreativeEditorToolGlyph::MaskEllipse,
                "ellipse mask maps to the dashed ellipse glyph") &&
         expect(terrainParent == CreativeEditorToolGlyph::ParentTerrain,
                "terrain palette category maps to the terrain parent") &&
         expect(coverGlyph == CreativeEditorToolGlyph::AssetCover,
                "cover asset category maps to the cover glyph") &&
         expect(gameplayAssets == CreativeEditorToolGlyph::ParentGameplay,
                "gameplay asset category reuses the flag parent");
}

bool strokeThicknessKeepsAOnePixelFloor() {
  const float atSixteen =
      iggy3d_creative_app::creativeEditorToolGlyphStrokeThicknessPixels(1.25F,
                                                                        16.0F);
  const float atTwentyFour =
      iggy3d_creative_app::creativeEditorToolGlyphStrokeThicknessPixels(1.25F,
                                                                        24.0F);
  const float atFortyEight =
      iggy3d_creative_app::creativeEditorToolGlyphStrokeThicknessPixels(1.25F,
                                                                        48.0F);
  return expect(near(atSixteen, 1.0F),
                "sixteen-pixel icons pin the one-pixel floor") &&
         expect(near(atTwentyFour, 1.25F),
                "twenty-four-pixel icons render the authored width") &&
         expect(near(atFortyEight, 2.5F),
                "forty-eight-pixel icons scale proportionally");
}

bool dashSegmentationTilesThePatternExactly() {
  const CreativeEditorToolGlyphPoint openLine[] = {{0.0F, 0.0F},
                                                   {10.0F, 0.0F}};
  std::vector<std::pair<CreativeEditorToolGlyphPoint,
                        CreativeEditorToolGlyphPoint>>
      openSegments;
  iggy3d_creative_app::appendCreativeEditorToolGlyphDashes(
      openLine, false, 2.0F, 2.0F, openSegments);
  float openLength = 0.0F;
  for (const auto& segment : openSegments) {
    openLength += std::fabs(segment.second.x - segment.first.x);
  }
  const CreativeEditorToolGlyphPoint square[] = {{0.0F, 0.0F},
                                                 {10.0F, 0.0F},
                                                 {10.0F, 10.0F},
                                                 {0.0F, 10.0F}};
  std::vector<std::pair<CreativeEditorToolGlyphPoint,
                        CreativeEditorToolGlyphPoint>>
      closedSegments;
  iggy3d_creative_app::appendCreativeEditorToolGlyphDashes(
      square, true, 2.0F, 2.0F, closedSegments);
  float closedLength = 0.0F;
  bool closedOnPerimeter = true;
  for (const auto& segment : closedSegments) {
    closedLength += std::fabs(segment.second.x - segment.first.x) +
                    std::fabs(segment.second.y - segment.first.y);
    for (const CreativeEditorToolGlyphPoint point :
         {segment.first, segment.second}) {
      if (point.x < 0.0F || point.x > 10.0F || point.y < 0.0F ||
          point.y > 10.0F) {
        closedOnPerimeter = false;
      }
    }
  }
  std::vector<std::pair<CreativeEditorToolGlyphPoint,
                        CreativeEditorToolGlyphPoint>>
      solidSegments;
  iggy3d_creative_app::appendCreativeEditorToolGlyphDashes(
      openLine, false, 0.0F, 0.0F, solidSegments);
  return expect(openSegments.size() == 3U,
                "a ten-cell line with a two-two pattern yields three dashes") &&
         expect(near(openSegments.front().first.x, 0.0F) &&
                    near(openSegments.back().second.x, 10.0F),
                "dashing starts at the line start and ends at the line end") &&
         expect(near(openLength, 6.0F),
                "open-line dash coverage totals six cells") &&
         expect(closedSegments.size() == 10U,
                "a forty-cell perimeter yields ten dashes") &&
         expect(near(closedLength, 20.0F),
                "closed-loop dash coverage totals half the perimeter") &&
         expect(closedOnPerimeter, "dash segments never leave the outline") &&
         expect(solidSegments.empty(),
                "solid strokes produce no dash segments");
}

bool pathFlatteningIsEndpointExact() {
  const CreativeEditorToolGlyphPoint straightChain[] = {{0.0F, 0.0F},
                                                        {3.0F, 0.0F},
                                                        {6.0F, 0.0F},
                                                        {9.0F, 0.0F}};
  std::vector<CreativeEditorToolGlyphPoint> flattened;
  iggy3d_creative_app::flattenCreativeEditorToolGlyphPath(straightChain,
                                                          flattened);
  bool stayedStraight = true;
  bool monotonic = true;
  for (std::size_t index = 0U; index < flattened.size(); ++index) {
    if (std::fabs(flattened[index].y) > 1.0e-4F) {
      stayedStraight = false;
    }
    if (index > 0U && flattened[index].x < flattened[index - 1U].x - 1.0e-4F) {
      monotonic = false;
    }
  }
  std::vector<CreativeEditorToolGlyphPoint> ellipsePoints;
  iggy3d_creative_app::flattenCreativeEditorToolGlyphEllipse(
      {5.0F, 5.0F}, {2.0F, 3.0F}, ellipsePoints);
  bool ellipseBounded = true;
  for (const CreativeEditorToolGlyphPoint point : ellipsePoints) {
    if (point.x < 3.0F - 1.0e-4F || point.x > 7.0F + 1.0e-4F ||
        point.y < 2.0F - 1.0e-4F || point.y > 8.0F + 1.0e-4F) {
      ellipseBounded = false;
    }
  }
  return expect(flattened.size() == 13U,
                "one cubic flattens to twelve segments plus the start") &&
         expect(near(flattened.front().x, 0.0F) &&
                    near(flattened.back().x, 9.0F),
                "flattening preserves the chain endpoints") &&
         expect(stayedStraight, "a straight chain flattens to a straight line") &&
         expect(monotonic, "flattened points advance monotonically") &&
         expect(ellipsePoints.size() == 48U,
                "ellipses flatten to forty-eight points") &&
         expect(ellipseBounded, "ellipse points stay within the radii box");
}

bool viewControlGlyphsJoinTheVocabulary() {
  constexpr std::string_view kViewControlNames[] = {
      "plan view",     "elevation view",      "3d view",
      "level up",      "level down",          "fit all",
      "fit selection", "roof visibility",     "lower level context",
      "contours",      "dimensions",          "snap"};
  bool allPresent = true;
  for (const std::string_view expected : kViewControlNames) {
    bool found = false;
    for (std::size_t index = 0U; index < kGlyphCount; ++index) {
      if (iggy3d_creative_app::toString(
              static_cast<CreativeEditorToolGlyph>(index)) == expected) {
        found = true;
      }
    }
    if (!found) {
      std::cerr << "missing view-control glyph: " << expected << '\n';
      allPresent = false;
    }
  }
  const auto ladder = [](float sizePixels) {
    return iggy3d_creative_app::creativeEditorToolGlyphStrokeThicknessPixels(
        1.25F, sizePixels);
  };
  return expect(kGlyphCount == 55U,
                "the vocabulary is the original 43 plus 12 view controls") &&
         expect(allPresent, "every view-control glyph is enumerated") &&
         expect(near(ladder(16.0F), 1.0F) && near(ladder(20.0F), 1.041667F) &&
                    near(ladder(24.0F), 1.25F) &&
                    near(ladder(32.0F), 1.666667F),
                "the icon-size ladder holds at 16, 20, 24, and 32 pixels");
}

}  // namespace

int main() {
  bool ok = true;
  ok = everyGlyphCarriesInkAndAUniqueName() && ok;
  ok = glyphGeometryStaysOnTheDraftingGrid() && ok;
  ok = toolSurfaceIsFullyIconified() && ok;
  ok = strokeThicknessKeepsAOnePixelFloor() && ok;
  ok = dashSegmentationTilesThePatternExactly() && ok;
  ok = pathFlatteningIsEndpointExact() && ok;
  ok = viewControlGlyphsJoinTheVocabulary() && ok;
  return ok ? 0 : 1;
}
