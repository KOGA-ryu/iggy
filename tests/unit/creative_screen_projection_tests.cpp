#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"

#include <cmath>
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

bool near(float actual, float expected) {
  return std::fabs(actual - expected) <= 1.0e-4F;
}

iggy3d::Mat4 matrixWithConstantW(float clipW) {
  iggy3d::Mat4 matrix = iggy3d::identityMat4();
  matrix.m[12] = 0.0F;
  matrix.m[13] = 0.0F;
  matrix.m[14] = 0.0F;
  matrix.m[15] = clipW;
  return matrix;
}

iggy3d::Mat4 matrixWithNegativeZAsW() {
  iggy3d::Mat4 matrix = iggy3d::identityMat4();
  matrix.m[12] = 0.0F;
  matrix.m[13] = 0.0F;
  matrix.m[14] = -1.0F;
  matrix.m[15] = 0.0F;
  return matrix;
}

bool pointsOwnPixelMappingAndVisibilityFacts() {
  const iggy3d::Mat4 identity = iggy3d::identityMat4();
  const cr::CreativeScreenPoint center =
      cr::projectCreativeWorldPointToScreen(identity, {}, 200U, 100U);
  const cr::CreativeScreenPoint upperRight =
      cr::projectCreativeWorldPointToScreen(identity, {1.0F, 1.0F, 0.25F},
                                             200U, 100U);
  const cr::CreativeScreenPoint outside =
      cr::projectCreativeWorldPointToScreen(identity, {2.0F, 0.0F, 0.0F},
                                             200U, 100U);
  const cr::CreativeScreenPoint retina =
      cr::projectCreativeWorldPointToScreen(identity, {0.5F, -0.5F, 0.0F},
                                             400U, 200U);

  return expect(center.valid && center.insideViewport &&
                    center.status ==
                        cr::CreativeScreenProjectionStatus::Projected &&
                    near(center.x, 100.0F) && near(center.y, 50.0F),
                "identity origin maps to viewport center") &&
         expect(upperRight.valid && upperRight.insideViewport &&
                    near(upperRight.x, 200.0F) && near(upperRight.y, 0.0F) &&
                    near(upperRight.ndcDepth, 0.25F) &&
                    near(upperRight.clipW, 1.0F),
                "NDC corner maps to drawable corner with depth facts") &&
         expect(outside.valid && !outside.insideViewport &&
                    near(outside.x, 300.0F),
                "offscreen point remains a valid unclipped projection") &&
         expect(retina.valid && near(retina.x, 300.0F) &&
                    near(retina.y, 150.0F),
                "drawable extent scales projected pixels deterministically");
}

bool pointFailuresAreExplicitAndFailClosed() {
  iggy3d::Mat4 invalidMatrix = iggy3d::identityMat4();
  invalidMatrix.m[0] = std::numeric_limits<float>::quiet_NaN();
  iggy3d::Mat4 overflowingMatrix = iggy3d::identityMat4();
  overflowingMatrix.m[0] = std::numeric_limits<float>::max();
  const cr::CreativeScreenPoint invalidViewport =
      cr::projectCreativeWorldPointToScreen(iggy3d::identityMat4(), {}, 0U,
                                             100U);
  const cr::CreativeScreenPoint invalidWorld =
      cr::projectCreativeWorldPointToScreen(
          iggy3d::identityMat4(),
          {std::numeric_limits<float>::infinity(), 0.0F, 0.0F}, 100U, 100U);
  const cr::CreativeScreenPoint behind =
      cr::projectCreativeWorldPointToScreen(matrixWithConstantW(-1.0F), {},
                                             100U, 100U);
  const cr::CreativeScreenPoint degenerate =
      cr::projectCreativeWorldPointToScreen(
          matrixWithConstantW(cr::kCreativeMinimumPositiveClipW * 0.5F), {},
          100U, 100U);
  const cr::CreativeScreenPoint invalidProjection =
      cr::projectCreativeWorldPointToScreen(invalidMatrix, {}, 100U, 100U);
  const cr::CreativeScreenPoint overflow =
      cr::projectCreativeWorldPointToScreen(
          overflowingMatrix, {std::numeric_limits<float>::max(), 0.0F, 0.0F},
          100U, 100U);

  return expect(!invalidViewport.valid &&
                    invalidViewport.status ==
                        cr::CreativeScreenProjectionStatus::InvalidViewport,
                "zero drawable extent is rejected") &&
         expect(!invalidWorld.valid &&
                    invalidWorld.status ==
                        cr::CreativeScreenProjectionStatus::InvalidWorldPoint,
                "non-finite world point is rejected") &&
         expect(!behind.valid &&
                    behind.status ==
                        cr::CreativeScreenProjectionStatus::BehindCamera,
                "negative clip W is behind the camera") &&
         expect(!degenerate.valid &&
                    degenerate.status ==
                        cr::CreativeScreenProjectionStatus::DegenerateClipW,
                "near-zero positive clip W fails closed") &&
         expect(!invalidProjection.valid &&
                    invalidProjection.status ==
                        cr::CreativeScreenProjectionStatus::InvalidMatrix,
                "non-finite projection matrix is rejected before projection") &&
         expect(!overflow.valid &&
                    overflow.status == cr::CreativeScreenProjectionStatus::
                                           NonFiniteProjection,
                "finite inputs that overflow projection fail closed");
}

bool boundsReportFullPartialAndAbsentProjection() {
  const cr::CreativeScreenBounds full =
      cr::projectCreativeWorldBoundsToScreen(
          iggy3d::identityMat4(), {-1.0F, -1.0F, -0.5F},
          {1.0F, 1.0F, 0.5F}, 200U, 100U);
  const iggy3d::Mat4 split = matrixWithNegativeZAsW();
  const cr::CreativeScreenBounds partial =
      cr::projectCreativeWorldBoundsToScreen(
          split, {-0.5F, -0.5F, -1.0F}, {0.5F, 0.5F, 1.0F}, 200U, 100U);
  const cr::CreativeScreenBounds absent =
      cr::projectCreativeWorldBoundsToScreen(
          split, {-0.5F, -0.5F, 1.0F}, {0.5F, 0.5F, 2.0F}, 200U, 100U);
  const cr::CreativeScreenBounds outside =
      cr::projectCreativeWorldBoundsToScreen(
          iggy3d::identityMat4(), {2.0F, -0.5F, 0.0F},
          {3.0F, 0.5F, 0.0F}, 200U, 100U);
  const cr::CreativeScreenBounds invalid =
      cr::projectCreativeWorldBoundsToScreen(
          iggy3d::identityMat4(), {1.0F, 0.0F, 0.0F},
          {-1.0F, 1.0F, 1.0F}, 200U, 100U);

  return expect(full.valid && full.allCornersProjected &&
                    full.projectedCornerCount == 8U &&
                    full.intersectsViewport && near(full.minX, 0.0F) &&
                    near(full.minY, 0.0F) && near(full.maxX, 200.0F) &&
                    near(full.maxY, 100.0F),
                "all eight bounds corners project to exact pixel extent") &&
         expect(partial.valid && !partial.allCornersProjected &&
                    partial.projectedCornerCount == 4U &&
                    partial.status == cr::CreativeScreenProjectionStatus::
                                          PartiallyProjected,
                "near-plane crossing reports four projected corners") &&
         expect(!absent.valid && absent.projectedCornerCount == 0U &&
                    absent.status == cr::CreativeScreenProjectionStatus::
                                         NoProjectedCorners,
                "fully behind bounds report no projected corners") &&
         expect(outside.valid && !outside.intersectsViewport,
                "offscreen bounds remain projected but do not intersect") &&
         expect(!invalid.valid &&
                    invalid.status == cr::CreativeScreenProjectionStatus::
                                          InvalidWorldBounds,
                "reversed world bounds are rejected");
}

}  // namespace

int main() {
  bool ok = true;
  ok = pointsOwnPixelMappingAndVisibilityFacts() && ok;
  ok = pointFailuresAreExplicitAndFailClosed() && ok;
  ok = boundsReportFullPartialAndAbsentProjection() && ok;
  return ok ? 0 : 1;
}
