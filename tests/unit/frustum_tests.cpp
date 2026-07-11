#include "core/math/Frustum.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using iggy3d::Aabb3;
using iggy3d::aabbInFrustum;
using iggy3d::classifyAabbAgainstFrustum;
using iggy3d::ClipDepthRange;
using iggy3d::FrustumCull;
using iggy3d::FrustumPlanes;
using iggy3d::frustumPlanesFromClip;
using iggy3d::makeAabb3;
using iggy3d::Mat4;
using iggy3d::Plane;
using iggy3d::Vec3;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

Aabb3 box(float a, float b, float c, float d, float e, float f) {
  return makeAabb3(Vec3{a, b, c}, Vec3{d, e, f});
}

// Six inward planes bounding the cube [-10, 10]^3, order [left,right,bottom,top,near,far].
FrustumPlanes cubeFrustum() {
  return FrustumPlanes{
      Plane{Vec3{1.0F, 0.0F, 0.0F}, 10.0F},  Plane{Vec3{-1.0F, 0.0F, 0.0F}, 10.0F},
      Plane{Vec3{0.0F, 1.0F, 0.0F}, 10.0F},  Plane{Vec3{0.0F, -1.0F, 0.0F}, 10.0F},
      Plane{Vec3{0.0F, 0.0F, 1.0F}, 10.0F},  Plane{Vec3{0.0F, 0.0F, -1.0F}, 10.0F}};
}

Mat4 rowMajor(const std::array<float, 16>& v) {
  Mat4 m;
  m.m = v;
  return m;
}

bool insideBoxIsInside() {
  const FrustumCull c = classifyAabbAgainstFrustum(cubeFrustum(), box(-1, -1, -1, 1, 1, 1));
  return expect(c == FrustumCull::Inside, "small central box is Inside") &&
         expect(aabbInFrustum(cubeFrustum(), box(-1, -1, -1, 1, 1, 1)), "and visible");
}

bool outsideBoxIsCulled() {
  const FrustumCull c = classifyAabbAgainstFrustum(cubeFrustum(), box(20, 20, 20, 22, 22, 22));
  return expect(c == FrustumCull::Outside, "far-away box is Outside") &&
         expect(!aabbInFrustum(cubeFrustum(), box(20, 20, 20, 22, 22, 22)), "and culled");
}

bool straddlingBoxIntersects() {
  const FrustumCull c = classifyAabbAgainstFrustum(cubeFrustum(), box(8, -1, -1, 12, 1, 1));
  return expect(c == FrustumCull::Intersecting, "box crossing +x wall Intersects") &&
         expect(aabbInFrustum(cubeFrustum(), box(8, -1, -1, 12, 1, 1)), "and still visible");
}

bool beyondFarPlaneCulled() {
  // z in [11,13] is entirely past the far wall (z <= 10).
  return expect(!aabbInFrustum(cubeFrustum(), box(-1, -1, 11, 1, 1, 13)),
                "box past far plane is culled");
}

bool invalidBoxCulled() {
  const Aabb3 inverted = box(1, 1, 1, -1, -1, -1);
  return expect(classifyAabbAgainstFrustum(cubeFrustum(), inverted) == FrustumCull::Outside,
                "invalid box is Outside") &&
         expect(!aabbInFrustum(cubeFrustum(), inverted), "invalid box not drawn");
}

bool degeneratePlaneIsSkipped() {
  FrustumPlanes planes = cubeFrustum();
  planes[5] = Plane{Vec3{0.0F, 0.0F, 0.0F}, 0.0F};  // wipe the far plane
  // z[11,13] was culled only by the far plane; with it degenerate the box survives.
  return expect(aabbInFrustum(planes, box(-1, -1, 11, 1, 1, 13)),
                "degenerate plane no longer culls");
}

bool glOrthoExtractionClassifies() {
  // Symmetric orthographic clip mapping world [-10,10]^3 -> clip [-1,1]^3.
  const Mat4 clip = rowMajor({0.1F, 0.0F, 0.0F, 0.0F, 0.0F, 0.1F, 0.0F, 0.0F, 0.0F,
                              0.0F, 0.1F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F});
  const FrustumPlanes planes = frustumPlanesFromClip(clip, ClipDepthRange::NegativeOneToOne);
  return expect(aabbInFrustum(planes, box(-5, -5, -5, 5, 5, 5)),
                "box inside ortho volume is visible") &&
         expect(!aabbInFrustum(planes, box(15, -5, -5, 20, 5, 5)),
                "box outside +x is culled") &&
         expect(classifyAabbAgainstFrustum(planes, box(-9, -9, -9, 9, 9, 9)) ==
                    FrustumCull::Inside,
                "fully-contained box is Inside");
}

bool vulkanDepthExtractionClassifies() {
  // x,y map [-10,10]->[-1,1]; z maps [-10,10]->[0,1] via z*0.05 + 0.5 (Vulkan depth).
  const Mat4 clip = rowMajor({0.1F, 0.0F, 0.0F, 0.0F, 0.0F, 0.1F, 0.0F, 0.0F, 0.0F,
                              0.0F, 0.05F, 0.5F, 0.0F, 0.0F, 0.0F, 1.0F});
  const FrustumPlanes planes = frustumPlanesFromClip(clip, ClipDepthRange::ZeroToOne);
  return expect(aabbInFrustum(planes, box(-5, -5, -5, 5, 5, 5)),
                "box inside vulkan-depth volume is visible") &&
         expect(!aabbInFrustum(planes, box(-5, -5, 15, 5, 5, 20)),
                "box past far (z>10) is culled") &&
         expect(!aabbInFrustum(planes, box(-5, -5, -20, 5, 5, -15)),
                "box behind near (z<-10) is culled");
}

bool nonFiniteMatrixFailsSafe() {
  const float inf = std::numeric_limits<float>::infinity();
  const Mat4 clip = rowMajor({inf, 0.0F, 0.0F, 0.0F, 0.0F, 0.1F, 0.0F, 0.0F, 0.0F,
                              0.0F, 0.1F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F});
  const FrustumPlanes planes = frustumPlanesFromClip(clip, ClipDepthRange::ZeroToOne);
  // All planes degenerate -> nothing is culled (better to over-draw than wrongly cull).
  return expect(aabbInFrustum(planes, box(1000, 1000, 1000, 1001, 1001, 1001)),
                "non-finite clip matrix culls nothing (fail-safe)");
}

}  // namespace

int main() {
  const bool ok = insideBoxIsInside() && outsideBoxIsCulled() &&
                  straddlingBoxIntersects() && beyondFarPlaneCulled() &&
                  invalidBoxCulled() && degeneratePlaneIsSkipped() &&
                  glOrthoExtractionClassifies() && vulkanDepthExtractionClassifies() &&
                  nonFiniteMatrixFailsSafe();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
