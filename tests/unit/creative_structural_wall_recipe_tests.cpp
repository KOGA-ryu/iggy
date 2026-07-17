#include "app/iggy3d/creative/recipes/StructuralWallRecipe.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

static_assert(std::is_standard_layout_v<cr::CreativeStructuralWallFrame>);
static_assert(std::is_trivially_copyable_v<cr::CreativeStructuralWallFrame>);
static_assert(
    std::is_trivially_copyable_v<cr::CreativeStructuralWallOpeningRequest>);

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-9;
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

bool sameBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

cr::CreativeStructuralWallRecipeResult plan(
    cr::CreativeVec3 start, cr::CreativeVec3 end,
    std::span<const cr::CreativeStructuralWallOpeningRequest> openings = {},
    double edgeClearance = 0.0, double separation = 0.0) {
  return cr::planCreativeStructuralWall(
      {start, end, 3.0, 0.5, edgeClearance, separation, openings});
}

bool cardinalFramesRetainAuthoredDirection() {
  const auto reverseX = plan({8.0, 2.0, 4.0}, {0.0, 2.0, 4.0});
  const auto positiveZ = plan({-3.0, 1.0, -2.0}, {-3.0, 1.0, 5.0});

  return expect(
             reverseX.accepted &&
                 reverseX.frame.axis == cr::CreativeStructuralWallAxis::X &&
                 sameVec3(reverseX.frame.tangent, {-1.0, 0.0, 0.0}) &&
                 sameVec3(reverseX.frame.normal, {0.0, 0.0, 1.0}) &&
                 near(reverseX.frame.lengthMeters, 8.0) &&
                 sameBounds(reverseX.frame.bounds,
                            {{0.0, 2.0, 3.75}, {8.0, 5.0, 4.25}}) &&
                 reverseX.fullHeightSpans.size() == 1U &&
                 sameBounds(reverseX.fullHeightSpans[0], reverseX.frame.bounds),
             "reverse-X frame keeps start-relative tangent and exact bounds") &&
         expect(positiveZ.accepted &&
                    positiveZ.frame.axis == cr::CreativeStructuralWallAxis::Z &&
                    sameVec3(positiveZ.frame.tangent, {0.0, 0.0, 1.0}) &&
                    sameVec3(positiveZ.frame.normal, {1.0, 0.0, 0.0}) &&
                    near(positiveZ.frame.lengthMeters, 7.0) &&
                    sameBounds(positiveZ.frame.bounds,
                               {{-3.25, 1.0, -2.0}, {-2.75, 4.0, 5.0}}),
                "positive-Z frame publishes stable cardinal basis");
}

bool slotsProduceDeterministicSpansAndCutoutPieces() {
  cr::CreativeStructuralWallOpeningRequest window;
  window.sortKey = "window";
  window.centerOffsetMeters = 6.0;
  window.widthMeters = 2.0;
  window.cutoutBottomMeters = 1.0;
  window.cutoutHeightMeters = 1.0;
  window.insertBottomMeters = 1.0;

  cr::CreativeStructuralWallOpeningRequest door;
  door.sortKey = "door";
  door.centerOffsetMeters = 2.0;
  door.widthMeters = 1.0;
  door.cutoutHeightMeters = 2.1;

  const std::vector openings{window, door};
  const auto result = plan({0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, openings);

  return expect(result.accepted && result.openings.size() == 2U &&
                    result.fullHeightSpans.size() == 3U,
                "two openings produce three ordered full-height spans") &&
         expect(
             result.openings[0].sourceIndex == 1U &&
                 result.openings[1].sourceIndex == 0U,
             "opening plans retain source identity after spatial ordering") &&
         expect(sameBounds(result.fullHeightSpans[0],
                           {{0.0, 0.0, -0.25}, {1.5, 3.0, 0.25}}) &&
                    sameBounds(result.fullHeightSpans[1],
                               {{2.5, 0.0, -0.25}, {5.0, 3.0, 0.25}}) &&
                    sameBounds(result.fullHeightSpans[2],
                               {{7.0, 0.0, -0.25}, {10.0, 3.0, 0.25}}),
                "spans stop at exact slot boundaries") &&
         expect(!result.openings[0].hasSill && result.openings[0].hasLintel &&
                    result.openings[0].hasInsert &&
                    sameBounds(result.openings[0].lintelBounds,
                               {{1.5, 2.1, -0.25}, {2.5, 3.0, 0.25}}),
                "door slot emits lintel and insert without sill") &&
         expect(result.openings[1].hasSill && result.openings[1].hasLintel &&
                    result.openings[1].hasInsert &&
                    sameBounds(result.openings[1].sillBounds,
                               {{5.0, 0.0, -0.25}, {7.0, 1.0, 0.25}}) &&
                    sameBounds(result.openings[1].insertBounds,
                               {{5.0, 1.0, -0.25}, {7.0, 2.0, 0.25}}),
                "window slot emits exact sill lintel and insert geometry");
}

bool openPoseUsesStableWallLocalFrame() {
  cr::CreativeStructuralWallOpeningRequest opening;
  opening.sortKey = "open-door";
  opening.centerOffsetMeters = 3.0;
  opening.widthMeters = 2.0;
  opening.cutoutHeightMeters = 2.5;
  opening.insertHeightMeters = 2.0;
  opening.insertWidthMeters = 1.8;
  opening.insertThicknessMeters = 0.2;
  opening.pose =
      cr::CreativeStructuralWallOpeningPose::OpenFromEndPositiveNormal;
  const std::vector openings{opening};
  const auto result = plan({4.0, 1.0, 10.0}, {4.0, 1.0, 0.0}, openings);

  return expect(result.accepted && result.openings.size() == 1U &&
                    sameVec3(result.frame.tangent, {0.0, 0.0, -1.0}),
                "reverse-Z wall retains authored tangent") &&
         expect(sameBounds(result.openings[0].insertBounds,
                           {{4.0, 1.0, 6.0}, {5.8, 3.0, 6.2}}),
                "open insert hinges from authored end toward positive normal");
}

bool invalidAndCrowdedGeometryFailsClosed() {
  const auto diagonal = plan({0.0, 0.0, 0.0}, {4.0, 0.0, 2.0});
  const auto uneven = plan({0.0, 0.0, 0.0}, {4.0, 1.0, 0.0});

  cr::CreativeStructuralWallOpeningRequest edge;
  edge.centerOffsetMeters = 0.5;
  edge.widthMeters = 1.0;
  const std::vector edgeOpenings{edge};
  const auto atEdge = plan({0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, edgeOpenings);

  edge.centerOffsetMeters = 1.25;
  const std::vector clearanceOpenings{edge};
  const auto belowClearance =
      plan({0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, clearanceOpenings, 1.0);

  cr::CreativeStructuralWallOpeningRequest first;
  first.sortKey = "first";
  first.centerOffsetMeters = 3.0;
  first.widthMeters = 2.0;
  cr::CreativeStructuralWallOpeningRequest second = first;
  second.sortKey = "second";
  second.centerOffsetMeters = 5.25;
  const std::vector separated{first, second};
  const auto insufficientSeparation =
      plan({0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, separated, 0.0, 0.5);

  first.insertWidthMeters = -1.0;
  const std::vector negativeInsert{first};
  const auto invalidInsert =
      plan({0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, negativeInsert);

  return expect(!diagonal.accepted &&
                    diagonal.status == cr::CreativeStructuralWallRecipeStatus::
                                           UnsupportedOrientation &&
                    diagonal.fullHeightSpans.empty(),
                "diagonal wall rejects without geometry") &&
         expect(!uneven.accepted && uneven.status ==
                                        cr::CreativeStructuralWallRecipeStatus::
                                            UnsupportedOrientation,
                "uneven wall plane rejects") &&
         expect(!atEdge.accepted &&
                    atEdge.status ==
                        cr::CreativeStructuralWallRecipeStatus::InvalidOpening,
                "opening cannot consume a wall endpoint") &&
         expect(!belowClearance.accepted &&
                    belowClearance.status ==
                        cr::CreativeStructuralWallRecipeStatus::InvalidOpening,
                "explicit edge clearance is enforced") &&
         expect(!insufficientSeparation.accepted &&
                    insufficientSeparation.status ==
                        cr::CreativeStructuralWallRecipeStatus::
                            OverlappingOpenings &&
                    insufficientSeparation.failedOpeningIndex == 1U,
                "explicit slot separation reports the crowded opening") &&
         expect(!invalidInsert.accepted &&
                    invalidInsert.status ==
                        cr::CreativeStructuralWallRecipeStatus::InvalidOpening,
                "negative insert dimensions do not masquerade as inheritance");
}

bool nonFiniteAndOverflowingInputsReject() {
  const double infinity = std::numeric_limits<double>::infinity();
  const double maximum = std::numeric_limits<double>::max();
  const auto nonFinite = plan({0.0, 0.0, 0.0}, {infinity, 0.0, 0.0});
  const auto overflow = cr::planCreativeStructuralWall(
      {{0.0, maximum, 0.0}, {1.0, maximum, 0.0}, maximum, 0.5, 0.0, 0.0, {}});

  cr::CreativeStructuralWallOpeningRequest invalidPose;
  invalidPose.centerOffsetMeters = 2.0;
  invalidPose.pose = static_cast<cr::CreativeStructuralWallOpeningPose>(255U);
  const std::vector invalidPoses{invalidPose};
  const auto pose = plan({0.0, 0.0, 0.0}, {8.0, 0.0, 0.0}, invalidPoses);

  return expect(!nonFinite.accepted &&
                    nonFinite.status ==
                        cr::CreativeStructuralWallRecipeStatus::InvalidWall,
                "non-finite endpoint rejects") &&
         expect(!overflow.accepted &&
                    overflow.status == cr::CreativeStructuralWallRecipeStatus::
                                           UnrepresentableGeometry,
                "overflowing wall bounds reject") &&
         expect(!pose.accepted &&
                    pose.status ==
                        cr::CreativeStructuralWallRecipeStatus::InvalidOpening,
                "invalid opening pose rejects");
}

}  // namespace

int main() {
  const bool ok = cardinalFramesRetainAuthoredDirection() &&
                  slotsProduceDeterministicSpansAndCutoutPieces() &&
                  openPoseUsesStableWallLocalFrame() &&
                  invalidAndCrowdedGeometryFailsClosed() &&
                  nonFiniteAndOverflowingInputsReject();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
