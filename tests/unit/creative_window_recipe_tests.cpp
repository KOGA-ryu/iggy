#include "app/iggy3d/creative/recipes/WindowRecipe.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-9;
}

cr::CreativeStructuralWallFrame xWall() {
  cr::CreativeStructuralWallFrame frame;
  frame.axis = cr::CreativeStructuralWallAxis::X;
  frame.start = {0.0, 0.0, 0.0};
  frame.end = {4.0, 0.0, 0.0};
  frame.tangent = {1.0, 0.0, 0.0};
  frame.normal = {0.0, 0.0, 1.0};
  frame.bounds = {{0.0, 0.0, -0.125}, {4.0, 3.0, 0.125}};
  frame.baseYMeters = 0.0;
  frame.lengthMeters = 4.0;
  frame.heightMeters = 3.0;
  frame.thicknessMeters = 0.25;
  return frame;
}

cr::CreativeWindowRecipeRequest windowRequest() {
  cr::CreativeWindowRecipeRequest request;
  request.wallFrame = xWall();
  request.cutoutBounds = {{1.0, 0.9, -0.125}, {2.5, 2.1, 0.125}};
  return request;
}

bool glazingOwnsExactFrameAndInsetPane() {
  const cr::CreativeWindowRecipeResult result =
      cr::planCreativeWindow(windowRequest());
  return expect(result.accepted &&
                    result.status == cr::CreativeWindowRecipeStatus::Ready,
                "glazing recipe accepted") &&
         expect(result.partCount == 5U &&
                    result.primaryInsertPartIndex == 4U,
                "glazing has four frame parts and one insert") &&
         expect(result.parts[0].kind ==
                        cr::CreativeWindowPartKind::MinimumJamb &&
                    result.parts[1].kind ==
                        cr::CreativeWindowPartKind::MaximumJamb &&
                    result.parts[2].kind ==
                        cr::CreativeWindowPartKind::FrameSill &&
                    result.parts[3].kind ==
                        cr::CreativeWindowPartKind::FrameHeader,
                "window frame has deterministic part order") &&
         expect(result.parts[4].kind ==
                        cr::CreativeWindowPartKind::PrimaryInsert &&
                    result.parts[4].objectKind ==
                        cr::CreativeObjectKind::Window,
                "glazing insert carries semantic window kind") &&
         expect(near(result.framedOpeningBounds.min.x, 1.06) &&
                    near(result.framedOpeningBounds.max.x, 2.44) &&
                    near(result.framedOpeningBounds.min.y, 0.96) &&
                    near(result.framedOpeningBounds.max.y, 2.04) &&
                    near(result.framedOpeningBounds.min.z, -0.01) &&
                    near(result.framedOpeningBounds.max.z, 0.01),
                "pane is inset from frame and wall faces");
}

bool proceduralShuttersSplitTheInsertDeterministically() {
  cr::CreativeWindowRecipeRequest request = windowRequest();
  request.settings.insertKind =
      cr::CreativeWindowInsertKind::PairedShutters;
  const cr::CreativeWindowRecipeResult result =
      cr::planCreativeWindow(request);
  return expect(result.accepted && result.partCount == 6U,
                "paired shutters use bounded six-part assembly") &&
         expect(result.parts[4].objectKind ==
                        cr::CreativeObjectKind::Window &&
                    result.parts[5].kind ==
                        cr::CreativeWindowPartKind::SecondaryShutter &&
                    result.parts[5].objectKind ==
                        cr::CreativeObjectKind::Prop,
                "primary shutter owns semantic identity") &&
         expect(near(result.parts[4].bounds.max.x, 1.745) &&
                    near(result.parts[5].bounds.min.x, 1.755),
                "paired shutters preserve the center gap");
}

bool importedInsertUsesOneExactAssemblyTarget() {
  cr::CreativeWindowRecipeRequest request = windowRequest();
  request.settings.insertKind =
      cr::CreativeWindowInsertKind::PairedShutters;
  request.proceduralInsert = false;
  const cr::CreativeWindowRecipeResult result =
      cr::planCreativeWindow(request);
  return expect(result.accepted && result.partCount == 5U,
                "asset-backed shutter assembly has one fit target") &&
         expect(result.parts[4].bounds.min.x ==
                        result.framedOpeningBounds.min.x &&
                    result.parts[4].bounds.max.x ==
                        result.framedOpeningBounds.max.x,
                "asset target is the exact framed opening");
}

bool invalidInputsFailClosed() {
  cr::CreativeWindowRecipeRequest outside = windowRequest();
  outside.cutoutBounds.max.x = 5.0;
  const cr::CreativeWindowRecipeResult outsideResult =
      cr::planCreativeWindow(outside);

  cr::CreativeWindowRecipeRequest invalidSettings = windowRequest();
  invalidSettings.settings.insertKind = cr::CreativeWindowInsertKind::Count;
  const cr::CreativeWindowRecipeResult settingsResult =
      cr::planCreativeWindow(invalidSettings);

  cr::CreativeWindowRecipeRequest impossible = windowRequest();
  impossible.frameWidthMeters = 0.8;
  const cr::CreativeWindowRecipeResult impossibleResult =
      cr::planCreativeWindow(impossible);

  cr::CreativeWindowRecipeRequest nonFinite = windowRequest();
  nonFinite.insertGapMeters = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWindowRecipeResult nonFiniteResult =
      cr::planCreativeWindow(nonFinite);
  return expect(!outsideResult.accepted &&
                    outsideResult.status ==
                        cr::CreativeWindowRecipeStatus::InvalidCutout,
                "outside cutout rejected") &&
         expect(!settingsResult.accepted &&
                    settingsResult.status ==
                        cr::CreativeWindowRecipeStatus::InvalidSettings,
                "invalid treatment rejected") &&
         expect(!impossibleResult.accepted &&
                    impossibleResult.status ==
                        cr::CreativeWindowRecipeStatus::InvalidDimensions,
                "unrepresentable frame rejected") &&
         expect(!nonFiniteResult.accepted &&
                    nonFiniteResult.status ==
                        cr::CreativeWindowRecipeStatus::InvalidDimensions,
                "non-finite dimensions rejected");
}

}  // namespace

int main() {
  const bool ok = glazingOwnsExactFrameAndInsetPane() &&
                  proceduralShuttersSplitTheInsertDeterministically() &&
                  importedInsertUsesOneExactAssemblyTarget() &&
                  invalidInputsFailClosed();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_window_recipe_tests: PASS\n";
  return EXIT_SUCCESS;
}
